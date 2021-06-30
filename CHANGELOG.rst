lib_src change log
==================

2.2.0
-----

  * CHANGED: Made the FIR coefficient array that is used with the voice fixed
    factor of 3 up and down sampling functions usable from within C files as
    well as XC files.
  * CHANGED: Aligned the FIR coefficient array to an 8-byte boundary. This
    ensures that the voice fixed factor of 3 up and down sampling functions do
    not crash with a LOAD_STORE exception.

2.1.0
-----

  * CHANGED: Use XMOS Public Licence Version 1

2.0.1
-----

  * CHANGED: Pin Python package versions
  * REMOVED: not necessary cpanfile

2.0.0
-----

  * CHANGED: Build files updated to support new "xcommon" behaviour in xwaf.

1.1.2
-----

  * CHANGED: initialisation lists to avoid warnings when building

1.1.1
-----

  * RESOLVED: correct compensation factor for voice upsampling
  * ADDED: test of voice unity gain

1.1.0
-----

  * ADDED: Fixed factor of 3 conversion functions for downsampling and
    oversampling
  * ADDED: Fixed factor of 3 downsampling function optimised for use with voice
    (reduced memory and compute footprint)
  * ADDED: Fixed factor of 3 upsampling function optimised for use with voice
    (reduced memory and compute footprint)

1.0.0
-----

  * Initial version

  * Changes to dependencies:

    - lib_logging: Added dependency 2.0.1

    - lib_xassert: Added dependency 2.0.1

