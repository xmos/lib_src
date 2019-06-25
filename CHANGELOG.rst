lib_src change log
==================

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

