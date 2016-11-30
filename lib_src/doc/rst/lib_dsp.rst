Overview
--------

Introduction
............

Library Organization
....................

The library is divided into function collections with each collection covering a specific
digital signal processing algorithm category.
The API and implementation for each category are provided by a single 'C' header file and
implementation file.

.. table:: DSP library organization

  +-------------+----------------+---------------------------------------------------------------+
  | Sample Rate | dsp_ds3,       | Downsample by factor of 3 (e.g. 48KHz to 16KHz Audio SRC).    |
  | Conversion  | dsp_os3        | Oversample by factor of 3 (e.g. 16KHz to 48KHz Audio SRC).    |
  +-------------+----------------+---------------------------------------------------------------+


Fixed-Point Format
------------------

Q Format Introduction
.....................

The library functions support 32 bit input and output data, with internal 64 bit accumulator.
The output data can be scaled to any of the supported Q Formats (Q8 through Q31), for all functions except for
sample rate conversion, which uses a fixed Q31 format.

Further details about Q Format numbers is available `here <https://en.wikipedia.org/wiki/Q_(number_format)>`_.

The 'q_format' Parameter
........................

All XMOS DSP library functions that incorporate a multiply operation accept a parameter called
q_format. This parameter can naively be used to specify the fixed point format for all
operands and results (if applicable) where the formats are the same for all parameters. For
example::

  result_q28 = dsp_math_multiply( input1_q28, input2_q28, 28 );
  
The 'q_format' parameter, being used after one or more sequences of multiply and/or
multiply-accumulate, is used to right-shift the 64-bit accumulator before truncating the value
back to a 32-bit integer (i.e. the 32-bit fixed-point result).  Therefore the 'q_format'
parameter can be used to perform the proper fixed-point adjustment for any combination of input
operand fixed-point format and desired fixed-point result format.

The output fixed-point fraction
bit count is equal to the sum of the two input fraction bit counts minus the desired result fraction
bit count::

  q_format = input1 fraction bit count +  input2 fraction bit count - result fraction bit count
  
For example::

  // q_format_parameter = 31 = 30 + 29 - 28
  result_q28 = dsp_math_multiply( input1_q30, input2_q29, 31 );

  // q_format_parameter = 27 = 28 + 29 - 30
  result_q30 = dsp_math_multiply( input1_q28, input2_q29, 27 );
  

Audio Sample Rate Conversion
----------------------------

The DSP library includes synchronous sample rate conversion functions to downsample (decimate) and oversample (upsample or interpolate) by a factor of three. In each case, the DSP processing is carried out each time a single output sample is required. In the case of the decimator, three input samples passed to filter with a resulting one sample output on calling the processing function. The interpolator produces an output sample each time the processing function is called but will require a single sample to be pushed into the filter every third cycle. All samples use Q31 format (left justified signed 32b integer).

Both sample rate converters are based on a 144 tap FIR filter with two sets of coefficients available, depending on application requirements:

 * firos3_b_144.dat / firds3_b_144.dat - These filters have 20dB of attenuation at the nyquist frequency and a higher cutoff frequency
 * firos3_144.dat / firds3_144.dat - These filters have 60dB of attenuation at the nyquist frequency but trade this off with a lower cutoff frequency

The filter coefficients may be selected by adjusting the line::

  #define   FIROS3_COEFS_FILE

and::

  #define   FIRDS3_COEFS_FILE

in the files ``dsp_os3.h`` (API for oversampling) and ``dsp_ds3.h`` (API for downsampling) respectively.

The OS3 processing takes up to 153 core cycles to compute a sample which translates to 1.53us at 100MHz or 2.448us at 62.5MHz core speed. This permits up to 8 channels of 16KHz -> 48KHz sample rate conversion in a single 62.5MHz core.

The DS3 processing takes up to 389 core cycles to compute a sample which translates to 3.89us at 100MHz or 6.224us at 62.5MHz core speed. This permits up to 9 channels of 48KHz -> 16KHz sample rate conversion in a single 62.5MHz core. 

Both downsample and oversample functions return ``ERROR`` or  ``NOERROR`` status codes as defined in return codes enums listed below.

The down sampling functions return the following error codes ::

  FIRDS3_NO_ERROR
  FIRDS3_ERROR

The up sampling functions return the following error codes ::

  FIROS3_NO_ERROR
  FIROS3_ERROR

For details on synchronous audio sample rate conversion by factors of two, or asynchronous audio sample rate conversion please see the XMOS Sample Rate Conversion Library [#]_.

.. [#] http://www.xmos.com/published/lib_src-userguide


DS3 Function API
................

.. doxygenenum:: dsp_ds3_return_code_t

.. doxygenfunction:: dsp_ds3_init

.. doxygenfunction:: dsp_ds3_sync

.. doxygenfunction:: dsp_ds3_proc

OS3 Function API
................

.. doxygenenum:: dsp_os3_return_code_t

.. doxygenfunction:: dsp_os3_init

.. doxygenfunction:: dsp_os3_sync

.. doxygenfunction:: dsp_os3_input

.. doxygenfunction:: dsp_os3_proc
