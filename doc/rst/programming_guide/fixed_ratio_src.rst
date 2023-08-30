Fixed factor of 3 functions
===========================

The SRC library also includes synchronous sample rate conversion functions to downsample (decimate) and oversample (upsample or interpolate) by a fixed factor of three. In each case, the processing is carried out each time a single output sample is required. In the case of the decimator, three input samples passed to filter with a resulting one sample output on calling the processing function. The interpolator produces an output sample each time the processing function is called but will require a single sample to be pushed into the filter every third cycle. All samples use Q31 format (left justified signed 32b integer).

Both sample rate converters are based on a 144 tap FIR filter with two sets of coefficients available, depending on application requirements:

 * firos3_b_144.dat / firds3_b_144.dat - These filters have 20dB of attenuation at the nyquist frequency and a higher cutoff frequency
 * firos3_144.dat / firds3_144.dat - These filters have 60dB of attenuation at the nyquist frequency but trade this off with a lower cutoff frequency

The default setting is to use the 60dB of attenuation at the nyquist frequency coefficients.

The filter coefficients may be selected by adjusting the line::

  #define   FIROS3_COEFS_FILE

and::

  #define   FIRDS3_COEFS_FILE

in the files ``src_ff3_os3.h`` (API for oversampling) and ``src_ff3_ds3.h`` (API for downsampling) respectively.

The OS3 processing takes up to 153 core cycles to compute a sample which translates to 1.53us at 100MHz or 2.448us at 62.5MHz core speed. This permits up to 8 channels of 16KHz -> 48KHz sample rate conversion in a single 62.5MHz core.

The DS3 processing takes up to 389 core cycles to compute a sample which translates to 3.89us at 100MHz or 6.224us at 62.5MHz core speed. This permits up to 9 channels of 48KHz -> 16KHz sample rate conversion in a single 62.5MHz core.

Both downsample and oversample functions return ``ERROR`` or  ``NOERROR`` status codes as defined in return codes enums listed below. The only way these functions can error is if the passed `delay_base` structure member has been uninitialised and is NULL.

The down sampling functions return the following error codes ::

  FIRDS3_NO_ERROR
  FIRDS3_ERROR

The up sampling functions return the following error codes ::

  FIROS3_NO_ERROR
  FIROS3_ERROR

API
---

.. doxygenenum:: src_ff3_return_code_t

DS3 API
-------

.. doxygenstruct:: src_ds3_ctrl_t

.. doxygenfunction:: src_ds3_init

.. doxygenfunction:: src_ds3_sync

.. doxygenfunction:: src_ds3_proc

OS3 API
-------

.. doxygenstruct:: src_os3_ctrl_t

.. doxygenfunction:: src_os3_init

.. doxygenfunction:: src_os3_sync

.. doxygenfunction:: src_os3_input

.. doxygenfunction:: src_os3_proc


Fixed factor of 3 functions optimised for use with voice
========================================================

TODO ADD SOME INTRO HERE

..
  .. doxygenvariable:: src_ff3v_fir_coefs_debug

..
  .. doxygenvariable:: src_ff3v_fir_coefs

Voice DS3 API
-------------

.. doxygenfunction:: src_ds3_voice_add_sample

.. doxygenfunction:: src_ds3_voice_add_final_sample

Voice US3 API
-------------

.. doxygenfunction:: src_us3_voice_input_sample

.. doxygenfunction:: src_us3_voice_get_next_sample


Fixed factor of 3 and 3/2 functions optimised for XS3 (xCORE-AI) and for use with voice
=======================================================================================

TODO ADD SOME INTRO HERE

Example usage 16 kHz <-> 48 kHz and 32 kHz <-> 48 kHz

ALSO UPDATE THESE BELOW

Use Pavel's table.

https://xmosjira.atlassian.net/wiki/spaces/CONF/pages/3771990022/XVF3800+SRC+Low-Pass+filter+design

https://xmosjira.atlassian.net/wiki/spaces/CONF/pages/3784245249/XVF3820+SRC+Low-Pass+filter+design

..
  .. doxygenvariable:: src_ff3v_fir_coefs_debug

..
  .. doxygenvariable:: src_ff3v_fir_coefs

Voice DS3 API
-------------

.. doxygengroup:: src_ff3_96t_ds
   :content-only:

Voice US3 API
-------------

.. doxygengroup:: src_ff3_96t_us
   :content-only:

Voice DS3/2 API
-------------

.. doxygengroup:: src_rat_2_3_96t_ds
   :content-only:

Voice US3/2 API
-------------

.. doxygengroup:: src_rat_3_2_96t_us
   :content-only:



