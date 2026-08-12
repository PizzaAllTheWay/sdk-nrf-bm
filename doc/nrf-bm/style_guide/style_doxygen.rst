.. _bm_style_doxygen:

Doxygen in public headers
#########################

.. contents::
   :local:
   :depth: 2

Doxygen comments in the public headers are the source of the |BMshort| API documentation.

What to document and how to phrase it is governed by the `nRF Connect SDK Doxygen guidelines`_, which build on the `Zephyr Doxygen style guidelines`_.
Any command in the `Doxygen commands`_ reference is usable, whether or not these guidelines mention it.

Doxygen conventions differ for file headers, functions, structs, enums, and typedefs.
The upstream page for each is listed below, any |BMshort| deviation is described in the matching section further down, and a construct with no deviation is not covered again beyond this link.

* `nRF Connect SDK Doxygen guidelines for file headers and groups`_
* `nRF Connect SDK Doxygen guidelines for functions`_
* `nRF Connect SDK Doxygen guidelines for structs`_
* `nRF Connect SDK Doxygen guidelines for enums`_
* `nRF Connect SDK Doxygen guidelines for typedefs`_

Doxygen renders only the public headers under :file:`include/bm/` and the :file:`.dox` page sources, set by ``INPUT`` and ``FILE_PATTERNS`` in :file:`doc/nrf-bm/nrf-bm.doxyfile.in`.
A Doxygen comment anywhere else is not rendered, so headers under :file:`lib/`, :file:`subsys/`, and :file:`drivers/` use plain ``/* */`` comments instead.
Adding a new public header directory requires adding it to ``INPUT``, and a header's group must be included from an RST page to appear in the rendered output (see `nRF Connect SDK guidelines for including doxygen in RST`_).

Rules that apply to every documented item
*****************************************

Use the multi-line form by default, at least three lines, with the opening ``/**`` and closing ``*/`` each on their own line.

.. code-block:: c

   /**
    * @brief Initialize a timer instance.
    */

Do not compress this for a function, macro, typedef, enum, struct, or union documented as a whole, the constant shape keeps API blocks easy to scan and keeps the diff small when a description grows later.

File headers and groups
***********************

Follow `nRF Connect SDK Doxygen guidelines for file headers and groups`_.
|BMshort| adds one convention the upstream guidelines leave open, prefix the group name with ``bm_``, matching the include path.

.. code-block:: c

   /**
    * @defgroup bm_timer NCS Bare Metal Timer library
    * @{
    */

Functions
*********

Follow `nRF Connect SDK Doxygen guidelines for functions`_ for ``@param``, ``@return``, and ``@retval``.
|BMshort| adds two conventions for cases the upstream guidelines do not cover:

* State the tick size for a Bluetooth timing parameter given in spec-defined ticks rather than milliseconds, since the C type gives no hint of the unit:

  .. code-block:: c

     /**
      * @brief Minimum connection interval.
      *
      * In units of 1.25 ms, range 6 to 3200 (7.5 ms to 4 s).
      */
     uint16_t min_conn_interval;

* State once that an underlying library's error code (for example, SoftDevice) is forwarded unchanged, instead of listing every value it can produce:

  .. code-block:: c

     /**
      * @brief Stop advertising.
      *
      * @param[in] ble_adv Bluetooth LE advertising instance.
      *
      * @retval NRF_SUCCESS On success.
      * @retval NRF_ERROR_INVALID_STATE Library is not initialized or not advertising.
      * @retval NRF_ERROR_NULL @p ble_adv is @c NULL.
      * @return Any error from @c sd_ble_gap_adv_stop on failure.
      */
     uint32_t ble_adv_stop(struct ble_adv *ble_adv);
