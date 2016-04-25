.. include:: ../../../README.rst

Usage
-----

Both SSRC and ASRC functions are accessed via a standard function calls, making them accessible from C or XC. Both SSRC and ASRC functions are passed an external state structure which provides re-entrancy. The functions may be called in-line with your processing or placed on a logical core within it's own task to provide guaranteed performance. By placing the calls to SRC functions on sepearte logical cores, multiple instances can be processed concurrently.

The API is designed to be as simple and intuitive with just two public functions per sample rate converter type.


Initialization
..............

There is an initialization call which sets up the variables within the structures associated with the SRC instance and clears the inter-stage buffers. Initialization must be called to ensure the correct selection and ordering and configuration of the filtering stages, be they decimators, interpolators or pass through blocks. This initialization call contains arguments defining selected input and output nominal sample rates as well as settings for the sample rate converter::

   void ssrc_init(const fs_code_t sr_in, const fs_code_t sr_out, ssrc_ctrl_t *ssrc_ctrl, const unsigned n_channels_per_instance, const unsigned n_in_samples, const dither_flag_t dither_on_off);

The initialization call is the same for ASRC::

   unsigned asrc_init(const fs_code_t sr_in, const fs_code_t sr_out, asrc_ctrl_t asrc_ctrl[], const unsigned n_channels_per_instance, const unsigned n_in_samples, const dither_flag_t dither_on_off);

The settings include:

 * Nominal input sample rate as an enumerated type
 * Nominal output sample rate as an enumerated type
 * The number of channels to be handled by this instance of SRC
 * The number of input samples to expect. Minimum 4 samples input per call, must be power of 2
 * The dither setting. Dithers the output from 32bit to 24bit

The input block size must be a power of 2 and is set by the ``n_in_samples`` argument. In the case where more than one channel is to be processed per SRC instance, the total number of input samples expected for each processing call is ``n_in_samples * n_channels_per_instance``. 

There are a number of arrays of structures that must be declared from the application which contain the state, buffers between the FIR stages, state and adapted coefficients (ASRC only). There must be one element of each structure declared for each channel handled by the SRC instance. The structures are then all linked into a single control structure, allowing a single reference to be passed each time a call to the SRC is made.

For the case of SSRC, the following state structures are required::

    //State of SSRC module
    ssrc_state_t     ssrc_state[SSRC_CHANNELS_PER_INSTANCE];
    //Buffers between processing stages
    int              ssrc_stack[SSRC_CHANNELS_PER_INSTANCE][SSRC_STACK_LENGTH_MULT * SSRC_N_IN_SAMPLES];
    //SSRC Control structure  
    ssrc_ctrl_t      ssrc_ctrl[SSRC_CHANNELS_PER_INSTANCE];

For the ASRC, the state structures must be declared. Note that only one instance of the filter coefficients need be declared because these are shared amongst channels within the instance::

    //ASRC state
    asrc_state_t       asrc_state[ASRC_CHANNELS_PER_INSTANCE];     //Buffers between filter stages                                    
    int                asrc_stack[ASRC_CHANNELS_PER_INSTANCE][ASRC_STACK_LENGTH_MULT * ASRC_N_IN_SAMPLES];  
    //Control structure
    asrc_ctrl_t        asrc_ctrl[ASRC_CHANNELS_PER_INSTANCE];
    //Adaptive filter coefficients  
    asrc_adfir_coefs_t asrc_adfir_coefs;                                                                 

Processing
..........

Following initialization, the processing API is called for each block of input samples. The logic is designed so that the final filtering stage always receives a sample to process. The sample rate converters have been designed to handle a maximum decimation of factor four from the first two stages. This architecture requires a minimum input block size of 4 to operate.

.. figure:: images/src_proc.pdf
   :width: 100%

   SRC Operation  

The processing function call is passed the input and output buffers and a reference to the control structure:: 

    unsigned ssrc_process(int in_buff[], int out_buff[], ssrc_ctrl_t *ssrc_ctrl)

In the case of ASRC, additionally a fractional frequency ratio is supplied::

    unsigned asrc_process(int *in_buff, int *out_buff, unsigned fs_ratio, asrc_ctrl_t asrc_ctrl[])

The SRC processing call always returns a whole number of output samples produced by the sample rate conversion. Depending on the sample ratios selected, this number may be between zero and ``(n_in_samples * n_channels_per_instance * SRC_N_OUT_IN_RATIO_MAX)``. ``SRC_N_OUT_IN_RATIO_MAX`` is the maximum number of output samples for a single input sample. For example, if the input frequency is 44.1KHz and the output rate is 192KHz then a sample rate conversion of one sample input may contain up to 5 output samples.

The fractional number of samples produced to be carried to the next operation are stored internally inside the control structure, and additional whole samples are added during subsequent calls to the sample rate converter.

For example, a sample rate conversion from 44.1KHz to 48KHz with a input block size of 4 will produce a 4 sample result with a 5 sample result approximately every third call. 

Each SRC processing call returns the integer number of samples produced during the sample rate conversion.

The SSRC is synchronous in nature and assumes that the ratio is equal to the nominal sample rate ratio. For example, to convert from 44.1KHz to 48KHz, it is assumed that the word clocks of the input and output stream are derived from the same master clock and have an exact ratio of 147:160.

If the word clocks are derived from separate oscillators, or are not synchronous (for example are derived from each other using a fractional PLL), the the ASRC must be used.

Buffer Formats
..............

The format of the sample buffers sent and received from each SRC instance is time domain interleaved. How this looks in practice depends on the number of channels and SRC instances. Three examples are shown below, each showing ``n_in_samples = 4``. The ordering of sample indicies is 0 representing the oldest number and ``n - 1``, where n is the buffer size, representing the newest sample.

In the case where two channels are handled by a single SRC instance, you can see that the samples are interleaved into a single buffer of size 8.

.. figure:: images/stereo_single_instance.pdf
   :width: 25%

   Buffer Format for Single Stereo SRC instance 


Where a single audio channel is mapped to a single instance, the buffers are simply an array of samples starting with the oldest sample and ending with the newest sample.

.. figure:: images/stereo_dual_instance.pdf
   :width: 50%

   Buffer Format for Dual Mono SRC instances 


|newpage|

In the case where four channels are processed by two instances, channels 0 & 1 are processed by SRC instance 0 and channels 2 & 3 are processed by SRC instance 1. For each instance, four pairs of samples are passed into the SRC processing function and n pairs of samples are returned, where n depends on the input and output sample rate ratio.

.. figure:: images/quad_dual_instance.pdf
   :width: 50%

   Buffer Format for Dual Stereo SRC instances (4 channels total)  

In addition to the above arguments the ``asrc_process()`` call also requires an unsigned Q4.28 fixed point ratio value specifying the actual input to output ratio for the next calculated block of samples. This allows the input and output rates to be fully asynchronous by allowing rate changes on each call to the ASRC. The converter dynamically computes coefficients using a spline interpolation within the last filter stage. It is up to the callee to maintain the input and output sample rate ratio difference. An example of this calculation, based on measuring the input and output rates, is provided in AN00231.

Further detail about these function arguments are contained within the API section of this guide. 


|newpage|


SSRC Performance
----------------

The performance of the SSRC library is as follows:

 * THD+N (1kHz, 0dBFs): better than -130dB, depending on the accuracy of the ratio estimation
 * SNR:   140dB (or better). Note that when dither is not used, SNR is infinite as output from a zero input signal is zero.

The performance was analyzed by converting output test files to 32 bits integer .wav files. These files were then run through an audio analysis tool (WinAudio MLS: http://www.dr-jordan-design.de/Winaudiomls.htm).

Below are a series FFT plots showing the most demanding rate conversion case. These clearly show that the above targets are comfortably exceeded. All outputs have been generated using 8192 samples at input sampling rate. A Kaiser-Bessel window with alpha=7 has been used.

.. figure:: images/ssrc_fft_44_192.pdf
   :width: 90%

   FFT of 1kHz sine, 0dB, 44.1kHz to 192kHz  

.. figure:: images/ssrc_fft_176_48.pdf
   :width: 90%

   FFT of 1kHz sine, 0dB, 176.4kHz to 48kHz 

.. figure:: images/ssrc_fft_96_88.pdf
   :width: 90%

   FFT of 10kHz+11kHz sines, -6dB, 96kHz to 88.2kHz


ASRC Performance
----------------

The performance of the SSRC library is as follows:

 * THD+N: (1kHz, 0dBFs): better than -130dB 
 * SNR:   135dB (or better). Note that when dither is not used, SNR is infinite as output from a zero input signal is zero.

The performance was analyzed by converting output test files to 32 bits integer .wav files. These files were then run through an audio analysis tool (WinAudio MLS: http://www.dr-jordan-design.de/Winaudiomls.htm).

Below are a series FFT plots showing the most demanding rate conversion case. These clearly show that the above targets are comfortably exceeded. All outputs have been generated using 8192 samples at input sampling rate. A Kaiser-Bessel window with alpha=7 has been used.


.. figure:: images/asrc_fft_44_192.pdf
   :width: 90%

   FFT of 1kHz sine, 0dB, 44.1kHz to 192kHz  

.. figure:: images/asrc_fft_176_48.pdf
   :width: 90%

   FFT of 1kHz sine, 0dB, 176.4kHz to 48kHz 

.. figure:: images/asrc_fft_96_88.pdf
   :width: 90%

   FFT of 10kHz+11kHz sines, -6dB, 96kHz to 88.2kHz

|newpage|

SRC Implementation
------------------

The SSRC and ASRC implementations are closely related to each other and share the majority of the system building blocks. The key difference between them is that SSRC uses fixed polyphase 160:147 and 147:160 final rate change filters whereas the ASRC uses an adaptive polyphase filter. The ASRC adaptive polyphase coefficients are computed for every sample using second order spline based interpolation.

SRC Nominal Rate Changes
........................

The nominal rate change ratios between 44.1KHz and 192KHz are shown in the below table.

.. list-table:: Rate Changes for Sample Rate Conversion
     :header-rows: 2

     * - 
       - Output sample rate
       -
       -
       -
       -
       -
     * - Input sample rate
       - 44.1KHz
       - 48KHz
       - 88.2KHz
       - 96KHz
       - 176.4KHz
       - 192KHz
     * - 44.1KHz
       - 1
       - 160/147
       - 2
       - 2x160/147
       - 4
       - 4x160/147
     * - 48KHz
       - 147/160
       - 1
       - 2x147/160
       - 2
       - 4x147/160
       - 4
     * - 88.2KHz
       - 1/2
       - 1/2x160/147
       - 1
       - 160/147
       - 2
       - 2x160/147
     * - 96KHz
       - 1/2x147/160
       - 1/2
       - 147/160
       - 1
       - 2x147/160
       - 2
     * - 176.4KHz
       - 1/4
       - 1/4x160/147
       - 1/2
       - 1/2x160/147
       - 1
       - 160/147
     * - 192KHz
       - 1/4x147/160
       - 1/4
       - 1/2x147/160
       - 1/2
       - 147/160
       - 1


.. tip::
  The table shows the case for SSRC where the ratios are equal to the nominal sample rate ratio. In the case of ASRC, where the ratios cannot be expressed rationally, these are the nominal ratios from which there will usually be a rate deviaton. 


SSRC Structure
..............

The SSRC algorithm is based on three cascaded FIR filter stages (F1, F2 and F3). These stages are configured differently depending on rate change and only part of them is used in certain cases. The following diagram shows an overall view of the SSRC algorithm:

.. figure:: images/ssrc_structure.pdf
   :width: 90%

   SSRC Algorithm Structure

The SSRC algorithm is implemented as a two stage structure:

 * The Bandwidth control stage which includes filters F1 and F2 is responsible for limiting the bandwidth of the input signal and for providing integer rate Sample Rate Conversion. It is also used for signal conditioning in the case of rational, non-integer, Sample Rate Conversion.
 * The Polyphase filter stage which effectively converts between the 44.1kHz and the 48kHz families of sample rates.

|newpage|

ASRC Structure
..............

Similar to the SSRC, the ASRC algorithm is based three cascaded FIR filters (F1, F2 and F3). These are configured differently depending on rate change and F2 is not used in certain rate changes. The following diagram shows an overall view of the ASRC algorithm:

.. figure:: images/asrc_structure.pdf
   :width: 90%

   ASRC Algorithm Structure

The ASRC algorithm is implemented as a two stage structure:

 * The Bandwidth control stage includes filters F1 and F2 which are responsible for limiting the bandwidth of the input signal (to min(Fsin/2,Fsout/2) and for providing integer rate sample rate conversion to condition the input signal for the adaptive polyphase stage (F3). 
 * The polyphase filter stage consists of the adaptive polyphase filter F3, which effectively provides the asynchronous connection between the input and output clock domains.


SRC Filter list
...............

A complete list of the filters supported by the SRC library, both SSRC and ASRC, is shown in the below table. The filters are implemented in C within the ``FilterDefs.c`` function and the coefficients can be found in the ``/FilterData`` folder. The particular combination of filters cascaded together for a given sample rate change is specified in ``ssrc.c`` and ``asrc.c``. 

.. list-table:: SSRC Processor Usage per channel (MHz)
     :header-rows: 1

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


|newpage|

SRC File Structure and Overview
-------------------------------

 * ssrc_wrapper.c / ssrc_wrapper.h

   These wrapper files provide a simplified public API to the SSRC initialization and processing functions. 


 * asrc_wrapper.c / asrc_wrapper.h

   These wrapper files provide a simplified public API to the ASRC initialization and processing functions. 


 * SSRC.c / SSRC.h

   These files contain the core of the SSRC algorithm. It sets up the correct filtering chains depending on rate change and applies them in the processing calls. The table sFiltersIDs declared in SSRC.c contains definitions of the filter chains for all supported rated changes. The files also integrate the code for the optional dithering function.


 * ASRC.c / ASRC.h

   These files contain the core of the ASRC algorithm. They setup the correct filtering chains depending on rate change and apply them for the corresponding processing calls. Note that filters F1, F2 and dithering are implemented using a block based approach (code similar to SSRC). The adaptive polyphase filter (ADFIR) is implemented on a sample by sample basis. These files also contain functions to compute the adaptive poly-phase filter coefficients.


 * FIR.c / FIR.h

   These files provide Finite Impulse Response (FIR) filtering setup, with calls to the  assembler-optimized inner loops. It provides functions for handling down-sampling by 2, synchronous or over-sampling by 2 FIRs. It also provides functions for handling polyphase filters used for rational ratio rate change in the SSRC and adaptive FIR filters used in the asynchronous section of the ASRC.


 * FilterDefs.c / FilterDefs.h

   These files define the size and coefficient sources for all the filters used by the SRC algorithms. 


 * /FilterData directory (various files)

   This directory contains the pre-computed coefficients for all of the fixed FIR filters. The numbers are stored as signed Q1.31 format and are directly included into the source from FilterDefs.c. Both the .dat files used by the C compiler and the .sfp ScopeFIR (http://iowegian.com/scopefir/) design source files, used to originally create the filters, are included.


 * fir_inner_loop_asm.S / fir_inner_loop_asm.h

   Inner loop for the standard FIR function optimized for double-word load and store, 32bit * 32bit -> 64bit MACC and saturation instructions. Even and odd sample long word alignment versions are provided.


 * fir_os_inner_loop_asm.S / fir_os_inner_loop_asm.h

   Inner loop for the oversampling FIR function optimized for double-word load and store, 32bit * 32bit -> 64bit MACC and saturation instructions. Both (long word) even and odd sample input versions are provided.


 * spline_coeff_gen_inner_loop_asm.S / spline_coeff_gen_inner_loop_asm.h

   Inner loop for generating the spline interpolated coefficients. This assembler function is optimized for double-word load and store, 32bit * 32bit -> 64bit MACC and saturation instructions. 


 * adfir_inner_loop_asm.S / adfir_inner_loop_asm.h

   Inner loop for the adaptive FIR function using the previously computed spline interpolated coefficients. It is optimized for double-word load and store, 32bit * 32bit -> 64bit MACC and saturation instructions. Both (long word) even and odd sample input versions are provided.


 * IntArithmetic.c / IntArithmetic.h

   These files contain simulation implementations of following XMOS assembler instructions. These are only used for dithering functions, and may be eliminated during future optimizations.

|newpage|

SSRC API
--------

All public SSRC functions are prototyped within the ``src.h`` header::

  #include <src.h>

Please ensure that you have reviewed the settings within ``src_config.h`` and they are correct for your application. The default settings allow for any input/output ratio between 44.1KHz and 192KHz.

You will also have to add ``lib_src`` to the
``USED_MODULES`` field of your application Makefile.

Initialization
..............

.. doxygenfunction:: ssrc_init


SSRC Processing
...............

.. doxygenfunction:: ssrc_process

|newpage|


ASRC API
--------

Initialization
..............

.. doxygenfunction:: asrc_init


ASRC Processing
...............

.. doxygenfunction:: asrc_process


|appendix|

Known Issues
------------

Certain ASRC configurations, mainly conversions between 176.4/192KHz to 176.4/192KHz, require greater than 100MHz for a single audio channel and so cannot currently be run in real time on a single core. The performance limit for a single core on a 500MHz xCORE-200 device is 100MHz (500/5), due to a 5 stage pipeline. A number of potential optimizations have been identified to permit these rates:

* Further inner loop optimization using assembler
* Increase in scope of assembler sections removing additional function calls
* Pipelining of the FIR filter stages into separate tasks
* Calculation of adaptive filter coefficients in a separate task 

These optimizations may be the target for future revisions of this library.

.. include:: ../../../CHANGELOG.rst
