.. _bm_spi_sample:

SPI
###

.. contents::
   :local:
   :depth: 2

The SPI sample demonstrates how to configure and use the SPIM and SPIS peripherals with the nrfx drivers.

Requirements
************

The sample supports the following development kits:

.. tabs::

   .. group-tab:: Simple board variants

      The following board variants do **not** have DFU capabilities:

      .. include:: /includes/supported_boards_all_non-mcuboot_variants_s115.txt

      .. include:: /includes/supported_boards_all_non-mcuboot_variants_s145.txt

   .. group-tab:: MCUboot board variants

      The following board variants have DFU capabilities:

      .. include:: /includes/supported_boards_all_mcuboot_variants_s115.txt

      .. include:: /includes/supported_boards_all_mcuboot_variants_s145.txt

Overview
********

The sample initializes the **SPIM** and **SPIS** instances with the pins configured in the board's :file:`board-config.h`.
When **Button 2** is pressed, the master sends a configurable string (:kconfig:option:`CONFIG_SAMPLE_SPI_MSG`) to the slave.
The slave receives the data, logs it, and toggles **LED 2**.

User interface
**************

LED 0
  Lit when the sample is initialized and ready.

LED 2
  Toggles each time the SPI slave completes a reception.

Button 2
  Sends a message from the local SPIM to the connected SPIS.

.. _bm_spi_wiring:

Wiring
======

The sample uses the **SPIM** and **SPIS** pins defined in the board's :file:`board-config.h` header (``BOARD_APP_SPIM_*`` and ``BOARD_APP_SPIS_*``).

.. include:: /includes/spi_board_connections.txt

Connect the four SPIM pins to the four SPIS pins, matching each signal by name (SCK→SCK, MOSI→MOSI, MISO→MISO, CSN→CSN). This can be done in two ways:

#. Single board (loopback):

   - Connect the **SPIM** pins to the **SPIS** pins on the same board.

#. Two boards:

   - Connect Device 1 **SPIM SCK/MOSI/MISO/CSN** to Device 2 **SPIS SCK/MOSI/MISO/CSN**.
   - Connect **GND** between both devices.

.. note:: Board-specific behavior

   * **nRF54L15 DK** — SPI Master pins overlap with **LED 3** (P1.14, flickers during transfers) and **Button 0** (P1.13, do not press during transfers).
   * **nRF54LV10 DK** — SPI Master and SPI Slave share the same hardware instance, so they cannot run simultaneously and single-board loopback is not possible by default. Use the sample's Kconfig role switch to pick master-only or slave-only and test with two DKs. To run both at once, move one of the SPI instances to ``NRF_SPIS30`` on P0.00-P0.03 in :file:`board-config.h`; this requires physical board modifications since those pins are not exposed on the expansion headers.

Building and running
********************

This sample can be found under :file:`samples/peripherals/spi/` in the |BMshort| folder structure.

For details on how to create, configure, and program a sample, see :ref:`getting_started_with_the_samples`.

Testing
=======

Before testing, select one of the following SPI roles with Kconfig to match your wiring and setup:

* ``SAMPLE_SPI_ROLE_BOTH`` (default) — both SPIM and SPIS active on the same device (required for single-board loopback and for two-device, two-bus setups).
* ``SAMPLE_SPI_ROLE_MASTER`` — only SPIM active (sending device in a two-device, one-bus setup).
* ``SAMPLE_SPI_ROLE_SLAVE`` — only SPIS active (receiving device in a two-device, one-bus setup).

The sample can be tested in three ways, depending on the selected role and wiring:

.. tabs::

   .. group-tab:: Single device (loopback)

      Test the sample with both SPIM and SPIS active on the same board.

      1. Flash the sample with :kconfig:option:`CONFIG_SAMPLE_SPI_ROLE_BOTH`.
      #. Connect the four SPIM→SPIS signal pairs on the same board (see :ref:`Wiring <bm_spi_wiring>`).
      #. Open console log and verify that ``SPI sample initialized`` appears in the log.
      #. Press **Button 2**.
         The log shows a master "sent" entry and a slave "received" entry.
         **LED 2** toggles on each completed reception.

      Observe the :kconfig:option:`CONFIG_SAMPLE_SPI_MSG` string in both the master completion and slave reception log entries.

   .. group-tab:: Two devices, one data path

      Test the sample with one device as master and another as slave.

      1. Flash Device 1 with :kconfig:option:`CONFIG_SAMPLE_SPI_ROLE_MASTER` and Device 2 with :kconfig:option:`CONFIG_SAMPLE_SPI_ROLE_SLAVE`.
      #. Wire the four SPIM→SPIS signals **and GND** between the devices (see :ref:`Wiring <bm_spi_wiring>`).
      #. Open a console for each device.
      #. Press **Button 2** on Device 1 (the SPIM side).
         Device 1 logs "sent" and Device 2 logs "received".
         **LED 2** on Device 2 toggles.

      Observe the :kconfig:option:`CONFIG_SAMPLE_SPI_MSG` string in both the master completion and slave reception log entries.

   .. group-tab:: Two devices, two data paths

      Test the sample with both devices running as master and slave simultaneously.

      1. Flash both devices with :kconfig:option:`CONFIG_SAMPLE_SPI_ROLE_BOTH`.
      #. Wire both sets of four SPIM→SPIS signals (8 wires) **and GND** (see :ref:`Wiring <bm_spi_wiring>`).
      #. Open a console for each device.
      #. Press **Button 2** on either device to send to the other.
         The sending device logs "sent", the receiving device logs "received",
         and **LED 2** toggles on the receiver.

      Observe the :kconfig:option:`CONFIG_SAMPLE_SPI_MSG` string in both the master completion and slave reception log entries.
