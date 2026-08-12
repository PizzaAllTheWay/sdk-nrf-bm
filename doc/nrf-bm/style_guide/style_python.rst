.. _bm_style_python:

Python
######

.. contents::
   :local:
   :depth: 2

Python in this repository is tooling rather than product code.
It covers the documentation build under :file:`doc/`, the metadata generators under :file:`scripts/`, the pytest plugins under :file:`scripts/pytest_plugins/`, and the hardware test suites under :file:`tests/`.

Formatting and linting follow the `Zephyr Python style guidelines`_, which specify `Ruff`_ and the options applied on top of its defaults.
Docstring conventions follow `PEP 257`_.

Ruff configuration
******************

The configuration is :file:`.ruff.toml` in the repository root, and that file is the authority on what is checked.

The formatter applies to every Python file in the repository, with no exclusion list.

Where you silence a rule, give the reason on the same line:

.. code-block:: python

   import redirects  # noqa: E402

Docstrings
**********

Write a module docstring for every script that is run directly.
State what the script produces and what invokes it:

.. code-block:: python

   """Flash a firmware image to the device and verify the CRC.

   Usage:
       python flash_and_verify.py --image path/to/firmware.hex --port /dev/ttyACM0
   """

Write a function docstring when a caller cannot tell what the function does just by looking at how it is called.

.. code-block:: python

   def no_reset(device_object: DeviceAdapter):
       """Do not reset after flashing."""

Skip the docstring for a short helper called directly, by name, in the same file.

Comments
********

Comment the parts that depend on something outside the script, because those are the parts a reader cannot verify from the code:

* An assumption about a file or directory layout the script depends on.
* A workaround for the behavior of an external tool.
* A magic number, such as a timeout, retry count, or buffer size, with the reason it was chosen.

.. code-block:: python

   # using nrfutil to upload an image due to mcumgr hangs on upload too large image
   ret = nrfutil.image_upload(serial_port, signed_output_zephyr, check=False)
