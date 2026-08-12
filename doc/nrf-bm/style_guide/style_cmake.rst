.. _bm_style_cmake:

CMake
#####

.. contents::
   :local:
   :depth: 2

Formatting, command names, argument layout, variable naming, and wrapping strings and variables in quotes follow the `Zephyr CMake style guidelines`_.

What to comment
***************

A sample or an application build file is usually short enough to need no comments at all.
Do not annotate a command whose purpose is already clear from its name and arguments.

Comment the cases where the build does something a reader would not predict from the commands alone:

* An ordering dependency between targets that CMake does not express itself.
* A generated file, custom command, or post-build step, where the comment says what is produced and what consumes it.
* A workaround for a toolchain or build system limitation, including why it is still needed.

Conditional blocks
******************

A short block does not need a closing comment:

.. code-block:: cmake

   if(CONFIG_BM_ZMS)
     add_subdirectory(zms)
   endif()

Where a block is long, name the symbol in the closing statement:

.. code-block:: cmake

   if(CONFIG_BM_NFC_NDEF_MSG)
     zephyr_library()
     zephyr_library_sources(msg.c)

     # Long block of code

     zephyr_library_sources_ifdef(CONFIG_BM_NFC_NDEF_LAUNCHAPP_MSG launchapp_msg.c)
     zephyr_library_sources_ifdef(CONFIG_BM_NFC_NDEF_LAUNCHAPP_REC launchapp_rec.c)
   endif() # CONFIG_BM_NFC_NDEF_MSG
