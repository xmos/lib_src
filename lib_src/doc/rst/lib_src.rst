.. include:: ../../../README.rst

Usage
-----

Both SSRC and ASRC functions are accessed via a standard function call, making them accessible from C or XC. To support multi-core operation, where multiple channels can be processed by more than one logical core concurrently, both SSRC and ASRC functions are passed an external state structure which provides re-entrancy.

Normally, each instance of SRC is mapped a to a logical core to provide guaranteed performance. The number of channels that that instance can handle depends on the same rate and available MHz allocated to that core. The logical cores that execute the SRC processing may exist on separate tiles. In this case, total chip memory usage will be increased due to the need of the compiler to replicate constants  such as coefficient storage on multiple tiles.

The API is designed to be as simple and intuitive with just two public functions per sample rate converter type.


Initialization
..............

There is an initialization call which sets up the variables associated with the SRC instance and clears any buffers. It is necessary to ensure the correct selection and ordering and configuration of the filtering stages, be they decimators, interpolators or pass through blocks. This initialization call contains arguments defining selected input and output nominal sample rates as well as settings for the sample rate converter. The settings include:

 * The number of channels handled by this instance of SRC
 * The number of input samples to expect. Minimum 4 samples input per call, must be power of 2
 * The dither setting. Dithers the output from 32bit to 24bit

The input block size must be a power of 2 and is set by ``SRC_N_IN_SAMPLES``. In the case where n channels are to be processed per SRC instance, the total number of input samples expected for each processing call is ``SRC_N_IN_SAMPLES * SRC_CHANNELS_PER_INSTANCE``. The processing call includes references to the input and output arrays, which contain 32bit signed Q1.31 audio samples.


Processing
..........

Following initialization, the processing API is call for each block of input samples. The logic is designed so that the final filtering stage always receives a sample to process. The sample rate converters have been designed to handle a maximum decimation of factor from the first two stages of four. This architecture requires a minimum input block size of 4 to operate.

The processing function call always returns a whole number of output samples produced by the sample rate conversion. This number may be between zero and ``(SRC_N_IN_SAMPLES  * SRC_CHANNELS_PER_INSTANCE * SRC_N_OUT_IN_RATIO_MAX)``, depending on the sample ratios selected. The fractional number of samples produced to be carried to the next operation are stored internally inside the control structure, and additional whole samples are added or subtracted from subsequent calls to the sample rate converter.

For example, a sample rate conversion from 44.1KHz to 48KHz with a input block size of 4 will produce a 4 sample result with a 5 sample result approximately every third call.

Each SRC processing call returns the integer number of samples produced during the sample rate conversion.

The SSRC is synchronous in nature and assumes that the long term average ratio between input and output rates can be expressed as a rational number. For example, to convert from 44.1KHz to 48KHz, it is assumed that the word clocks of the input and output stream are derived from the same master clock and have an exact ratio of 147:160. If the word clocks are derived from separate oscillators, or are not synchronous (for example are derived from each other using a fractional PLL), the the ASRC must be used to avoid buffer over/under-runs.

Buffer Formats
..............

The format of the sample buffers sent and received from each SRC instance is time domain interleaved. How this looks in practice depends on the number of channels and SRC instances. Three examples are shown below, each showing ``SRC_N_IN_SAMPLES = 1``.

In the case where two channels are handled by a single SRC instance, you can see that the samples are interleaved into a single big buffer.

.. figure:: images/stereo_single_instance.pdf
   :width: 25%

   Buffer Format for Single Stereo SRC instance 


Where a single audio channel is mapped to a single instance, the buffers are simply an array of samples starting with the oldest sample and ending with the newest sample.

.. figure:: images/stereo_dual_instance.pdf
   :width: 70%

   Buffer Format for Dual Mono SRC instances 


|newpage|

In the case where four channels are processed by two instances, channels 0 & 1 are processed by SRC instance 0 and channels 2 & 3 are processed by SRC instance 1. For each instance, four pairs of samples are passed into the SRC processing function and n pairs of samples are returned, where n depends on the input and output sample rate ratio.

.. figure:: images/quad_dual_instance.pdf
   :width: 80%

   Buffer Format for Dual Stereo SRC instances (4 channels total)  

In addition to the above arguments the ``asrc_process()`` call also requires an unsigned Q4.28 fixed point ratio value specifying the actual input to output ratio for the next calculated block of samples. This allows the input and output rates to be fully asynchronous by allowing rate changes on each call to the ASRC. The converter dynamically computes coefficients using a spline interpolation within the last filter stage. It is up to the callee to maintain the input and output sample rate ratio difference. An example of this calculation, based on measuring the input and output rates, is provided in AN00231.

Further detail about these function arguments are contained within the API section of this guide. 


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


SSRC processing
...............

.. doxygenfunction:: ssrc_process

|newpage|


ASRC API
--------

Initialization
..............

.. doxygenfunction:: asrc_init


ASRC processing
...............

.. doxygenfunction:: asrc_process


|appendix|

Known Issues
------------

The logic that decides on the correct FIR functions to call is implemented using function pointers. The current version of tools at the time of authoring (14.1.2) has limited support for function pointers and is unable to correctly resolve memory usage. A workaround is included using the ``#pragma stackfunction`` pragma which allows memory usage to be reconciled, but results in a "Maybe" status result in the resource usage report. Improved support for function pointers is planned in future versions of the tools and will address the memory usage reporting issue. This issue does not affect the robustness or performance of the library, only the reported resource usage.

Certain ASRC configurations, mainly conversions between 176.4/192KHz to 176.4/192KHz, require greater than 100MHz for a single audio channel and so cannot currently be run in real time on a single core. The performance limit for a single core on a 500MHz xCORE-200 device is 100MHz (500/5), due to a 5 stage pipeline. A number of potential optimizations have been identified to permit these rates:

* Further inner loop optimization using assembler
* Increase in scope of assembler sections removing additional function calls
* Pipelining of the FIR filter stages into separate tasks
* Calculation of adaptive filter coefficients in a seperate task 

These optimisations may be the target for future revisions of this library.

.. include:: ../../../CHANGELOG.rst
