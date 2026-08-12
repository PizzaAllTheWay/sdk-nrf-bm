.. _bm_style_kconfig:

Kconfig
#######

.. contents::
   :local:
   :depth: 2

Kconfig prompts and help text are documentation.
They are rendered into the Kconfig reference of this documentation set, and they are what a user reads in the configuration interface.

The rules are the `nRF Connect SDK Kconfig guidelines`_, which extend the `Zephyr Kconfig style guidelines`_.
Between them they cover formatting, symbol naming, menu organisation, prompt wording, help text, and the required copyright header.
The per-type patterns for ``bool``, ``int``, ``hex``, and ``string`` symbols are in `nRF Connect SDK Kconfig symbol-specific patterns`_, and dependency structure is covered by `Zephyr Kconfig tips and best practices`_.

Timing units in prompts
***********************

Some |BMshort| options set a timing value as a count of a spec-defined tick rather than milliseconds.
State the tick size in parentheses:

.. code-block:: kconfig

   config BLE_ADV_DIRECTED_ADVERTISING_INTERVAL
   	int "Directed advertising interval (0.625 ms units)"
   	depends on BLE_ADV_DIRECTED_ADVERTISING
   	range 32 16384
   	default 32
  	help
   	  The minimum is 32, or 20 ms,
   	  the lowest value the SoftDevice accepts.
   	  The maximum is 16384, or 10.24 s,
   	  the Bluetooth Core Specification limit for directed advertising.

When help text is required
**************************

Write help text for a |BMshort| option when it does any of the following:

* Changes the memory layout or the size of the image.
* Interacts with an external library, a bootloader, or a peripheral that the application also drives.
* Has limits that come from a specification rather than from the code.

In those cases, state where the limit comes from and what happens at each end of the range.

Symbol naming
*************

Prefix a symbol with the name of the library or module it belongs to, rather than a single prefix shared by the whole repository.

* A Bluetooth library uses ``BLE_``, for example ``BLE_ADV_DIRECTED_ADVERTISING`` for :file:`lib/bluetooth/ble_adv/Kconfig`.
* A non-Bluetooth |BMshort| library uses ``BM_``, for example ``BM_GPIOTE_IRQ_PRIO`` for :file:`lib/bm_gpiote/Kconfig`.
* Never use the ``NCS_`` prefix, the |NCS| reserves it for options in the repositories it forks, and this repository is not one of them.

Logging symbols
***************

Use the standard logging template from the `nRF Connect SDK Kconfig guidelines`_ for every component.
Do not define a custom equivalent by hand.
The |NCS| guidelines apply this to subsystems and libraries.
|BMshort| applies it to drivers as well.
