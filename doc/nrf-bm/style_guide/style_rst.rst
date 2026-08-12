.. _bm_style_rst:

RST documentation
#################

.. contents::
   :local:
   :depth: 2

Conceptual documentation is written in RST and built with Sphinx.
Most of the documentation source lives in :file:`doc/nrf-bm/`.
Each sample's own :file:`README.rst` is also built into the documentation.

The rules are the `nRF Connect SDK RST guidelines`_, which build on the `Zephyr documentation guidelines`_.
They cover headings, tables of contents, linking, and substitutions.
Prose style, grammar, and punctuation follow the `Nordic TechDocs Style Guide`_.
The documentation is built with `Sphinx`_.

Reference labels
****************

Add a reference label above every page title, prefixed with ``bm_``:

.. code-block:: rst

   .. _bm_style_rst:

   RST documentation
   #################

One sentence per line
*********************

Write one sentence per line, and never wrap a sentence across lines.

Substitutions
*************

Use the substitutions defined in :file:`doc/nrf-bm/shortcuts.txt` instead of writing the names out.
For example, ``|BMshort|``, ``|NCS|``, or ``|release|``.

Linking outside this documentation set
**************************************

The |BMshort| documentation set has no intersphinx mapping to the |NCS| or to Zephyr.
``:ref:`` and ``:kconfig:option:`` only resolve for a target or symbol defined in this repository, so the syntax used to cross-reference another documentation set does not work here:

.. code-block:: rst

   # Unsupported: cross-set references do not resolve.
   :ref:`zephyr:coding_style`
   :ref:`nrf:ug_bootloader`
   :kconfig:option:`CONFIG_BT`

   # Supported: a target defined in this repository.
   :ref:`bm_style_rst`
   :kconfig:option:`CONFIG_BM_ZMS`

Link to |NCS|, Zephyr, nrfxlib, or any other external webpage as an external link, defined once in :file:`doc/nrf-bm/links.txt` and referenced by its link text:

.. code-block:: rst

   .. In links.txt:

   .. _`Zephyr coding style guidelines`: https://nrfconnectdocs.nordicsemi.com/ncs/latest/zephyr/contribute/style/index.html

   .. In the page:

   Formatting follows the `Zephyr coding style guidelines`_.

Use the latest version in the URL, so the link does not go stale at the next release, and pin a version only where the point being made is about that version.

Reused text
*************

Text that appears on several pages goes in :file:`doc/nrf-bm/includes/` as a shared snippet text file.
Use the include wherever the same text occurs in multiple places:

.. code-block:: rst

   .. include:: /includes/supported_boards_all_mcuboot_variants_s115.txt
