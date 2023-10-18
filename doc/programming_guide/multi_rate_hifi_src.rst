Multi-rate HiFi Sample Rate Conversion
======================================

Usage
-----

Both SSRC and ASRC functions are accessed via standard function calls, making them accessible from C or XC. Both SSRC and ASRC functions are passed an external state structure which provides re-entrancy. The functions may be called in-line with other signal processing or placed on a logical core within its own task to provide guaranteed performance. By placing the calls to SRC functions on separate logical cores, multiple instances can be processed concurrently.

The API is designed to be simple and intuitive with just two public functions per sample rate converter type.


Initialization
..............

All public ASRC and SSRC functions are declared within the ``src.h`` header::

  #include "src.h"

There are a number of arrays of structures that must be declared from the application which contain the buffers between the FIR stages, state and adapted coefficients (ASRC only). 
There must be one element of each structure declared for each channel handled by the SRC instance. 
The structures are then all linked into a single control structure, allowing a single reference to be passed each time a call to the SRC is made.

For SSRC, the following state structures are required::

    //State of SSRC module
    ssrc_state_t     ssrc_state[SSRC_CHANNELS_PER_INSTANCE];
    //Buffers between processing stages
    int              ssrc_stack[SSRC_CHANNELS_PER_INSTANCE][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];
    //SSRC Control structure
    ssrc_ctrl_t      ssrc_ctrl[SSRC_CHANNELS_PER_INSTANCE];


For ASRC, the following state structures are required. Note that only one instance of the filter coefficients need be declared because these are shared amongst channels within the instance::

    //ASRC state
    asrc_state_t       asrc_state[ASRC_CHANNELS_PER_INSTANCE];
    int                asrc_stack[ASRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * ASRC_N_IN_SAMPLES];
    //Control structure
    asrc_ctrl_t        asrc_ctrl[ASRC_CHANNELS_PER_INSTANCE];
    //Adaptive filter coefficients
    asrc_adfir_coefs_t asrc_adfir_coefs;



There is an initialization call which sets up the variables within the structures associated with the SRC instance and clears the inter-stage buffers. 
Initialization ensures the correct selection, ordering and configuration of the filtering stages, be they decimators, interpolators or pass-through blocks. 
This initialization call contains arguments defining selected input and output nominal sample rates as well as settings for the sample rate converter:

:c:func:`ssrc_init`

The initialization call is the same for ASRC:

:c:func:`asrc_init`

The input block size must be a power of 2 and is set by the ``n_in_samples`` argument. In the case where more than one channel is to be processed per SRC instance, the total number of 
input samples expected for each processing call is ``n_in_samples * n_channels_per_instance``.

Please ensure the settings within ``src_config.h`` are correct for the application. The default settings allow for any input/output ratio between 44.1 kHz and 192 kHz.



Processing
..........

Following initialization, the processing API is called for each block of input samples which will then produce a block of output samples as shown in :numref:`fig_src_proc`.

.. _fig_src_proc:
.. figure:: images/src_proc.pdf
   :width: 100%

   SRC Operation

The logic is designed so that the final filtering stage always receives a sample to process. The sample rate converters have been designed to handle a maximum decimation of factor four from the first two stages. This architecture requires a minimum input block size of 4 to operate.


The processing function call is passed the input and output buffers and a reference to the control structure:

:c:func:`ssrc_process`

In the case of ASRC a fractional frequency ratio argument is also supplied:

:c:func:`asrc_process`

The SRC processing call always returns a whole number of output samples produced by the sample rate conversion. Depending on the sample ratios selected, this number may be between zero and ``(n_in_samples * n_channels_per_instance * SRC_N_OUT_IN_RATIO_MAX)``. ``SRC_N_OUT_IN_RATIO_MAX`` is the maximum number of output samples for a single input sample. For example, if the input frequency is 44.1 kHz and the output rate is 192 kHz then a sample rate conversion of one sample input may produce up to 5 output samples.

The fractional number of samples produced to be carried to the next operation is stored inside the control structure, and additional whole samples are added during subsequent calls to the sample rate converter as necessary.

For example, a sample rate conversion from 44.1 kHz to 48 kHz with a input block size of 4 will produce a 4 sample result with a 5 sample result approximately every third call.

Each SRC processing call returns the integer number of samples produced during the sample rate conversion.

The SSRC is synchronous in nature and assumes that the ratio is equal to the nominal sample rate ratio. For example, to convert from 44.1 kHz to 48 kHz, it is assumed that the word clocks of the input and output stream are derived from the same master clock and have an exact ratio of 147:160.

If the word clocks are derived from separate oscillators, or are not synchronous (for example are derived from each other using a fractional PLL), the ASRC must be used.

Buffer Formats
..............

The format of the sample buffers sent and received from each SRC instance is time domain interleaved. How this looks in practice depends on the number of channels and SRC instances. Three examples are shown below, each showing ``n_in_samples = 4``. The ordering of sample indicies is 0 representing the oldest sample and ``n - 1``, where n is the buffer size, representing the newest sample.

In the case where two channels are handled by a single SRC instance :numref:`fig_st_si` shows that the samples are interleaved into a single buffer of size 8.

.. _fig_st_si:
.. figure:: images/stereo_single_instance.pdf
   :width: 25%

   Buffer Format for Single Stereo SRC instance


Where a single audio channel is mapped to a single instance, the buffers are simply an array of samples starting with the oldest sample and ending with the newest sample as shown in :numref:`fig_m_si`.

.. _fig_m_si:
.. figure:: images/stereo_dual_instance.pdf
   :width: 50%

   Buffer Format for Dual Mono SRC instances




In the case where four channels are processed by two instances, channels 0 & 1 are processed by SRC instance 0 and channels 2 & 3 are processed by SRC instance 1 as shown in :numref:`fig_4_di`. For each instance, four pairs of samples are passed into the SRC processing function and n pairs of samples are returned, where n depends on the input and output sample rate ratio.

.. _fig_4_di:
.. figure:: images/quad_dual_instance.pdf
   :width: 50%

   Buffer Format for Dual Stereo SRC instances (4 channels total)

In addition to the above arguments the ``asrc_process()`` call also requires an unsigned Q4.28 fixed point ratio value specifying the actual input to output ratio for the next calculated block of samples. This allows the input and output rates to be fully asynchronous by allowing rate changes on each call to the ASRC. The converter dynamically computes coefficients using a spline interpolation within the last filter stage. It is up to the callee to maintain the input and output sample rate ratio difference.

Further details about these function arguments are contained here: `SSRC API`_.


Performance and resource utilization
------------------------------------


Audio Performance
.................

The performance of the SSRC library is as follows:

 * THD+N (1 kHz, 0 dBFs): better than -130 dB, depending on the accuracy of the ratio estimation
 * SNR: 140 dB (or better). Note that when dither is not used, SNR is infinite as output from a zero input signal is zero.


To see frequency plots illustrating the noise floor with respect to a sample rate converted tone please refer to the :ref:`performance-plots` section of this document.


SSRC Resource utilization
.........................

.. include:: ./resource_usage_ssrc.rst





ASRC Performance
................

The performance of the ASRC library is as follows:

 * THD+N: (1 kHz, 0 dBFs): better than -130 dB
 * SNR:   135 dB (or better). Note that when dither is not used, SNR is infinite as output from a zero input signal is zero.

To see frequency plots illustrating the noise floor with respect to a sample rate converted tone please refer to the :ref:`performance-plots` section of this document.


ASRC Resource utilization
.........................

.. include:: ./resource_usage_asrc.rst




SRC Implementation
------------------

The SSRC and ASRC implementations are closely related to each other and share the majority of the system building blocks. The key difference between them is that SSRC uses fixed polyphase 160:147 and 147:160 final rate change filters whereas the ASRC uses an adaptive polyphase filter. The ASRC adaptive polyphase coefficients are computed for every sample using second order spline based interpolation.



SSRC Structure
..............

The SSRC algorithm is based on three cascaded FIR filter stages (F1, F2 and F3). These stages are configured differently depending on rate change and only part of them is used in certain cases. :numref:`fig_ssrc_struct` shows an overall view of the SSRC algorithm:

.. _fig_ssrc_struct:
.. figure:: images/ssrc_structure.pdf
   :width: 90%

   SSRC Algorithm Structure

The SSRC algorithm is implemented as a two stage structure:

 * The bandwidth control stage which includes filters F1 and F2 is responsible for limiting the bandwidth of the input signal and for providing integer rate Sample Rate Conversion. It is also used for signal conditioning in the case of rational non-integer Sample Rate Conversion.
 * The polyphase filter stage which converts between the 44.1 kHz and the 48 kHz families of sample rates.



ASRC Structure
..............

Similar to the SSRC, the ASRC algorithm is based on three cascaded FIR filters (F1, F2 and F3). These are configured differently depending on rate change and F2 is not used in certain rate changes. :numref:`fig_asrc_struct` shows an overall view of the ASRC algorithm:

.. _fig_asrc_struct:
.. figure:: images/asrc_structure.pdf
   :width: 90%

   ASRC Algorithm Structure

The ASRC algorithm is implemented as a two stage structure:

 * The bandwidth control stage includes filters F1 and F2 which are responsible for limiting the bandwidth of the input signal and for providing integer rate sample rate conversion to condition the input signal for the adaptive polyphase stage (F3).
 * The polyphase filter stage consists of the adaptive polyphase filter F3, which effectively provides the asynchronous connection between the input and output clock domains.


SRC Filter list
...............

A complete list of the filters supported by the SRC library, both SSRC and ASRC, is shown in :numref:`fig_src_filters`. The filters are implemented in C within the ``FilterDefs.c`` function and the coefficients can be found in the ``/FilterData`` folder. The particular combination of filters cascaded together for a given sample rate change is specified in ``ssrc.c`` and ``asrc.c``.


.. _fig_src_filters:
.. list-table:: SRC Filter Specifications
     :header-rows: 2

     * - Filter
       - Fs (norm)
       - Passband
       - Stopband
       - Ripple
       - Attenuation
       - Taps
       - Notes
     * - BL
       - 2
       - 0.454
       - 0.546
       - 0.01 dB
       - 155 dB
       - 144
       - Down-sampler by two, steep
     * - BL9644
       - 2
       - 0.417
       - 0.501
       - 0.01 dB
       - 155 dB
       - 160
       - Low-pass filter, steep for 96 to 44.1
     * - BL8848
       - 2
       - 0.494
       - 0.594
       - 0.01 dB
       - 155 dB
       - 144
       - Low-pass, steep for 88.2 to 48
     * - BLF
       - 2
       - 0.41
       - 0.546
       - 0.01 dB
       - 155 dB
       - 96
       - Low-pass at half band
     * - BL19288
       - 2
       - 0.365
       - 0.501
       - 0.01 dB
       - 155 dB
       - 96
       - Low pass, steep for 192 to 88.2
     * - BL17696
       - 2
       - 0.455
       - 0.594
       - 0.01 dB
       - 155 dB
       - 96
       - Low-pass, steep for 176.4 to 96
     * - UP
       - 2
       - 0.454
       - 0.546
       - 0.01 dB
       - 155 dB
       - 144
       - Over sample by 2, steep
     * - UP4844
       - 2
       - 0.417
       - 0.501
       - 0.01 dB
       - 155 dB
       - 160
       - Over sample by 2, steep for 48 to 44.1
     * - UPF
       - 2
       - 0.41
       - 0.546
       - 0.01 dB
       - 155 dB
       - 96
       - Over sample by 2, steep for 176.4 to 192
     * - UP192176
       - 2
       - 0.365
       - 0.501
       - 0.01 dB
       - 155 dB
       - 96
       - Over sample by 2, steep for 192 to 176.4
     * - DS
       - 4
       - 0.57
       - 1.39
       - 0.01 dB
       - 160 dB
       - 32
       - Down sample by 2, relaxed
     * - OS
       - 2
       - 0.57
       - 1.39
       - 0.01 dB
       - 160 dB
       - 32
       - Over sample by 2, relaxed
     * - HS294
       - 284
       - 0.55
       - 1.39
       - 0.01 dB
       - 155 dB
       - 2352
       - Polyphase 147/160 rate change
     * - HS320
       - 320
       - 0.55
       - 1.40
       - 0.01 dB
       - 151 dB
       - 2560
       - Polyphase 160/147 rate change
     * - ADFIR
       - 256
       - 0.45
       - 1.45
       - 0.012 dB
       - 170 dB
       - 1920
       - Adaptive polyphase prototype filter




SRC File Structure and Overview
-------------------------------

All source files for the SSRC and ASRC are located within the ``multirate_hifi`` subdirectory.

 * src_mrhf_ssrc_wrapper.c / src_mrhf_ssrc_wrapper.h

   These wrapper files provide a simplified public API to the SSRC initialization and processing functions.


 * src_mrhf_asrc_wrapper.c / src_mrhf_asrc_wrapper.h

   These wrapper files provide a simplified public API to the ASRC initialization and processing functions.


 * src_mrhf_ssrc.c / src_mrhf_ssrc.h

   These files contain the core of the SSRC algorithm. They set up the correct filtering chains depending on rate change and apply in the processing calls. The table sFiltersIDs declared in SSRC.c contains definitions of the filter chains for all supported rate changes. The files also integrate the code for the optional dithering function.


 * src_mrhf_asrc.c / src_mrhf_asrc.h

   These files contain the core of the ASRC algorithm. They setup the correct filtering chains depending on rate change and apply them for the corresponding processing calls. Note that filters F1, F2 and dithering are implemented using a block based approach similar to SSRC. The adaptive polyphase filter (ADFIR) is implemented on a sample by sample basis. These files also contain functions to compute the adaptive polyphase filter coefficients.


 * src_mrhf_fir.c / src_mrhf_fir.h

   These files provide Finite Impulse Response (FIR) filtering setup, with calls to the assembler-optimized inner loops. They provide functions for handling down-sampling by 2, synchronous or over-sampling by 2 FIRs. They also provides functions for handling polyphase filters used for rational ratio rate change in the SSRC and adaptive FIR filters used in the asynchronous section of the ASRC.


 * src_mrhf_filter_defs.c / src_mrhf_filter_defs.h

   These files define the size and coefficient sources for all the filters used by the SRC algorithms.


 * /FilterData directory (various files)

   This directory contains the pre-computed coefficients for all of the fixed FIR filters. The numbers are stored as signed Q1.31 format and are directly included in the source of FilterDefs.c. Both the .dat files used by the C compiler and the .sfp ScopeFIR (http://iowegian.com/scopefir/) design source files, used to originally create the filters, are included.


 * src_mrhf_fir_inner_loop_asm.S / src_mrhf_fir_inner_loop_asm.h

   Inner loop for the standard FIR function optimized for double-word load and store, 32 bit * 32 bit -> 64 bit MACC and saturation instructions. Even and odd sample long word alignment versions are provided.


 * src_mrhf_fir_os_inner_loop_asm.S / scr_mrhf_fir_os_inner_loop_asm.h

   Inner loop for the oversampling FIR function optimized for double-word load and store, 32 bit * 32 bit -> 64 bit MACC and saturation instructions. Both (long word) even and odd sample input versions are provided.


 * src_mrhf_spline_coeff_gen_inner_loop_asm.S / src_mrhf_spline_coeff_gen_inner_loop_asm.h

   Inner loop for generating the spline interpolated coefficients. This assembler function is optimized for double-word load and store, 32 bit * 32 bit -> 64 bit MACC and saturation instructions.


 * src_mrhf_adfir_inner_loop_asm.S / src_mrhf_adfir_inner_loop_asm.h

   Inner loop for the adaptive FIR function using the previously computed spline interpolated coefficients. It is optimized for double-word load and store, 32 bit * 32 bit -> 64 bit MACC and saturation instructions. Both (long word) even and odd sample input versions are provided.


 * src_mrhf_int_arithmetic.c / src_mrhf_int_arithmetic.h

   These files contain simulation implementations of XMOS ISA specific assembler instructions. These are only used for dithering functions, and may be eliminated during future optimizations.



SSRC API
--------

.. doxygengroup:: src_ssrc
   :content-only:

ASRC API
--------

.. doxygengroup:: src_asrc
   :content-only:
