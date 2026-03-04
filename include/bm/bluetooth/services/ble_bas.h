/*
 * Copyright (c) 2012 - 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/** @file
 *
 * @defgroup ble_bas Battery Service
 * @{
 * @brief Battery Service.
 */
#ifndef BLE_BAS_H__
#define BLE_BAS_H__

#include <stdint.h>
#include <stdbool.h>
#include <ble.h>
#include <bm/softdevice_handler/nrf_sdh_ble.h>
#include <bm/bluetooth/services/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define a Battery service instance.
 *
 * Define a battery service instance and register it as a Bluetooth event observer.
 */
#define BLE_BAS_DEF(_name)                                                                         \
	static struct ble_bas _name;                                                               \
	extern void ble_bas_on_ble_evt(const ble_evt_t *ble_evt, void *ctx);                       \
	NRF_SDH_BLE_OBSERVER(_name##_obs, ble_bas_on_ble_evt, &_name, HIGH)

/** @brief Default security configuration. */
#define BLE_BAS_CONFIG_SEC_MODE_DEFAULT                                                            \
	{                                                                                          \
		.battery_lvl_char = {                                                              \
			.read = BLE_GAP_CONN_SEC_MODE_OPEN,                                        \
			.cccd_write = BLE_GAP_CONN_SEC_MODE_OPEN,                                  \
		},                                                                                 \
		.battery_report_ref.read = BLE_GAP_CONN_SEC_MODE_OPEN,                             \
	}

/**
 * @brief Battery service event types.
 */
enum ble_bas_evt_type {
	/**
	 * @brief Battery level notification enabled.
	 */
	BLE_BAS_EVT_NOTIFICATION_ENABLED,
	/**
	 * @brief Battery level notification disabled.
	 */
	BLE_BAS_EVT_NOTIFICATION_DISABLED,
	/**
	 * @brief Error event.
	 */
	BLE_BAS_EVT_ERROR,
};

/**
 * @brief BAS client context structure holding host-related state.
 */
struct ble_bas_client_context {
	/**
	 * @brief Indicate if the peer has enabled notification of the battery characteristic.
	 */
	bool is_notification_enabled;
};

/**
 * @brief Battery service event.
 */
struct  ble_bas_evt {
	/**
	 * @brief Event type.
	 */
	enum ble_bas_evt_type evt_type;
	/**
	 * @brief Connection handle for which the event applies.
	 */
	uint16_t conn_handle;
	union {
		/** @ref BLE_BAS_EVT_ERROR event data. */
		struct {
			/** Error reason. */
			uint32_t reason;
		} error;
	};
};

/* Forward declaration */
struct ble_bas;

/**
 * @brief Battery service event handler type.
 */
typedef void (*ble_bas_evt_handler_t)(struct ble_bas *bas, const struct ble_bas_evt *evt);

/**
 * @brief Battery service configuration.
 */
struct ble_bas_config {
	/**
	 * @brief Battery service event handler.
	 */
	ble_bas_evt_handler_t evt_handler;
	/**
	 * @brief Report Reference Descriptor.
	 *
	 * If provided, a Report Reference descriptor with the specified
	 * value will be added to the Battery Level characteristic.
	 */
	struct {
		/**
		 * @brief Report ID.
		 *
		 * A non-zero value indicates that there is more than one instance
		 * of the same Report Type.
		 */
		uint8_t report_id;
		/**
		 * @brief Report type.
		 */
		uint8_t report_type;
	} *report_ref;
	/**
	 * @brief Allow notifications.
	 */
	bool can_notify;
	/**
	 * @brief Initial battery level.
	 */
	uint8_t battery_level;
	/** Characteristic security. */
	struct {
		/**  Battery Level characteristic */
		struct {
			/** Security requirement for reading battery level characteristic value. */
			ble_gap_conn_sec_mode_t read;
			/** Security requirement for writing battery level characteristic CCCD. */
			ble_gap_conn_sec_mode_t cccd_write;
		} battery_lvl_char;
		/** Battery Service report reference. */
		struct {
			/** Security requirement for reading Battery Service report reference. */
			ble_gap_conn_sec_mode_t read;
		} battery_report_ref;
	} sec_mode;
};

/**
 * @brief Battery Service structure.
 */
struct ble_bas {
	/**
	 * @brief Battery Service event handler.
	 */
	ble_bas_evt_handler_t evt_handler;
	/**
	 * @brief Battery service handle.
	 */
	uint16_t service_handle;
	/**
	 * @brief Report reference descriptor handler.
	 */
	uint16_t report_ref_handle;
	/**
	 * @brief Battery level characteristic handles.
	 */
	ble_gatts_char_handles_t battery_level_handles;
	/**
	 * @brief Battery level.
	 */
	uint8_t battery_level;
	/**
	 * @brief Whether notifications of battery level changes are supported.
	 */
	bool can_notify;
	/**
	 * @brief Link context with handles of all current connections and its context.
	 */
	struct ble_bas_client_context contexts[CONFIG_NRF_SDH_BLE_TOTAL_LINK_COUNT];
};

/**
 * @brief Initialize the battery service.
 *
 * @param bas Battery service.
 * @param bas_config Battery service configuration.
 *
 * @retval NRF_SUCCESS On success.
 * @retval NRF_ERROR_NULL If @p bas or @p bas_config are @c NULL.
 * @return In addition, this function may return any error
 *	   returned by the following SoftDevice functions:
 *	   - @ref sd_ble_gatts_service_add()
 *	   - @ref sd_ble_gatts_characteristic_add()
 *	   - @ref sd_ble_gatts_descriptor_add()
 */
uint32_t ble_bas_init(struct ble_bas *bas, const struct ble_bas_config *bas_config);

/**
 * @brief Sync notification flag with the restored CCCD value from flash.
 *
 * @details When a bonded peer reconnects, the peer manager restores the CCCD
 * values in the GATT database from flash. However, the local notification flag
 * in the client context is not updated automatically. This function reads the
 * restored CCCD and syncs the flag so that @ref ble_bas_battery_level_update
 * can correctly decide whether to notify the peer.
 *
 * Call this from @c PM_EVT_LOCAL_DB_CACHE_APPLIED.
 *
 * @param bas Battery service.
 * @param conn_handle Connection handle of the reconnected bonded peer.
 *
 * @retval NRF_SUCCESS On success.
 * @retval NRF_ERROR_NULL If @p bas is @c NULL.
 * @retval NRF_ERROR_NOT_FOUND If no client context exists for @p conn_handle.
 * @return In addition, this function may return any error
 *	   returned by the following SoftDevice function:
 *	   - @ref sd_ble_gatts_value_get()
 */
uint32_t ble_bas_cccd_sync(struct ble_bas *bas, uint16_t conn_handle);

/**
 * @brief Update battery level.
 *
 * Updates the battery level in the GATT database. If the peer with the given
 * @p conn_handle has notifications enabled (CCCD written), a notification is
 * sent automatically. Otherwise, only the GATT database value is updated.
 *
 * @param bas Battery service.
 * @param conn_handle Connection handle.
 * @param battery_level Battery level (in percent of full capacity).
 *
 * @retval NRF_SUCCESS On success.
 * @retval NRF_ERROR_NULL If @p bas is @c NULL.
 * @retval NRF_ERROR_NOT_FOUND If no client context exists for @p conn_handle.
 * @return In addition, this function may return any error
 *	   returned by the following SoftDevice functions:
 *	   - @ref sd_ble_gatts_value_set()
 *	   - @ref sd_ble_gatts_hvx()
 */
uint32_t ble_bas_battery_level_update(struct ble_bas *bas, uint16_t conn_handle,
				      uint8_t battery_level);

#ifdef __cplusplus
}
#endif

#endif /* BLE_BAS_H__ */

/** @} */
