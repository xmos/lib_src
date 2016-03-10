.. include:: ../../../README.rst

Usage
-----

Both SSRC and ASRC functions are accessed via a standard function call, making them accesible from C or XC. To support multi-core operation, where multiple channels can be processed by more than one logical core concurrently, both SSRC and ASRC functions are passed an external state structure which provides reentrancy. Normally, each instance of SRC is mapped a to a logical core. The number of channels that that instance can handle depends on the same rate and available MHz allocated to that core. The logical cores that execute the SRC processing may exist on separate tiles. In this case, memory usage is increased due to the need to replicate coefficient storage on multiple tile.

The API is designed to be as simple and intuitive with just two public functions per sample rate converter. There is an initialisation call which sets up the variables associated with the SRC instance and clears any buffers. This initialisation call contains arguments defining selected input and output nominal sample rates, as well as settings for the sample rate converter. This function call is necessary to ensure the correct selection and ordering of the filtering stages, be they decimators, interpolators or pass through blocks.

Following initialisation, the procesing API is call for each block of input samples. The logic is designed so that the final filtering stage always receives a sample to process. The sample rate converters have been designed to handle a maximum decimation of factor from the first two stages of four. This architecture requires a mimimum input block size of 4. The input block size must be a power of 2. In the case where n channels are to be processed per SRC instance, the total number of input samples expected for each processing call is ``n * 4``. The processing call includes references to the input and output arrays, which contain 32bit signed Q1.31 audio samples.

The processing function call always returns the whole number of output samples produced by the sample rate conversion. This number may be between zero and ``(SRC_N_IN_SAMPLES * SRC_N_OUT_IN_RATIO_MAX)``, depending on the sample ratios selected. The fractional number of samples to carry to the next operation is stored internally inside the control structure.

The SSRC is synchronous in nature and assumes that the long term average ratio between input and output rates can be expressed as a rational number. For example, to convert from 44.1KHz to 48KHz, it is assumed that the word clocks of the input and output stream are derviced from the same master clock and have an exact ration of 147:160. If the word clocks are derived from seperate oscillators, or are not synchrnonous (for example are derived from each other using a fractional PLL), the the ASRC must be used to avoid buffer over/under-runs.

The format of the sample buffers sent and received from each SRC instance depends on the number of channels is shown below.

Buffer Format
.............

.. figure:: images/buffer_format.pdf
   :width: 80%

   Buffer Format

For example, in the case where four channels are processed by two instances, channels 0 & 1 are processed by instance 0 and channels 2 & 3 are processed by instance 1. For each instance, four pairs of samples are passed into the SRC processing function and n pairs of samples are returned, where n depends on the sample ratio.

In addition to requiring the input/output arrays and logical core index, the ``asrc_process()`` call requires a Q4.28 fixed point ratio value specifying the actual input to output ratio for the next calculated block of samples. This allows the input and output rates to be fully asynchronous by dynamically computing coefficients for a spline interpolation withn the last filter stage. It is up to the callee to maintain the input and output sample rate ratio differenece. An example of this calculation, based on measuring the input and output rates, is provided in ANXXXX.

There are a number of configuration parameters which are which control the setting for both SSRC and ASRC implementations. These set the number of channels and processing cores to be used as well as control the inclusion of the optional dithering stage, which can reduce quantisation noise when truncating to 24b output samples. An overview of these parameters is contained within the API section of this guide. 


|newpage|

SSRC API
--------

All public SSRC functions are prototyped within the ``src.h`` header::

  #include <src.h>

Please ensure that you have reviewed the settings within ``src_config.h`` and they are correct for your application. The default settings allow for any input/output ratio between 44.1KHz and 192KHz.

You will also have to add ``lib_src`` to the
``USED_MODULES`` field of your application Makefile.

Initialisation
..............

.. doxygenfunction:: ssrc_init


SSRC processing
...............

.. doxygenfunction:: ssrc_process

|newpage|


ASRC API
--------

Initialisation
..............

.. doxygenfunction:: asrc_init


ASRC processing
...............

.. doxygenfunction:: asrc_process


|appendix|

Known Issues
------------

The logic that decides on the correct FIR functions to call is implemented using function pointers. The current version of tools at the time of authoring (14.1.2) has limited support for function pointers and is unable to correctly resolve memory usage. A workaround is included using the ``#pragma stackfunction`` pragma which allows memory usage to be reconciled, but results in a "Maybe" status result in the resource usage report. Improved support for function pointers is planned in future versions of the tools and will address the memory usage reporting issue. This issue does not affect the robustness or performance of the library, only the reported resource usage.

Certain ASRC configurations require greater than 100MHz so cannot be run in real time on a single core, mainly conversions between 176.4/192KHz to 176.4/192KHz. The performance limit for a single core on a 500MHz xCORE-200 device is 100MHz (500/5), due to a 5 stage pipeline. A number of potential optimisations have been identified to permit these rates:

* Assembler inner loop optimisation
* Increase in scope of assembler sections removing additional function calls
* Pipelining of the FIR filter stages
* Calulation of adaptive filter coefficients in a seperate task 

.. include:: ../../../CHANGELOG.rst
