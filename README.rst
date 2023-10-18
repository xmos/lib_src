Sample Rate Conversion Library
==============================

Overview
--------

The XMOS Sample Rate Conversion (SRC) library provides both synchronous and asynchronous audio sample rate conversion functions for use on xCORE multicore micro-controllers.

In systems where the rate change is exactly equal to the ratio of nominal rates, synchronous sample rate conversion (SSRC) provides efficient and high performance rate conversion. Where the input and output rates are not locked by a common clock or clocked by an exact rational frequency ratio, the Asynchronous Sample Rate Converter (ASRC) provides a way of streaming high quality audio between the two different clock domains, at the cost of higher processing resource usage. ASRC can ease interfacing in cases where there are multiple digital audio inputs or allow cost saving by removing the need for physical clock recovery using a PLL.

Features
........

Multi-rate Hi-Fi functionality:

 * Conversion between 44.1, 48, 88.2, 96, 176.4 and 192 KHz input and output sample rates.
 * 32 bit PCM input and output data in Q1.31 signed format.
 * Optional output dithering to 24 bit using Triangular Probability Density Function (TPDF).
 * Optimized for xCORE-200 instruction set with dual-issue and compatible with XCORE-AI.
 * Block based processing - Minimum 4 samples input per call, must be power of 2.
 * Up to 10000 ppm sample rate ratio deviation from nominal rate (ASRC only).
 * Very high quality - SNR greater than 135 dB (ASRC) or 140 dB (SSRC), with THD of less than 0.0001% (reference 1KHz).
 * Configurable number of audio channels per SRC instance.
 * Reentrant library permitting multiple instances with differing configurations and channel count.
 * No external components (PLL or memory) required.

Fixed factor functionality:

 * Synchronous fixed factor of 3 downsample and oversample functions supporting either HiFi quality or reduced resource requirements for voice applications.
 * Synchronous fixed factor of 3 and 3/2 downsample and oversample functions for voice applications optimized for the XS3 Vector Processing Unit.

Building
........

The library can be built under `cmake` or `xcommon` via `xmake` offering backwards compatibility for legacy applications.
It is recommended to use `cmake` where the library name `lib_src` is included in the cmake files. See `Related application notes`_ for example usage. 
The library has no dependencies when building under `cmake` although does require `lib_logging` and `lib_xassert` when using `xcommon`. 

Components
..........

 * Synchronous Sample Rate Converter function (ssrc)
 * Asynchronous Sample Rate Converter function (asrc)

 * Synchronous factor of 3 downsample function (ds3)
 * Synchronous factor of 3 oversample function (os3)

 * Synchronous factor of 3 downsample function optimized for use with voice (src_ds3_voice)
 * Synchronous factor of 3 oversample function optimised for use with voice (src_us3_voice)

 * Synchronous factor of 3 downsample function for use with voice optimized for XS3 (ff3_96t_ds)
 * Synchronous factor of 3 oversample function for use with voice optimized for XS3 (ff3_96t_us)

 * Synchronous factor of 3/2 downsample function for use with voice optimized for XS3 (rat_2_3_96t_ds)
 * Synchronous factor of 3/2 oversample function for use with voice optimized for XS3 (rat_3_2_96t_us)

There are three different component options that support fixed factor of 3 up/downsampling. To help choose which one to use follow these steps:

 #. If HiFi quality (130 dB SNR) up/downsampling is required, use ds3 or os3.
 #. If voice quality (65 dB SNR) is required running on xCORE-200, use ds3_voice or us3_voice.
 #. If voice quality (75 dB SNR) is required running xcore-ai, use ff3_96t_ds or ff3_96t_us.

.. warning::
    Synchronous fixed factor of 3 and 3/2 downsample and oversample functions for voice applications optimized for the XS3 Vector Processing Unit
    currently overflow rather than saturate in cases where a full scale input causes a perturbation above full scale at the output.
    To avoid this scenario, please ensure that the input amplitude is always 3.5 dB below full scale.
    The overflow behavior of these SRC components will be replaced by saturating behavior (to match all other SRC components) in a future release.


Related Application Notes
.........................

An adaptive USB Audio ASRC example can be found in https://github.com/xmos/sln_voice/tree/develop/examples.

Simple file-based test applications may be found in this repo under `tests/xxxx_test` where xxxx is either `asrc`, `ssrc`, `ds3`, `os3`, `ds3_voice`, `os3_voice`, `vpu_ff3` or `vpu_rat`.
These test applications may be used as basic examples to show how these components are used in a minimal application.