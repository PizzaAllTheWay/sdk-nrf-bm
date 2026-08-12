.. _bm_style_guide:

Style guide
###########

This guide defines how to write comments and documentation in the |BMshort| repository.

|BMshort| is a downstream option of the |NCS|, so it builds on the |NCS| and Zephyr style rules rather than replacing them.
Each page names the upstream page that governs its file type, links to it, and then lists only what |BMshort| adds, narrows, or decides differently.

Absence of a rule on these pages means the upstream rule applies unchanged.
When in doubt, follow the link for the file type you are editing.
If that still leaves a question open, follow the |NCS| ruling and proceed from there.

Order of precedence
*******************

Check this guide first.
If it does not cover the rule you need, move down to the |NCS| guidelines, then to the Zephyr guidelines.
Where two of these disagree, the higher one in this list takes precedence:

1. This guide.
#. The `nRF Connect SDK documentation guidelines`_.
#. The `Zephyr coding style guidelines`_.

File type reference
*******************

.. list-table::
   :header-rows: 1
   :widths: 18 18 27 37

   * - File type
     - Guide page
     - Location
     - Upstream authority
   * - Public C header
     - :doc:`style_doxygen`
     - :file:`include/bm/**`
     - `nRF Connect SDK Doxygen guidelines`_ and `Zephyr Doxygen style guidelines`_
   * - C source and private header
     - :doc:`style_c_source`
     - :file:`lib/`, :file:`subsys/`, :file:`drivers/`, :file:`samples/`
     - `Zephyr C code style guidelines`_ and `Zephyr naming conventions`_
   * - Kconfig
     - :doc:`style_kconfig`
     - :file:`Kconfig*` throughout the tree
     - `nRF Connect SDK Kconfig guidelines`_ and `Zephyr Kconfig style guidelines`_
   * - Devicetree
     - :doc:`style_devicetree`
     - :file:`boards/`, overlays
     - `Zephyr devicetree style guidelines`_
   * - CMake
     - :doc:`style_cmake`
     - :file:`CMakeLists.txt` and :file:`*.cmake`
     - `Zephyr CMake style guidelines`_
   * - Python
     - :doc:`style_python`
     - :file:`scripts/`, :file:`doc/`, pytest suites
     - `Zephyr Python style guidelines`_
   * - RST
     - :doc:`style_rst`
     - :file:`doc/nrf-bm/` and sample :file:`README.rst` files
     - `nRF Connect SDK RST guidelines`_ and `Zephyr documentation guidelines`_

Rules that apply to every file
******************************

Licensing
=========

Start every source, build, and configuration file with a copyright notice and an SPDX identifier, in the comment syntax of that file type.

Use ``LicenseRef-Nordic-5-Clause`` as the |BMshort| identifier, and write the entity as ``Nordic Semiconductor ASA``.

Files derived from an Apache-2.0 upstream keep the upstream identifier instead, without adding a Nordic copyright line.

Indentation and line length
===========================

Keep lines under 100 characters.

RST is the exception, each sentence stays on its own line and is never wrapped.

.. toctree::
   :hidden:

   style_doxygen.rst
   style_c_source.rst
   style_kconfig.rst
   style_devicetree.rst
   style_cmake.rst
   style_python.rst
   style_rst.rst
