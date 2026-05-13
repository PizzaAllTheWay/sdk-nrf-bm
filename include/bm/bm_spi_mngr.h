/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @defgroup bm_spi_mngr NCS Bare Metal SPI transaction manager
 * @{
 *
 * @brief SPI master transaction queue on top of @ref nrfx_spim.
 *
 * @details Transactions wait in a FIFO queue and run one after another on the bus. Each
 *          transaction is one or more TX and RX steps in order. You can hook @c begin_callback and
 *          @c end_callback, and optionally pass a different @ref nrfx_spim_config_t per
 *          transaction. That lets you change pins between jobs, for example a different software
 *          chip select line. Pins must still be valid for this SPIM instance and on the same GPIO
 *          port when your SoC or board wiring requires it.
 *
 *          @ref bm_spi_mngr_schedule adds a transaction and returns right away in a nonblocking
 *          way. When it finishes, @c end_callback runs from the SPIM interrupt. @ref
 *          bm_spi_mngr_perform does the same work but waits until it is done in a blocking way.
 *          Only use @c idle_fn from normal code, not from an interrupt handler.
 *
 *          Connect and enable the SPIM interrupt, for example @ref BM_IRQ_DIRECT_CONNECT, before
 *          @ref bm_spi_mngr_init.
 */

#ifndef BM_SPI_MNGR_H__
#define BM_SPI_MNGR_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nrfx_spim.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro for creating a simple SPI transfer initializer.
 *
 * @param[in] _p_tx_data Pointer to data to send, or NULL if @a _tx_length is zero.
 * @param[in] _tx_length Number of bytes to send, must fit in @c uint8_t.
 * @param[in] _p_rx_data Pointer to buffer for received data, or NULL if @a _rx_length is zero.
 * @param[in] _rx_length Number of bytes to receive, must fit in @c uint8_t.
 */
#define BM_SPI_MNGR_TRANSFER(_p_tx_data, _tx_length, _p_rx_data, _rx_length) \
	{                                                                                         \
		.p_tx_data = (uint8_t const *)(_p_tx_data),                                       \
		.tx_length = (uint8_t)(_tx_length),                                               \
		.p_rx_data = (uint8_t *)(_p_rx_data),                                             \
		.rx_length = (uint8_t)(_rx_length),                                               \
	}

/**
 * @brief Transaction end callback.
 *
 * @param[in] result @c 0 on success. On failure, a negative error code from nrfx SPIM or from this
 *                   module (for example @c -EIO).
 * @param[in] p_user_data Pointer passed through from @ref bm_spi_mngr_transaction_t member
 *                        @c p_user_data.
 */
typedef void (*bm_spi_mngr_callback_end_t)(int result, void *p_user_data);

/**
 * @brief Transaction begin callback, runs from the SPIM event handler context.
 *
 * @param[in] p_user_data Pointer passed through from @ref bm_spi_mngr_transaction_t member
 *                        @c p_user_data.
 */
typedef void (*bm_spi_mngr_callback_begin_t)(void *p_user_data);

/**
 * @brief SPI transfer descriptor, one segment of a transaction.
 */
typedef struct {
	/* Pointer to data to send. */
	uint8_t const *p_tx_data;
	/* Number of bytes to send. */
	uint8_t tx_length;
	/* Pointer to buffer for received data. */
	uint8_t *p_rx_data;
	/* Number of bytes to receive. */
	uint8_t rx_length;
} bm_spi_mngr_transfer_t;

/**
 * @brief SPI transaction descriptor.
 *
 * @note If @ref p_required_spim_cfg is non NULL it must remain valid until the transaction
 *       completes. If it is NULL, the default configuration passed to @ref bm_spi_mngr_init is
 *       used.
 *
 * @note If @ref p_required_spim_cfg differs from the configuration currently in use, the module
 *       initializes the SPIM instance again before starting the transaction.
 */
typedef struct {
	/* User function invoked immediately before the first transfer of the transaction starts. */
	bm_spi_mngr_callback_begin_t begin_callback;
	/* User function invoked when the transaction completes or aborts after an error. */
	bm_spi_mngr_callback_end_t end_callback;
	/* Opaque pointer passed to the optional begin and end callbacks. */
	void *p_user_data;
	/* Array of transfers that make up the transaction. */
	bm_spi_mngr_transfer_t const *p_transfers;
	/* Number of entries in @ref p_transfers. */
	uint8_t number_of_transfers;
	/* Optional SPIM hardware configuration for this transaction; NULL selects the default. */
	nrfx_spim_config_t const *p_required_spim_cfg;
} bm_spi_mngr_transaction_t;

/**
 * @brief SPI manager control block (writable runtime state).
 */
typedef struct {
	/* Transaction currently being executed (NULL when idle). */
	bm_spi_mngr_transaction_t const *volatile p_current_transaction;
	/* Default SPIM configuration (copy of the argument to @ref bm_spi_mngr_init). */
	nrfx_spim_config_t default_configuration;
	nrfx_spim_config_t const *p_current_configuration;
	/* Index of the active transfer within @ref p_current_transaction. */
	uint8_t volatile current_transfer_idx;
} bm_spi_mngr_cb_t;

/**
 * @brief Transaction queue (backing store + Zephyr @ref ring_buf).
 *
 * @details @ref bm_spi_mngr_t::p_queue points here. Member @c size is the maximum number of
 *          pending transactions (not counting the transaction currently in progress). Member
 *          @c p_byte_storage must provide @c size contiguous pointer-sized slots, each
 *          @c sizeof(bm_spi_mngr_transaction_t const *), in bytes.
 */
typedef struct {
	/* Maximum number of pending transactions (not counting the one in progress). */
	size_t size;
	/* Zephyr @c ring_buf used as a FIFO of transaction pointers. */
	struct ring_buf ring;
	/* Backing memory for @c ring, length at least @c size * @c sizeof(transaction pointer). */
	uint8_t *p_byte_storage;
} bm_spi_mngr_queue_t;

/**
 * @brief SPI transaction manager instance.
 *
 * @note Instantiate with @ref BM_SPI_MNGR_DEF. Do not modify fields directly.
 */
typedef struct {
	/* Control block for this instance. */
	bm_spi_mngr_cb_t *p_bm_spi_mngr_cb;
	/* Pending transaction queue. */
	bm_spi_mngr_queue_t *p_queue;
	/* nrfx SPIM driver instance. */
	nrfx_spim_t *p_spim;
} bm_spi_mngr_t;

/**
 * @brief Macro for defining an SPI transaction manager instance.
 *
 * @details This macro allocates a static buffer for the transaction queue. Therefore, use it in
 *          only one place in the code for a given instance name.
 *
 * @note The queue size is the maximum number of pending transactions not counting the one that is
 *       running. For an empty queue with size of for example 4 elements, it is possible to schedule
 *       up to 5 transactions.
 *
 * @param[in] _name Name of the instance to be created.
 * @param[in] _queue_size Size of the transaction queue (maximum number of pending transactions).
 * @param[in] _spim_inst Index of the SPIM hardware instance to be used.
 */
#define BM_SPI_MNGR_DEF(_name, _queue_size, _spim_inst)                                            \
	static uint8_t _name##_queue_bytes[(_queue_size) *                                         \
					   sizeof(bm_spi_mngr_transaction_t const *)];             \
	static bm_spi_mngr_queue_t _name##_queue = {                                               \
		.size = (_queue_size),                                                             \
		.p_byte_storage = _name##_queue_bytes,                                             \
	};                                                                                         \
	static bm_spi_mngr_cb_t _name##_cb;                                                        \
	static nrfx_spim_t _name##_spim = NRFX_SPIM_INSTANCE(_spim_inst);                          \
	static const bm_spi_mngr_t _name = {                                                       \
		.p_bm_spi_mngr_cb = &_name##_cb,                                                   \
		.p_queue = &_name##_queue,                                                         \
		.p_spim = &_name##_spim,                                                           \
	}

/**
 * @brief Initialize the SPI manager and the underlying SPIM driver.
 *
 * @details Initializes the transaction queue and calls @ref nrfx_spim_init with the internal event
 *          handler. On success, clears the current transaction pointer and stores this SPIM
 *          configuration.
 *
 * @param[in] mgr Manager instance to initialize.
 * @param[in] p_default_spim_cfg  Pointer to the SPIM driver configuration. This configuration
 *                                will be used whenever the scheduled transaction has
 *                                @c p_required_spim_cfg set to NULL.
 *
 * @retval 0 On success.
 * @return Negative error code from @ref nrfx_spim_init on failure.
 */
int bm_spi_mngr_init(bm_spi_mngr_t const *mgr, nrfx_spim_config_t const *p_default_spim_cfg);

/**
 * @brief Uninitialize the SPI manager and SPIM.
 *
 * @param[in] mgr Manager instance.
 */
void bm_spi_mngr_uninit(bm_spi_mngr_t const *mgr);

/**
 * @brief Schedule an SPI transaction.
 *
 * @details The transaction is enqueued and started as soon as the SPI bus is available, thus when
 *          all previously scheduled transactions have been finished (possibly immediately).
 *
 *          If @ref bm_spi_mngr_transaction_t::p_required_spim_cfg is set to a non-NULL value, the
 *          module compares it with the current configuration and reinitializes the SPIM instance
 *          with the new parameters if any differences are found. If
 *          @ref bm_spi_mngr_transaction_t::p_required_spim_cfg is NULL, the default configuration
 *          passed to @ref bm_spi_mngr_init is used instead.
 *
 * @param[in] mgr SPI transaction manager instance.
 * @param[in] p_transaction Descriptor of the transaction to be scheduled.
 *
 * @retval 0 On success.
 * @retval -ENOMEM If the queue is full.
 */
int bm_spi_mngr_schedule(bm_spi_mngr_t const *mgr,
			 bm_spi_mngr_transaction_t const *p_transaction);

/**
 * @brief Schedule a transaction and wait until it is finished.
 *
 * @details This function schedules a transaction that consists of one or more transfers and waits
 *          until it is finished.
 *
 * @param[in] mgr SPI transaction manager instance.
 * @param[in] p_config SPIM configuration for this transaction. If NULL, the default configuration
 *                     passed to @ref bm_spi_mngr_init is used.
 * @param[in] p_transfers Array of transfers to be performed.
 * @param[in] number_of_transfers Number of transfers to be performed.
 * @param[in] idle_fn User function called while waiting, or NULL if not needed. Must not be called
 *                    from ISR context.
 *
 * @retval 0 On success.
 * @retval -ENOMEM If the queue is full.
 * @return Negative error code from the @ref nrfx_spim driver or from this module during the
 *         transaction.
 */
int bm_spi_mngr_perform(bm_spi_mngr_t const *mgr, nrfx_spim_config_t const *p_config,
			bm_spi_mngr_transfer_t const *p_transfers, uint8_t number_of_transfers,
			void (*idle_fn)(void));

/**
 * @brief Get the current state of an SPI transaction manager instance.
 *
 * @param[in] mgr SPI transaction manager instance.
 *
 * @retval true If all scheduled transactions have been finished.
 * @retval false Otherwise.
 */
static inline bool bm_spi_mngr_is_idle(bm_spi_mngr_t const *mgr)
{
	return mgr->p_bm_spi_mngr_cb->p_current_transaction == NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* BM_SPI_MNGR_H__ */

/** @} */
