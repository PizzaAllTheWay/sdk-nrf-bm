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

      .. list-table::
         :header-rows: 1

         * - Hardware platform
           - PCA
           - SoftDevice
           - Board target
         * - `nRF54L15 DK`_
           - PCA10156
           - S115
           - bm_nrf54l15dk/nrf54l15/cpuapp/s115_softdevice
         * - `nRF54L15 DK`_ (emulating nRF54L10)
           - PCA10156
           - S115
           - bm_nrf54l15dk/nrf54l10/cpuapp/s115_softdevice
         * - `nRF54L15 DK`_ (emulating nRF54L05)
           - PCA10156
           - S115
           - bm_nrf54l15dk/nrf54l05/cpuapp/s115_softdevice
         * - `nRF54LM20 DK`_
           - PCA10184
           - S115
           - bm_nrf54lm20dk/nrf54lm20a/cpuapp/s115_softdevice
         * - nRF54LS05 DK
           - PCA10214
           - S115
           - bm_nrf54ls05dk/nrf54ls05b/cpuapp/s115_softdevice
         * - `nRF54LV10 DK`_
           - PCA10188
           - S115
           - bm_nrf54lv10dk/nrf54lv10a/cpuapp/s115_softdevice
         * - `nRF54L15 DK`_
           - PCA10156
           - S145
           - bm_nrf54l15dk/nrf54l15/cpuapp/s145_softdevice
         * - `nRF54L15 DK`_ (emulating nRF54L10)
           - PCA10156
           - S145
           - bm_nrf54l15dk/nrf54l10/cpuapp/s145_softdevice
         * - `nRF54L15 DK`_ (emulating nRF54L05)
           - PCA10156
           - S145
           - bm_nrf54l15dk/nrf54l05/cpuapp/s145_softdevice
         * - `nRF54LM20 DK`_
           - PCA10184
           - S145
           - bm_nrf54lm20dk/nrf54lm20a/cpuapp/s145_softdevice
         * - nRF54LS05 DK
           - PCA10214
           - S145
           - bm_nrf54ls05dk/nrf54ls05b/cpuapp/s145_softdevice
         * - `nRF54LV10 DK`_
           - PCA10188
           - S145
           - bm_nrf54lv10dk/nrf54lv10a/cpuapp/s145_softdevice

   .. group-tab:: MCUboot board variants

      The following board variants have DFU capabilities:

      .. list-table::
         :header-rows: 1

         * - Hardware platform
           - PCA
           - SoftDevice
           - Board target
         * - `nRF54L15 DK`_
           - PCA10156
           - S115
           - bm_nrf54l15dk/nrf54l15/cpuapp/s115_softdevice/mcuboot
         * - `nRF54L15 DK`_ (emulating nRF54L10)
           - PCA10156
           - S115
           - bm_nrf54l15dk/nrf54l10/cpuapp/s115_softdevice/mcuboot
         * - `nRF54L15 DK`_ (emulating nRF54L05)
           - PCA10156
           - S115
           - bm_nrf54l15dk/nrf54l05/cpuapp/s115_softdevice/mcuboot
         * - `nRF54LM20 DK`_
           - PCA10184
           - S115
           - bm_nrf54lm20dk/nrf54lm20a/cpuapp/s115_softdevice/mcuboot
         * - nRF54LS05 DK
           - PCA10214
           - S115
           - bm_nrf54ls05dk/nrf54ls05b/cpuapp/s115_softdevice/mcuboot
         * - `nRF54LV10 DK`_
           - PCA10188
           - S115
           - bm_nrf54lv10dk/nrf54lv10a/cpuapp/s115_softdevice/mcuboot
         * - `nRF54L15 DK`_
           - PCA10156
           - S145
           - bm_nrf54l15dk/nrf54l15/cpuapp/s145_softdevice/mcuboot
         * - `nRF54L15 DK`_ (emulating nRF54L10)
           - PCA10156
           - S145
           - bm_nrf54l15dk/nrf54l10/cpuapp/s145_softdevice/mcuboot
         * - `nRF54L15 DK`_ (emulating nRF54L05)
           - PCA10156
           - S145
           - bm_nrf54l15dk/nrf54l05/cpuapp/s145_softdevice/mcuboot
         * - `nRF54LM20 DK`_
           - PCA10184
           - S145
           - bm_nrf54lm20dk/nrf54lm20a/cpuapp/s145_softdevice/mcuboot
         * - nRF54LS05 DK
           - PCA10214
           - S145
           - bm_nrf54ls05dk/nrf54ls05b/cpuapp/s145_softdevice/mcuboot
         * - `nRF54LV10 DK`_
           - PCA10188
           - S145
           - bm_nrf54lv10dk/nrf54lv10a/cpuapp/s145_softdevice/mcuboot

.. _bm_spi_wiring:

Wiring
======

The sample uses the **SPIM** and **SPIS** pins defined in the board's :file:`board-config.h` header (``BOARD_APP_SPIM_*`` and ``BOARD_APP_SPIS_*``).

.. include:: /includes/spi_board_connections_nrf54l15.txt

Connect the pins in one of the following configurations:

.. tabs::

   .. tab:: Single device (loopback)

      Connect the four SPIM pins to the four SPIS pins **on the same board**, matching each signal by name (SCK→SCK, MOSI→MOSI, MISO→MISO, CSN→CSN).

   .. tab:: Two devices, one data path

      Wire **Device 1 SPIM** to **Device 2 SPIS** — four signal wires plus GND:

      =================  =================
      Device 1 (SPIM)    Device 2 (SPIS)
      =================  =================
      SCK                SCK
      MOSI               MOSI
      MISO               MISO
      CSN                CSN
      **GND**            **GND**
      =================  =================

      Only Device 1 can send. Press **Button 2** on Device 1 to trigger a transfer.

   .. tab:: Two devices, two data paths

      Wire **both** buses so each device is master toward the other's slave, eight signal wires plus GND:

      .. list-table:: Bus A — Device 1 sends to Device 2
         :widths: auto
         :header-rows: 1

         * - Device 1 (SPIM)
           - Device 2 (SPIS)
         * - SCK
           - SCK
         * - MOSI
           - MOSI
         * - MISO
           - MISO
         * - CSN
           - CSN

      .. list-table:: Bus B — Device 2 sends to Device 1
         :widths: auto
         :header-rows: 1

         * - Device 2 (SPIM)
           - Device 1 (SPIS)
         * - SCK
           - SCK
         * - MOSI
           - MOSI
         * - MISO
           - MISO
         * - CSN
           - CSN

      Connect **GND** between the two devices.

      Press **Button 2** on either device to send to the other.

.. note:: LED 3 and SPIM CSN (nRF54L15)

   On the `nRF54L15 DK`_, P1.14 is shared between **SPIM CSN** and **LED 3**.
   The peripheral drives chip select, so the LED is not independent and flickers during transfers.

.. note:: Button 0 and SPIM MISO (nRF54L15)

   On the `nRF54L15 DK`_, P1.13 is shared between **SPIM MISO** and **Button 0**.
   Do not press Button 0 while the SPI bus is active.

.. note:: UARTE pin conflict (nRF54L15)

   On the `nRF54L15 DK`_, **SPIS** pins P0.00-P0.03 overlap with the default **Application UARTE** in :file:`board-config.h`.
   The :ref:`uarte_sample` and this sample cannot use those pins at the same time.
   Reassign one peripheral in the board file if both are needed.

.. note:: LPUARTE pin conflict (nRF54L15)

   On the `nRF54L15 DK`_, **LPUARTE TX, RX, and RDY** share P1.11, P1.12, and P1.14 with **SPIM SCK, MOSI, and CSN**. **LPUARTE REQ** (P0.04) overlaps with **Button 3**.
   The :ref:`bm_lpuarte_sample` and this sample cannot run together without reassigning pins in the board file.

Optionally, connect a logic analyzer or oscilloscope to the SPI lines.
Use a short GND lead to the dev kit ground and a sample rate high enough to resolve the 4 MHz default SPI clock.

Overview
********

The sample initializes the **SPIM** and **SPIS** instances from the board's :file:`board-config.h`.
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

Building and running
********************

This sample is in :file:`samples/peripherals/spi/` in the |BMshort| folder structure.

For general instructions, see :ref:`getting_started_with_the_samples`.

Testing
=======

After building and flashing, verify the wiring configuration you chose:

.. rubric:: Single device (loopback)

#. Flash the sample and open a console log.
#. Connect the four SPIM-SPIS signal pairs on the same board (see :ref:`bm_spi_wiring`).
#. Verify ``SPI sample initialized`` appears in the log.
#. Press **Button 2**.
   The log shows a master "sent" entry and a slave "received" entry.
   **LED 2** toggles on each completed reception.

.. rubric:: Two devices, one data path

#. Flash both devices with the same image.
#. Wire the four SPIM→SPIS signals **and GND** between the devices (see :ref:`bm_spi_wiring`).
#. Open a console for each device.
#. Press **Button 2** on Device 1 (the SPIM side).
   Device 1 logs "sent" and Device 2 logs "received".
   **LED 2** on Device 2 toggles.

.. rubric:: Two devices, two data paths

#. Flash both devices with the same image.
#. Wire both sets of four SPIM→SPIS signals (8 wires) **and GND** (see :ref:`bm_spi_wiring`).
#. Open a console for each device.
#. Press **Button 2** on either device to send to the other.
   The sending device logs "sent", the receiving device logs "received",
   and **LED 2** toggles on the receiver.

A successful test shows the :kconfig:option:`CONFIG_SAMPLE_SPI_MSG` string in both the master completion and slave reception log entries.
