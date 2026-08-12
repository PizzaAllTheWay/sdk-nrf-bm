.. _bm_style_c_source:

Comments in C source files
##########################

.. contents::
   :local:
   :depth: 2

This page covers comments in C source files and in private headers.
A private header is any :file:`.h` file outside :file:`include/bm/`, such as those under :file:`lib/`, :file:`subsys/`, and :file:`drivers/`.

Comment syntax, formatting, and the surrounding code follow the `Zephyr C code style guidelines`_, identifiers follow the `Zephyr naming conventions`_.
Neither is repeated here.

No Doxygen outside public headers
*********************************

Do not use Doxygen comments (``/** */``) or Doxygen commands (``@brief``, ``@param``, ``@retval``, and so on) in C source files or private headers, write plain ``/* */`` comments instead.

The documentation build reads only the public headers under :file:`include/bm/`.
A Doxygen block anywhere else is never rendered, so it promises a reference page that does not exist, and drifts out of date with no build warning to catch it.

Some private headers and source files still carry Doxygen blocks left over from before this page applied.
Convert a leftover block to a plain comment when editing the code it documents, rather than in a dedicated sweep.

A parameter list like ``int foo(int x, int y)`` already tells the reader the names and types.
A Doxygen block that just repeats that, one line per parameter with no real explanation, adds nothing.
Delete blocks like that instead of converting them to plain comments.

Prefer clear code to a comment
******************************

Keep functions in C source files short and self-explanatory, and let the names carry the meaning.
A comment is the right answer when the code cannot state something itself, not when the code is hard to read.

Write a comment to record:

* Why the code does something, when the reason is not local.
  Hardware errata, SoftDevice constraints, protocol requirements, and timing dependencies all qualify.
* A constraint the compiler cannot express, such as the interrupt context a function runs in, or a lock the caller must already hold.
* Where a magic value comes from, such as a datasheet section or a register field.

A comment that restates the code earns nothing:

.. code-block:: c

   /* Increment the counter. */
   counter++;

A comment that supplies what the reader cannot deduce earns its place:

.. code-block:: c

   /* The SoftDevice expects the connection interval in 1.25 ms units,
    * while this API takes milliseconds.
    */
   conn_params.min_conn_interval = MSEC_TO_UNITS(min_interval_ms, UNIT_1_25_MS);

Place a comment directly above the code it describes, with no blank line between them, at the same indentation.
Use a trailing comment only where it describes a single field or value and stays within the line limit.

Private header guards
*********************

Name the include guard after the file, in upper case, with a ``_H__`` suffix, and repeat it in a comment on the closing ``#endif``:

.. code-block:: c

   #ifndef PEER_DATABASE_H__
   #define PEER_DATABASE_H__

   /* Declarations. */

   #endif /* PEER_DATABASE_H__ */

Do not add a ``@file`` block, a file-contents summary, a change log, or an author name to a C source file or a private header.
Git already tracks who changed what and when.
A hand-maintained summary duplicates that information and goes stale when nobody updates it.

Commented-out code
******************

Do not commit commented-out code.
Delete it.

Where a block is unfinished rather than obsolete, mark it with ``TODO:`` and give enough context for somebody else to act on it:

.. code-block:: c

   /* TODO: Replace the fixed delay once the driver reports readiness. */

Do not leave a bare ``TODO`` or ``FIXME`` with no explanation.
