:orphan:

###############################
lib_src: Sample Rate Conversion
###############################

:vendor: XMOS
:version: 2.5.0
:scope: General Use
:description: SSRC/ASRC for xcore
:category: Audio
:keywords: SRC, SSRC, ASRC, DSP
:devices: xcore.ai, xcore-200

Summary
*******

The XMOS Sample Rate Conversion (SRC) library provides both synchronous and asynchronous audio
sample rate conversion functions for use on xcore multicore micro-controllers.

In systems where the rate change is exactly equal to the ratio of nominal rates, synchronous sample rate conversion (SSRC) provides efficient and high performance rate conversion. Where the input and output rates are not locked by a common clock or clocked by an exact rational frequency ratio, the Asynchronous Sample Rate Converter (ASRC) provides a way of streaming high quality audio between the two different clock domains, at the cost of higher processing resource usage. ASRC can ease interfacing in cases where there are multiple digital audio inputs or allow cost saving by removing the need for physical clock recovery using a PLL.

Features
********

  * Multi-rate Hi-Fi functionality:
    * Synchronous Sample Rate Converter function (SSRC)
    * Asynchronous Sample Rate Converter function (ASRC)
    * Conversion between 44.1, 48, 88.2, 96, 176.4 and 192 KHz input and output sample rates.
    * 32 bit PCM input and output data in Q1.31 signed format.
    * Optional output dithering to 24 bit using Triangular Probability Density Function (TPDF).
    * Optimized for xCORE-200 instruction set with dual-issue and for the Vector Processing Unit for xcore.ai.
    * Block based processing - Minimum 4 samples input per call, must be power of 2.
    * Up to 10000 ppm sample rate ratio deviation from nominal rate (ASRC only).
    * Very high quality - SNR greater than 135 dB (ASRC) or 140 dB (SSRC), with THD of less than 0.0001% (reference 1KHz).
    * Configurable number of audio channels per SRC instance.
    * Reentrant library permitting multiple instances with differing configurations and channel count.
    * No external components (PLL or memory) required.
  * Fixed factor functionality:
    * Synchronous fixed factor of 3 downsample and oversample functions supporting either HiFi quality
      or reduced resource requirements for voice applications.
    * Synchronous fixed factor of 3 and 3/2 downsample and oversample functions for voice applications
      optimized for the XS3 Vector Processing Unit.
  * Asynchronous FIFO:
    * Non-blocking thread-safe FIFO for use with ASRC systems.
    * Built in phase detector and control loop to determine the ASRC conversion ratio
    * Enables practical audio systems to be constructed using multiple clock domains and sample rates

Known Issues
************

  * Synchronous fixed factor of 3 and 3/2 downsample and oversample functions for voice applications optimized for the XS3 Vector Processing Unit
    currently overflow rather than saturate in cases where a full scale input causes a perturbation above full scale at the output.
    To avoid this scenario, please ensure that the input amplitude is always 3.5 dB below full scale.
    The overflow behavior of these SRC components will be replaced by saturating behavior (to match all other SRC components) in a future release.

Required Tools
**************

  * XMOS XTC Tools: 15.3.0

Required Libraries (dependencies)
*********************************

  * lib_logging (www.github.com/xmos/lib_logging)

Related Application Notes
*************************

The following application notes use this library:

  * `AN02003: SPDIF/ADAT/I2S Receive to I2S Slave Bridge with ASRC <https://www.xmos.com/file/an02003>`_

Support
*******

This package is supported by XMOS Ltd. Issues can be raised against the software at: http://www.xmos.com/support

