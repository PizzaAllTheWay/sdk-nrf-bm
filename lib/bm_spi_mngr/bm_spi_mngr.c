/* Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bm/bm_spi_mngr.h>

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>

/* State shared between bm_spi_mngr_perform() and its internal end callback. The callback writes
 * the result and clears transaction_in_progress to release the caller from its wait loop.
 */
typedef volatile struct {
	bool transaction_in_progress;
	int transaction_result;
} bm_spi_mngr_cb_data_t;

/* Add one transaction pointer to the back of the queue. IRQs are locked because the queue is
 * accessed both from the caller of bm_spi_mngr_schedule() and from the SPIM interrupt handler
 * that starts the next queued transaction when one finishes.
 */
static int queue_push(bm_spi_mngr_t const *p_bm_spi_mngr, void const *p_src)
{
	struct ring_buf *rb = &p_bm_spi_mngr->p_queue->ring;
	const uint32_t rec = (uint32_t)sizeof(bm_spi_mngr_transaction_t const *);
	int ret;
	unsigned int key = irq_lock();

	if (ring_buf_space_get(rb) < rec) {
		ret = -ENOMEM;
	} else {
		uint32_t w = ring_buf_put(rb, (uint8_t const *)p_src, rec);

		ret = (w == rec) ? 0 : -ENOMEM;
	}

	irq_unlock(key);
	return ret;
}

/* Take one transaction pointer from the front of the queue. IRQs are locked for the same reason
 * as in queue_push().
 */
static int queue_pop(bm_spi_mngr_t const *p_bm_spi_mngr, void *p_element)
{
	struct ring_buf *rb = &p_bm_spi_mngr->p_queue->ring;
	const uint32_t rec = (uint32_t)sizeof(bm_spi_mngr_transaction_t const *);
	int ret;
	unsigned int key = irq_lock();

	if (ring_buf_size_get(rb) < rec) {
		ret = -ENOENT;
	} else {
		uint32_t r = ring_buf_get(rb, (uint8_t *)p_element, rec);

		ret = (r == rec) ? 0 : -ENOENT;
	}

	irq_unlock(key);
	return ret;
}

/* Start the active segment of the current transaction via nrfx_spim_xfer(). */
static int start_transfer(bm_spi_mngr_t const *p_bm_spi_mngr)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);

	/* Use a local copy so we do not read two volatile fields in one expression. */
	uint8_t curr_transfer_idx = p_bm_spi_mngr->p_bm_spi_mngr_cb->current_transfer_idx;
	bm_spi_mngr_cb_t *p_cb = p_bm_spi_mngr->p_bm_spi_mngr_cb;
	bm_spi_mngr_transaction_t const *p_txn = p_cb->p_current_transaction;
	bm_spi_mngr_transfer_t const *p_transfer = &p_txn->p_transfers[curr_transfer_idx];

	nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TRX(
		p_transfer->p_tx_data, p_transfer->tx_length, p_transfer->p_rx_data,
		p_transfer->rx_length);

	return nrfx_spim_xfer(p_bm_spi_mngr->p_spim, &xfer, 0);
}

/* If the transaction defines begin_callback, call it before the first transfer starts. */
static void transaction_begin_signal(bm_spi_mngr_t const *p_bm_spi_mngr)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);

	bm_spi_mngr_transaction_t const *p_current_transaction =
		p_bm_spi_mngr->p_bm_spi_mngr_cb->p_current_transaction;

	if (p_current_transaction->begin_callback != NULL) {
		void *p_user_data = p_current_transaction->p_user_data;

		p_current_transaction->begin_callback(p_user_data);
	}
}

/* If end_callback is set, call it with the transaction result (success or error code). */
static void transaction_end_signal(bm_spi_mngr_t const *p_bm_spi_mngr, int result)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);

	bm_spi_mngr_transaction_t const *p_current_transaction =
		p_bm_spi_mngr->p_bm_spi_mngr_cb->p_current_transaction;

	if (p_current_transaction->end_callback != NULL) {
		void *p_user_data = p_current_transaction->p_user_data;

		p_current_transaction->end_callback(result, p_user_data);
	}
}

static void spim_event_handler(nrfx_spim_event_t const *p_event, void *p_context);

/* Start the next transaction from the queue. Called from the scheduling path when the bus is
 * idle, and from the SPIM interrupt handler after a transaction finishes to force a switch to
 * the next one. The current transaction pointer is only cleared when the queue is empty, so
 * back-to-back transactions never look idle in between.
 */
static void start_pending_transaction(bm_spi_mngr_t const *p_bm_spi_mngr, bool switch_transaction)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);

	while (1) {
		bool start_transaction = false;
		bm_spi_mngr_cb_t *p_cb = p_bm_spi_mngr->p_bm_spi_mngr_cb;

		/* IRQs are locked while touching the queue and the current transaction pointer
		 * because both are also accessed from the SPIM interrupt handler.
		 */
		unsigned int key = irq_lock();

		if (switch_transaction || bm_spi_mngr_is_idle(p_bm_spi_mngr)) {
			if (queue_pop(p_bm_spi_mngr,
				      (void *)(&p_cb->p_current_transaction)) == 0) {
				start_transaction = true;
			} else {
				/* Queue is empty, mark the manager as idle. */
				p_cb->p_current_transaction = NULL;
			}
		}
		irq_unlock(key);

		if (!start_transaction) {
			return;
		}

		/* A transaction can carry its own SPIM configuration, otherwise the default
		 * configuration from bm_spi_mngr_init() is used.
		 */
		nrfx_spim_config_t const *p_instance_cfg;

		if (p_cb->p_current_transaction->p_required_spim_cfg == NULL) {
			p_instance_cfg = &p_cb->default_configuration;
		} else {
			p_instance_cfg = p_cb->p_current_transaction->p_required_spim_cfg;
		}

		int result;

		/* Reinitialize the SPIM instance only when this transaction needs a different
		 * configuration than the one currently active (different pins, frequency, mode,
		 * and so on).
		 */
		if (memcmp(p_cb->p_current_configuration, p_instance_cfg,
			   sizeof(*p_instance_cfg)) != 0) {
			nrfx_spim_uninit(p_bm_spi_mngr->p_spim);
			result = nrfx_spim_init(p_bm_spi_mngr->p_spim,
						p_instance_cfg,
						spim_event_handler,
						(void *)p_bm_spi_mngr);
			__ASSERT_NO_MSG(result == 0);
			p_cb->p_current_configuration = p_instance_cfg;
		}

		/* Try to start first transfer for this new transaction. */
		p_cb->current_transfer_idx = 0;

		/* Execute user code if available before starting transaction. */
		transaction_begin_signal(p_bm_spi_mngr);
		result = start_transfer(p_bm_spi_mngr);

		/* If transaction started successfully there is nothing more to do here now. */
		if (result == 0) {
			return;
		}

		/* Transfer failed to start. Notify the user that this transaction cannot be
		 * started and try with the next one in the next iteration of the loop.
		 */
		transaction_end_signal(p_bm_spi_mngr, result);

		switch_transaction = true;
	}
}

/* Handle SPIM events. Called from the SPIM interrupt when a transfer finishes. If there are more
 * transfers in the current transaction, start the next one. Otherwise, notify the user and start
 * the next queued transaction (if any).
 */
static void spim_event_handler(nrfx_spim_event_t const *p_event, void *p_context)
{
	__ASSERT_NO_MSG(p_event != NULL);
	__ASSERT_NO_MSG(p_context != NULL);

	int result;
	bm_spi_mngr_cb_t *p_cb = ((bm_spi_mngr_t const *)p_context)->p_bm_spi_mngr_cb;

	/* This callback should be called only during a transaction. */
	__ASSERT_NO_MSG(p_cb->p_current_transaction != NULL);

	if (p_event->type == NRFX_SPIM_EVENT_DONE) {
		result = 0;

		/* Transfer finished successfully. If there is another one to be performed in the
		 * current transaction, start it now. Use a local variable to avoid using two
		 * volatile variables in one expression.
		 */
		uint8_t curr_transfer_idx = p_cb->current_transfer_idx;

		++curr_transfer_idx;
		if (curr_transfer_idx < p_cb->p_current_transaction->number_of_transfers) {
			p_cb->current_transfer_idx = curr_transfer_idx;

			result = start_transfer(((bm_spi_mngr_t const *)p_context));

			if (result == 0) {
				/* The current transaction is running and its next transfer has
				 * been successfully started. There is nothing more to do.
				 */
				return;
			}
			/* If the next transfer could not be started due to some error, finish
			 * the transaction with this error code as the result.
			 */
		}
	} else {
		result = -EIO;
	}

	/* The current transaction has been completed or interrupted by some error. Notify the
	 * user and start the next one (if there is any). Switch transactions here so that
	 * p_current_transaction is set to NULL only if there is nothing more to do, in order to
	 * not generate a spurious idle status (even for a moment).
	 */
	transaction_end_signal(((bm_spi_mngr_t const *)p_context), result);
	start_pending_transaction(((bm_spi_mngr_t const *)p_context), true);
}

int bm_spi_mngr_init(bm_spi_mngr_t const *p_bm_spi_mngr,
		     nrfx_spim_config_t const *p_default_spim_config)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);
	__ASSERT_NO_MSG(p_bm_spi_mngr->p_queue != NULL);
	__ASSERT_NO_MSG(p_bm_spi_mngr->p_queue->size > 0);
	__ASSERT_NO_MSG(p_bm_spi_mngr->p_queue->p_byte_storage != NULL);
	__ASSERT_NO_MSG(p_default_spim_config != NULL);

	bm_spi_mngr_queue_t *p_queue = p_bm_spi_mngr->p_queue;

	/* Initialize the ring buffer that holds the queued transactions. Each slot stores one
	 * pointer to a transaction descriptor, so the total size in bytes is the number of queue
	 * slots multiplied by the size of one pointer.
	 */
	ring_buf_init(&p_queue->ring,
		      (uint32_t)(p_queue->size * sizeof(bm_spi_mngr_transaction_t const *)),
		      p_queue->p_byte_storage);

	int err_code = nrfx_spim_init(p_bm_spi_mngr->p_spim,
				      p_default_spim_config,
				      spim_event_handler,
				      (void *)p_bm_spi_mngr);

	if (err_code == 0) {
		bm_spi_mngr_cb_t *p_cb = p_bm_spi_mngr->p_bm_spi_mngr_cb;

		p_cb->p_current_transaction = NULL;
		p_cb->default_configuration = *p_default_spim_config;
		p_cb->p_current_configuration = &p_cb->default_configuration;
	}

	return err_code;
}

void bm_spi_mngr_uninit(bm_spi_mngr_t const *p_bm_spi_mngr)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);

	nrfx_spim_uninit(p_bm_spi_mngr->p_spim);

	ring_buf_reset(&p_bm_spi_mngr->p_queue->ring);
	p_bm_spi_mngr->p_bm_spi_mngr_cb->p_current_transaction = NULL;
}

int bm_spi_mngr_schedule(bm_spi_mngr_t const *p_bm_spi_mngr,
			 bm_spi_mngr_transaction_t const *p_transaction)
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);
	__ASSERT_NO_MSG(p_transaction != NULL);
	__ASSERT_NO_MSG(p_transaction->p_transfers != NULL);
	__ASSERT_NO_MSG(p_transaction->number_of_transfers != 0);

	int result = queue_push(p_bm_spi_mngr, &p_transaction);

	if (result == 0) {
		/* New transaction has been successfully added to queue,
		 * so if we are currently idle it's time to start the job.
		 */
		start_pending_transaction(p_bm_spi_mngr, false);
	}

	return result;
}

/* Internal end callback used by bm_spi_mngr_perform(). Stores the transaction result and clears
 * the in-progress flag, which releases the blocking caller from its wait loop.
 */
static void spi_internal_transaction_cb(int result, void *p_user_data)
{
	bm_spi_mngr_cb_data_t *p_cb_data = (bm_spi_mngr_cb_data_t *)p_user_data;

	p_cb_data->transaction_result = result;
	p_cb_data->transaction_in_progress = false;
}

int bm_spi_mngr_perform(bm_spi_mngr_t const *p_bm_spi_mngr,
			nrfx_spim_config_t const *p_config,
			bm_spi_mngr_transfer_t const *p_transfers, uint8_t number_of_transfers,
			void (*user_function)(void))
{
	__ASSERT_NO_MSG(p_bm_spi_mngr != NULL);
	__ASSERT_NO_MSG(p_transfers != NULL);
	__ASSERT_NO_MSG(number_of_transfers != 0);

	bm_spi_mngr_cb_data_t cb_data = {
		.transaction_in_progress = true,
	};

	/* Wrap the transfers in an internal transaction so the same scheduling path can be
	 * reused. The internal end callback signals completion back to this function.
	 */
	bm_spi_mngr_transaction_t internal_transaction = {
		.begin_callback = NULL,
		.end_callback = spi_internal_transaction_cb,
		.p_user_data = (void *)&cb_data,
		.p_transfers = p_transfers,
		.number_of_transfers = number_of_transfers,
		.p_required_spim_cfg = p_config,
	};

	int result = bm_spi_mngr_schedule(p_bm_spi_mngr, &internal_transaction);

	if (result != 0) {
		return result;
	}

	/* The user function may sleep the CPU waiting for an interrupt, so it must not be
	 * called from ISR context. This assert is a precaution against silent deadlocks. If
	 * the function were called from an ISR with a sleeping idle hook, the SPIM interrupt
	 * that ends the transaction could not run, and the system would lock up with no
	 * indication of what went wrong.
	 */
	__ASSERT_NO_MSG(user_function == NULL || !k_is_in_isr());

	/* Block until the internal end callback runs from the SPIM interrupt and clears the
	 * in-progress flag.
	 */
	while (cb_data.transaction_in_progress) {
		if (user_function) {
			user_function();
		}
	}

	return cb_data.transaction_result;
}
