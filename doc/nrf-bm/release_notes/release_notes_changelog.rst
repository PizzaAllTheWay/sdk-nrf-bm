:orphan:

.. _nrf_bm_release_notes_2099:

Changelog for |BMlong| v2.0.99
##############################

This changelog reflects the most relevant changes from the latest official release.

Changelog
*********

The following sections provide detailed lists of changes by component.

SDK installation
================

No changes since the latest nRF Connect SDK Bare Metal release.

S115 SoftDevice
===============

No changes since the latest nRF Connect SDK Bare Metal release.

S145 SoftDevice
===============

No changes since the latest nRF Connect SDK Bare Metal release.

SoftDevice Handler
==================

No changes since the latest nRF Connect SDK Bare Metal release.

Boards
======

No changes since the latest nRF Connect SDK Bare Metal release.

Build system
============

No changes since the latest nRF Connect SDK Bare Metal release.

Interrupts
==========

No changes since the latest nRF Connect SDK Bare Metal release.

Logging
=======

No changes since the latest nRF Connect SDK Bare Metal release.

Drivers
=======

No changes since the latest nRF Connect SDK Bare Metal release.

Subsystems
==========

Storage
-------

No changes since the latest nRF Connect SDK Bare Metal release.

Filesystem
----------

No changes since the latest nRF Connect SDK Bare Metal release.

Libraries
=========


* :ref:`lib_ble_adv` library:

   * Added the :c:func:`ble_adv_data_manufacturer_data_find` function to locate manufacturer-specific data in an advertising payload and prefix-match it against a target value.

* :ref:`lib_ble_scan` library:

   * Added:

      * The :c:struct:`ble_scan_filter_data` structure as input to the :c:func:`ble_scan_filter_add` function.
      * Support for filtering by manufacturer-specific data using the :c:macro:`BLE_SCAN_MANUFACTURER_DATA_FILTER` filter type.
      * The :kconfig:option:`CONFIG_BLE_SCAN_MANUFACTURER_DATA_COUNT` and :kconfig:option:`CONFIG_BLE_SCAN_MANUFACTURER_DATA_MAX_LEN` Kconfig options to configure the manufacturer data filter capacity and maximum payload length.

Bluetooth LE Services
---------------------

No changes since the latest nRF Connect SDK Bare Metal release.

Libraries for NFC
-----------------

No changes since the latest nRF Connect SDK Bare Metal release.

Utils
-----

No changes since the latest nRF Connect SDK Bare Metal release.

Samples
=======

No changes since the latest nRF Connect SDK Bare Metal release.

Bluetooth LE samples
--------------------

No changes since the latest nRF Connect SDK Bare Metal release.

NFC samples
-----------

No changes since the latest nRF Connect SDK Bare Metal release.

Peripheral samples
------------------

No changes since the latest nRF Connect SDK Bare Metal release.

DFU samples
-----------

No changes since the latest nRF Connect SDK Bare Metal release.

Subsystem samples
-----------------

No changes since the latest nRF Connect SDK Bare Metal release.

Known issues and limitations
============================

No changes since the latest nRF Connect SDK Bare Metal release.

Documentation
=============

No changes since the latest nRF Connect SDK Bare Metal release.
