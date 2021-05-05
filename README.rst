Sample Rate Conversion Library
==============================

.. |i2s| replace:: I\ :sup:`2`\ S
.. |i2c| replace:: I\ :sup:`2`\ C


Overview
--------

The XMOS Sample Rate Conversion (SRC) library provides both synchronous and asynchronous audio sample rate conversion functions for use on xCORE-200 multicore micro-controllers.

In systems where the rate change is exactly equal to the ratio of nominal rates, synchronous sample rate conversion (SSRC) provides efficient and high performance rate conversion. Where the input and output rates are not locked by a common clock or clocked by an exact rational frequency ratio, the Asynchronous Sample Rate Converter (ASRC) provides a way of streaming high quality audio between the two different clock domains, at the cost of higher processing resource usage. ASRC can ease interfacing in cases where the are multiple digital audio inputs or allow cost saving by removing the need for physical clock recovery using a PLL.

Features
........

 * Multi-rate Hi-Fi functionality:

   * Conversion between 44.1, 48, 88.2, 96, 176.4 and 192KHz input and output sample rates
   * 32 bit PCM input and output data in Q1.31 signed format
   * Optional output dithering to 24 bit using Triangular Probability Density Function (TPDF)
   * Optimized for xCORE-200 instruction set with dual-issue
   * Block based processing - Minimum 4 samples input per call, must be power of 2
   * Up to 10000 ppm sample rate ratio deviation from nominal rate (ASRC only)
   * Very high quality - SNR greater than 135db (ASRC) or 140db (SSRC), with THD of less than 0.0001% ( reference 1KHz)
   * Configurable number of audio channels per SRC instance
   * Reentrant library permitting multiple instances with differing configurations and channel count

 * Synchronous fixed factor of 3 downsample and oversample functions with reduced resource requirements
 * No external components (PLL or memory) required

Components
..........

 * Synchronous Sample Rate Converter function
 * Asynchronous Sample Rate Converter function
 * Synchronous factor of 3 downsample function
 * Synchronous factor of 3 oversample function
 * Synchronous factor of 3 downsample function optimised for use with voice


Software version and dependencies
.................................

The CHANGELOG contains information about the current and previous versions.
For a list of direct dependencies, look for DEPENDENT_MODULES in lib_src/module_build_info.

Related application notes
.........................
The following application notes use this library:

    * AN00230 - [Adding Synchronous Sample Rate Conversion to the USB Audio reference design]
    * AN00231 - [SPDIF receive to |i2s| output using Asynchronous Sample Rate Conversion]

