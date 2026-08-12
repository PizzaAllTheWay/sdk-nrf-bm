.. _bm_style_devicetree:

Devicetree
##########

.. contents::
   :local:
   :depth: 2

Formatting, node and property naming, and property layout follow the `Zephyr devicetree style guidelines`_.
Binding files follow the `Zephyr devicetree bindings syntax`_ and the `Zephyr rules for upstream devicetree bindings`_.

Comments
********

Most nodes need no comment, because the node name and its properties already say what they are.

The exception is the partition layout.
A partition offset or size is usually fixed by something outside the file, such as the size of another firmware component, a requirement from the bootloader, or the flash device's erase-block size.
None of that is visible in the number itself.
Record where the number itself comes from:

.. code-block:: devicetree

   /* Placed at the end of SRAM, but adjusted to 1kB boundary
    * (leaving 512 bytes as unused at the end)
    */
   clipboard_partition: sram@2003fb00 {
   	compatible = "zephyr,memory-region", "mmio-sram";
   	reg = <0x2003fb00 0x100>;
   	zephyr,memory-region = "RetainedMem";
   	status = "okay";
   };
