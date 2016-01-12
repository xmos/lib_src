.. include:: ../../../README.rst

Usage
-----

Both SSRC and ASRC functions are accessed via a standard function call, making them accesible from C or XC. To support multi-core operation, where multiple channels can be processed by more than one logical core concurrently, all API calls include an index to a logical processing core. The logical cores that execute the SRC processing may exist on separate tiles. In this case, memory usage is increased due to the need to replicate coefficient storage on multiple tile.

The API is designed to be as simple and intuitive with just two public functions. There is an initialisation call which sets up the variables associated with the SRC instance and clears any buffers. This initialisation call contains arguments defining selected input and output nominal sample rates. THis allows the correct selection and ordering of the filtering stages, be they decimators, interpolators or pass through blocks.

Following initialisation, the procesing API is call for each block of input samples. The logic is designed so that the final filtering stage always receives a sample to process. Since the maximum decimation of factor from the first two stage is four, a mimimum input block size of 4 is required. The input block size must be a power of 2. In the case n channels are to be processed, the total number of input samples expected for each processing call is ``n * 4``. The processing call includes references to the input and output arrays, which contain 32bit Q1.31 values. It also includes an index indicating on which logical core the function is to be executed. This allows the sample rate processing to process only the channels asigned to it. The index ranges from 0 to n-1, where n is defined as ``SRC_CHANNELS_PER_CORE``.

The processing function call always returns the number of output samples produced by the sample rate conversion. This number may be between zero and ``(SRC_N_IN_SAMPLES * SRC_N_OUT_IN_RATIO_MAX)``, depending on the sample ratios selected.

The format of the buffers sent and received from each SRC instance depends on the number of channels is shown below.

Buffer Format
.............

.. figure:: images/buffer_format.pdf
   :width: 80%

   Buffer Format

For example, in the case where four channels are processed by two instances, channels 0 & 1 are processed by instance 0 and channels 2 & 3 are processed by instance 1. For each instance, four pairs of samples are passed into the SRC processing function and n pairs of samples are returned, where n depends on the sample ratio.

In addition to requiring the input/output arrays and logical core index, the ``asrc_process()`` call requires a Q4.28 fixed point ratio value indicating the actual input to output ratio. This allows the input and output rates to be asynchronous by dynamically computing coefficients for a spline interpolation withn the last filter stage. It is up to the callee to maintain the input and output sample rate ratio differenece. An example of this calculation is provided in ANXXXX.

There are a number of static configuration parameters which are contained within the file ``src_config.h`` which control the setting for both SSRC and ASRC implementations (although these may be separated if required). These set the number of channels and processing cores to be used as well as control the inclusion of the optional dithering stage, which can reduce quantisation noise when truncating to 24b output samples. An overview of these parameters is shown below. 


.. literalinclude:: ../../../lib_src/src/src_config.h
   :start-on: //General SRC configuration defines.
   :end-before: //SSRC defines

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

.. include:: ../../../CHANGELOG.rst
