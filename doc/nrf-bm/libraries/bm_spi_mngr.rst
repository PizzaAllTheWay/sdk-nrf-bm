.. _lib_bm_spi_mngr:

SPI transaction manager
#######################

.. contents::
   :local:
   :depth: 2

The SPI transaction manager library serializes SPI master (SPIM) work on one hardware instance.
Applications describe work as *transactions* (one or more TX/RX *transfers* in order), and the library keeps pending transactions in a FIFO queue and runs them back-to-back on the bus.

Overview
********

The library sits on top of the nrfx SPIM driver and uses a Zephyr ring buffer as a FIFO queue of transaction pointers.
The queue is accessed both from the application (when scheduling) and from the SPIM interrupt handler (when one transaction finishes and the next one is started), so internal queue operations run with interrupts locked.

The library exposes a non-blocking API and a blocking API on top of the same scheduling engine.
Return values follow the negative errno convention:

* :c:func:`bm_spi_mngr_init` and :c:func:`bm_spi_mngr_perform` return ``0`` on success, or a negative errno-style code from this module or from nrfx SPIM on failure.
* :c:func:`bm_spi_mngr_schedule` returns ``0`` on success or ``-ENOMEM`` if the queue is full.
* The end callback receives ``0`` on success, or a negative errno-style code (for example ``-EIO`` from this module, or a negative errno from nrfx SPIM) on failure.

The two APIs work as follows:

* :c:func:`bm_spi_mngr_schedule` enqueues a :c:struct:`bm_spi_mngr_transaction_t` and returns immediately.
  Optional begin and end callbacks on the descriptor run around the transaction (see `Callback context`_).

* :c:func:`bm_spi_mngr_perform` builds an internal transaction for a one-shot multi-transfer job, schedules it through the same path, and blocks until completion.
  While waiting, it may call an optional idle function (for example ``k_cpu_idle``).
  This idle function must not be called from ISR context, because the SPIM interrupt that ends the transaction must still run to release the wait loop.
  Calling it from an ISR would deadlock silently, and the library asserts on this case as a precaution.

Callback context
================

* ``end_callback`` always runs from the SPIM interrupt handler when the transaction completes or aborts.
* ``begin_callback`` runs just before the first transfer of the transaction is started on the bus.
  It runs from whichever context drives the start: the caller of :c:func:`bm_spi_mngr_schedule` if the bus was idle, or the SPIM interrupt handler if the previous transaction finished and chained into this one.

Treat both as if they could run from interrupt context.
Keep them short, for example setting a flag to indicate end of transaction.

Per-transaction configuration
=============================

Each transaction may supply its own :c:member:`bm_spi_mngr_transaction_t.p_required_spim_cfg` pointer.
If that member is NULL, the library uses the SPIM configuration passed to :c:func:`bm_spi_mngr_init`.
If it points at a different :c:struct:`nrfx_spim_config_t`, the driver is reinitialized when the configuration does not match the one already active.
This allows changing pins, clock, or mode between devices on the same bus, for example to drive a different software chip select line per peripheral.

A typical use is sharing one SPIM instance between multiple peripherals, where each peripheral has its own chip select pin.
Each peripheral defines its own :c:struct:`nrfx_spim_config_t` with the matching chip select, and transactions targeted at that peripheral point :c:member:`bm_spi_mngr_transaction_t.p_required_spim_cfg` at it.
The manager then reconfigures the SPIM instance on the fly as transactions for different peripherals are pulled from the queue.

Before calling :c:func:`bm_spi_mngr_init`, connect and enable the SPIM interrupt for the chosen instance (for example using :c:macro:`BM_IRQ_DIRECT_CONNECT`), as required by nrfx SPIM.

Configuration
*************

Enable the library with the :kconfig:option:`CONFIG_BM_SPI_MNGR` Kconfig option.

The option depends on :kconfig:option:`CONFIG_NRFX_SPIM` and selects :kconfig:option:`CONFIG_RING_BUFFER` for the internal FIFO.

Defining an instance
====================

Use the :c:macro:`BM_SPI_MNGR_DEF` macro once per logical bus manager.
It allocates the queue backing store, control block, :c:struct:`nrfx_spim_t` instance, and a const :c:struct:`bm_spi_mngr_t` handle.

The macro argument ``_queue_size`` is the number of *pending* transaction slots, not counting the transaction currently executing.
With queue depth ``N``, you can have at most ``N`` waiting transactions while one runs, so up to ``N + 1`` transactions may be in flight in total.

Initialization
==============

Call :c:func:`bm_spi_mngr_init` with the manager handle and the SPIM configuration the instance should use when a scheduled transaction leaves :c:member:`bm_spi_mngr_transaction_t.p_required_spim_cfg` as NULL.
The library copies that configuration into its control block and uses it as the default for any transaction that does not provide its own.

Call :c:func:`bm_spi_mngr_uninit` to shut down SPIM and reset the queue.
Do not call it while a transaction is in progress or while a caller is blocked in :c:func:`bm_spi_mngr_perform`.
The function does not cancel pending work or release blocked callers.

Scheduling transactions
=======================

Fill in a :c:struct:`bm_spi_mngr_transaction_t` with:

* :c:member:`bm_spi_mngr_transaction_t.p_transfers` -- array of :c:struct:`bm_spi_mngr_transfer_t` segments.
* :c:member:`bm_spi_mngr_transaction_t.number_of_transfers` -- length of that array.
* Optional :c:member:`bm_spi_mngr_transaction_t.begin_callback`, called just before the first transfer of the transaction is started on the bus. See `Callback context`_ for which context it runs in.
* Optional :c:member:`bm_spi_mngr_transaction_t.end_callback`, called from the SPIM interrupt handler when the transaction completes or aborts after an error. The first argument is ``0`` on success, or a negative errno-style code from nrfx or this module on failure.
* Optional :c:member:`bm_spi_mngr_transaction_t.p_user_data`, an opaque pointer passed through to both callbacks.
* Optional :c:member:`bm_spi_mngr_transaction_t.p_required_spim_cfg` -- per-transaction SPIM settings, or NULL to use the configuration from :c:func:`bm_spi_mngr_init`.

Use the :c:macro:`BM_SPI_MNGR_TRANSFER` macro to initialize simple transfer descriptors.

The transaction descriptor (and any non-NULL configuration pointer it carries) must stay valid until the transaction completes, because the library only stores a pointer to it in the queue.

Pass the descriptor to :c:func:`bm_spi_mngr_schedule`.
The function returns ``0`` on success and ``-ENOMEM`` if the queue is full.
On success, the transaction is started immediately if the bus is idle, otherwise it runs back-to-back after the transactions ahead of it.

Blocking helper
===============

:c:func:`bm_spi_mngr_perform` is a convenience wrapper around :c:func:`bm_spi_mngr_schedule` for a single synthetic transaction.
Its ``p_config`` argument maps to :c:member:`bm_spi_mngr_transaction_t.p_required_spim_cfg`, where NULL means use the configuration from :c:func:`bm_spi_mngr_init`.
This function does not expose begin or end callbacks. It simply returns when the transaction is finished.
The optional ``idle_fn`` argument is called repeatedly while the function waits for the transaction to finish, and must not be called from ISR context.

Idle detection
==============

:c:func:`bm_spi_mngr_is_idle` returns true when no transaction is in progress.
The library only clears the in-progress flag once the queue is empty, so this also implies no pending transactions and can be used as a reliable signal that all scheduled work has finished.

Dependencies
************

* nrfx SPIM (:kconfig:option:`CONFIG_NRFX_SPIM`)
* Zephyr ring buffer (:kconfig:option:`CONFIG_RING_BUFFER`), pulled in automatically when the library is enabled
* Zephyr kernel symbols for the ISR assertion in :c:func:`bm_spi_mngr_perform` (``k_is_in_isr``)

API documentation
*****************

| Header file: :file:`include/bm/bm_spi_mngr.h`
| Source files: :file:`lib/bm_spi_mngr/`

:ref:`SPI transaction manager API reference <api_bm_spi_mngr>`
