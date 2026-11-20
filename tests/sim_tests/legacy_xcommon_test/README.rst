Legacy xcommon smoke test
=========================

This standalone XC/C application checks that legacy xcommon can build and link
``lib_src`` for the XK-EVK-XU316. It also checks that:

* SSRC doubles the sample count and preserves silence when converting from
  48 kHz to 96 kHz.
* The asynchronous FIFO passes frames through in order, then reports underflow,
  enters reset, and is cleared by the producer. It also handles a reset
  requested by the consumer.

With the XMOS tools environment sourced and the dependencies checked out
alongside the ``lib_src`` repository, run from this directory. The test uses
local modules and reports missing dependencies without downloading them::

    xmake
    xsim bin/legacy_xcommon_test.xe

The Jenkins ``Legacy xcommon build`` stage uses the XTC version selected by
``TOOLS_VERSION_XS`` to run ``xmake`` and then the binary in ``xsim``. The stage
fails if the application returns a non-zero exit code.
