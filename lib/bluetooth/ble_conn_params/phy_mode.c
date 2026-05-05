/*
 * Copyright (c) 2012-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ble_gap.h>
#include <bm/bluetooth/ble_conn_params.h>
#include <bm/softdevice_handler/nrf_sdh_ble.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ble_conn_params, CONFIG_BLE_CONN_PARAMS_LOG_LEVEL);

extern void ble_conn_params_event_send(const struct ble_conn_params_evt *evt);

/* App-configured PHY allow-mask derived from Kconfig. */
#define BLE_CONN_PARAMS_PHY_APP_MASK CONFIG_BLE_CONN_PARAMS_PHY
/* Auto mode fallback uses 1M as a conservative recovery PHY. */
#define BLE_CONN_PARAMS_PHY_AUTO_FALLBACK BLE_GAP_PHY_1MBPS
#define BLE_CONN_PARAMS_PHY_IS_AUTO (BLE_CONN_PARAMS_PHY_APP_MASK == BLE_GAP_PHY_AUTO)
#define BLE_CONN_PARAMS_PHY_HAS_STACK_SUPPORT ((BLE_CONN_PARAMS_PHY_APP_MASK & \
						BLE_GAP_PHYS_SUPPORTED) != 0)
#define BLE_CONN_PARAMS_PHY_EFFECTIVE_APP_MASK \
	(BLE_CONN_PARAMS_PHY_IS_AUTO ? BLE_GAP_PHYS_SUPPORTED : \
				       (BLE_CONN_PARAMS_PHY_APP_MASK & BLE_GAP_PHYS_SUPPORTED))
/* Recovery mask used for one fallback retry on resource errors. */
#define BLE_CONN_PARAMS_PHY_FALLBACK_MASK \
	(BLE_CONN_PARAMS_PHY_IS_AUTO ? BLE_CONN_PARAMS_PHY_AUTO_FALLBACK : \
				       BLE_CONN_PARAMS_PHY_APP_MASK)

static struct {
	/* Last requested/effective PHY preference for this link. */
	ble_gap_phys_t phy_mode;
	/* Deferred retry flag when SoftDevice reports busy/collision. */
	uint8_t phy_mode_update_pending : 1;
} links[CONFIG_NRF_SDH_BLE_TOTAL_LINK_COUNT] = {
	[0 ... CONFIG_NRF_SDH_BLE_TOTAL_LINK_COUNT - 1] = {
		.phy_mode.tx_phys = BLE_CONN_PARAMS_PHY_APP_MASK,
		.phy_mode.rx_phys = BLE_CONN_PARAMS_PHY_APP_MASK,
	},
};

BUILD_ASSERT(BLE_CONN_PARAMS_PHY_IS_AUTO || BLE_CONN_PARAMS_PHY_HAS_STACK_SUPPORT,
	     "Invalid PHY config");

static const char *phy_mask_to_str(uint8_t phy_mask)
{
	switch (phy_mask) {
	case BLE_GAP_PHY_AUTO:
		return "AUTO";
	case BLE_GAP_PHY_NOT_SET:
		return "NOT_SET";
	case BLE_GAP_PHY_1MBPS:
		return "1M";
	case BLE_GAP_PHY_2MBPS:
		return "2M";
	case BLE_GAP_PHY_CODED:
		return "CODED";
	case BLE_GAP_PHY_1MBPS | BLE_GAP_PHY_2MBPS:
		return "1M|2M";
	case BLE_GAP_PHY_1MBPS | BLE_GAP_PHY_CODED:
		return "1M|CODED";
	case BLE_GAP_PHY_2MBPS | BLE_GAP_PHY_CODED:
		return "2M|CODED";
	case BLE_GAP_PHY_1MBPS | BLE_GAP_PHY_2MBPS | BLE_GAP_PHY_CODED:
		return "1M|2M|CODED";
	default:
		return "UNKNOWN";
	}
}

static ble_gap_phys_t radio_phy_mode_prepare(ble_gap_phys_t phy_mode)
{
	/* Apply app policy on top of what this SoftDevice can support. */
	const uint8_t app_mask = BLE_CONN_PARAMS_PHY_EFFECTIVE_APP_MASK;
	ble_gap_phys_t phys = phy_mode;

	if (phys.tx_phys == BLE_GAP_PHY_AUTO) {
		/* AUTO means "choose from allowed PHYs", so expand to effective allow-mask. */
		phys.tx_phys = app_mask;
	} else if (phys.tx_phys != BLE_GAP_PHY_NOT_SET) {
		/* Keep caller/peer intent, but clamp it to the allowed PHY set. */
		phys.tx_phys &= app_mask;
	}

	if (phys.rx_phys == BLE_GAP_PHY_AUTO) {
		/* Same policy for RX direction: AUTO resolves to the allowed PHY mask. */
		phys.rx_phys = app_mask;
	} else if (phys.rx_phys != BLE_GAP_PHY_NOT_SET) {
		/* Intersection avoids broadening an explicit peer/application request. */
		phys.rx_phys &= app_mask;
	}

	/* If filtering removed all bits, fall back to app mask or 1M in auto mode. */
	if (phys.tx_phys == 0) {
		phys.tx_phys = app_mask ? app_mask : BLE_CONN_PARAMS_PHY_FALLBACK_MASK;
	}
	if (phys.rx_phys == 0) {
		phys.rx_phys = app_mask ? app_mask : BLE_CONN_PARAMS_PHY_FALLBACK_MASK;
	}

	return phys;
}

static void radio_phy_mode_update(uint16_t conn_handle, int idx)
{
	uint32_t nrf_err;
	ble_gap_phys_t phys;
	struct ble_conn_params_evt app_evt = {
		.evt_type = BLE_CONN_PARAMS_EVT_ERROR,
		.conn_handle = conn_handle,
	};

	for (int attempt = 0; attempt < 2; attempt++) {
		/* Always sanitize before calling into SoftDevice. */
		phys = radio_phy_mode_prepare(links[idx].phy_mode);

		nrf_err = sd_ble_gap_phy_update(conn_handle, &phys);
		if (nrf_err == NRF_SUCCESS) {
			return;
		} else if (nrf_err == NRF_ERROR_BUSY) {
			/* Retry */
			links[idx].phy_mode_update_pending = true;
			LOG_DBG("Failed PHY update procedure, another procedure is ongoing, "
				"Will retry");
			return;
		} else if (nrf_err == NRF_ERROR_RESOURCES && attempt == 0) {
			/* Retry once with fallback mask, avoids recursive call chain. */
			LOG_WRN("Failed PHY update procedure. Retrying with app fallback PHY");
			LOG_DBG("GAP event length (%d) may be too small",
				CONFIG_NRF_SDH_BLE_GAP_EVENT_LENGTH);
			links[idx].phy_mode.tx_phys = BLE_CONN_PARAMS_PHY_FALLBACK_MASK;
			links[idx].phy_mode.rx_phys = BLE_CONN_PARAMS_PHY_FALLBACK_MASK;
			continue;
		}

		LOG_ERR("Failed PHY update procedure, nrf_error %#x", nrf_err);
		app_evt.error.reason = nrf_err;
		ble_conn_params_event_send(&app_evt);
		return;
	}
}

static void on_radio_phy_mode_update_evt(uint16_t conn_handle, int idx,
					 const ble_gap_evt_phy_update_t *evt)
{
	struct ble_conn_params_evt app_evt = {
		.conn_handle = conn_handle,
	};

	if (evt->status == BLE_HCI_STATUS_CODE_SUCCESS) {
		links[idx].phy_mode_update_pending = false;
		links[idx].phy_mode.tx_phys = evt->tx_phy;
		links[idx].phy_mode.rx_phys = evt->rx_phy;
		LOG_INF("PHY mode selected for peer %#x: tx=%s (%u), rx=%s (%u)", conn_handle,
			phy_mask_to_str(links[idx].phy_mode.tx_phys), links[idx].phy_mode.tx_phys,
			phy_mask_to_str(links[idx].phy_mode.rx_phys), links[idx].phy_mode.rx_phys);
	} else if (evt->status == BLE_HCI_DIFFERENT_TRANSACTION_COLLISION) {
		/* Retry */
		links[idx].phy_mode_update_pending = true;
		LOG_DBG("Failed to initiate PHY update procedure, another procedure is ongoing, "
			"Will retry");
	} else {
		links[idx].phy_mode_update_pending = false;
		LOG_ERR("PHY update failed with status %u for peer %#x", evt->status,
			conn_handle);

		/* Send error event to application and return */
		app_evt.evt_type = BLE_CONN_PARAMS_EVT_ERROR;
		app_evt.error.reason = evt->status;
		ble_conn_params_event_send(&app_evt);
		return;
	}

	app_evt.evt_type = BLE_CONN_PARAMS_EVT_RADIO_PHY_MODE_UPDATED;
	app_evt.phy_update_evt = *evt;

	ble_conn_params_event_send(&app_evt);
}

static void on_radio_phy_mode_update_request_evt(uint16_t conn_handle, int idx,
						 const ble_gap_evt_phy_update_request_t *evt)
{
	ble_gap_phys_t peer_phys = evt->peer_preferred_phys;

	LOG_INF("Peer %#x requested PHY preference mask: tx=%s (%u), rx=%s (%u)", conn_handle,
		phy_mask_to_str(evt->peer_preferred_phys.tx_phys),
		evt->peer_preferred_phys.tx_phys,
		phy_mask_to_str(evt->peer_preferred_phys.rx_phys),
		evt->peer_preferred_phys.rx_phys);

	/* Respect peer request only within app + SoftDevice allowed masks. */
	links[idx].phy_mode = radio_phy_mode_prepare(peer_phys);

	radio_phy_mode_update(conn_handle, idx);
}

static void on_connected(uint16_t conn_handle, int idx)
{
	if (IS_ENABLED(CONFIG_BLE_CONN_PARAMS_INITIATE_PHY_UPDATE)) {
		LOG_INF("Initiating PHY update procedure for peer %#x", conn_handle);
		radio_phy_mode_update(conn_handle, idx);
	}
}

static void on_disconnected(uint16_t conn_handle, int idx)
{
	ARG_UNUSED(conn_handle);

	links[idx].phy_mode_update_pending = false;
}

static void on_ble_evt(const ble_evt_t *evt, void *ctx)
{
	const uint16_t conn_handle = evt->evt.common_evt.conn_handle;
	const int idx = nrf_sdh_ble_idx_get(conn_handle);

	__ASSERT(idx >= 0, "Invalid idx %d for conn_handle %#x, evt_id %#x",
		 idx, conn_handle, evt->header.evt_id);

	switch (evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		on_connected(conn_handle, idx);
		/* There is no pending PHY update or the update was just attempted
		 * and retry is not needed immediately. Return here.
		 */
		return;
	case BLE_GAP_EVT_DISCONNECTED:
		on_disconnected(conn_handle, idx);
		/* No neeed to retry PHY update if disconnected. Return here. */
		return;

	case BLE_GAP_EVT_PHY_UPDATE:
		on_radio_phy_mode_update_evt(conn_handle, idx, &evt->evt.gap_evt.params.phy_update);
		break;
	case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
		on_radio_phy_mode_update_request_evt(
			conn_handle, idx, &evt->evt.gap_evt.params.phy_update_request);
		break;

	default:
		/* Ignore */
		break;
	}

	/* Retry any procedures that were busy */
	if (links[idx].phy_mode_update_pending) {
		links[idx].phy_mode_update_pending = false;
		radio_phy_mode_update(conn_handle, idx);
	}
}
NRF_SDH_BLE_OBSERVER(ble_observer, on_ble_evt, NULL, HIGH);

uint32_t ble_conn_params_phy_radio_mode_set(uint16_t conn_handle, ble_gap_phys_t phy_pref)
{
	const int idx = nrf_sdh_ble_idx_get(conn_handle);

	if (idx < 0) {
		return NRF_ERROR_INVALID_PARAM;
	}

	links[idx].phy_mode = phy_pref;
	radio_phy_mode_update(conn_handle, idx);

	return NRF_SUCCESS;
}

uint32_t ble_conn_params_phy_radio_mode_get(uint16_t conn_handle, ble_gap_phys_t *phy_pref)
{
	const int idx = nrf_sdh_ble_idx_get(conn_handle);

	if (idx < 0) {
		return NRF_ERROR_INVALID_PARAM;
	}

	if (!phy_pref) {
		return NRF_ERROR_NULL;
	}

	*phy_pref = links[idx].phy_mode;

	return NRF_SUCCESS;
}
