lib_src change log
==================

2.3.0
-----

  * ADDED: XS3 VPU optimised voice fixed factor of 3 upsampling/downsampling
  * ADDED: XS3 VPU optimised voice fixed factor of 3/2 upsampling/downsampling
  * CHANGED: OS3 uses firos3_144.dat coefficients by default inline with model
  * CHANGED: Replaced xmostest with pytest for all SRC automated tests
  * CHANGED: Used XMOS doc builder for documentation
  * CHANGED: Golden reference test signals now generated automatically by CI
  * RESOLVED: Linker warning on channel ends
  * REMOVED: AN00231 ASRC App Note. See github.com/xmos/sln_voice/examples
  * CHANGED: Increased precision of the fFsRatioDeviation used in the C
    emulator from float to double

  * Changes to dependencies:

    - lib_logging: 2.0.1 -> 3.1.1

    - lib_xassert: 2.0.1 -> 4.1.0

2.2.0
-----

  * CHANGED: Made the FIR coefficient array that is used with the voice fixed
    factor of 3 up and down sampling functions usable from within C files as
    well as XC files.
  * CHANGED: Aligned the FIR coefficient array to an 8-byte boundary. This
    ensures that the voice fixed factor of 3 up and down sampling functions do
    not crash with a LOAD_STORE exception.
  * ADDED: Missing device attributes to the .xn file of the AN00231 app note.
  * ADDED: Minimal cmake support.

2.1.0
-----

  * CHANGED: Use XMOS Public License Version 1

2.0.1
-----

  * CHANGED: Pin Python package versions
  * REMOVED: not necessary cpanfile

2.0.0
-----

  * CHANGED: Build files updated to support new "xcommon" behavior in xwaf.

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

