SPDIF Receive to I2S output using Asynchronous Sample Rate Conversion
=====================================================================

.. |i2s| replace:: I\ :sup:`2`\ S
.. |i2c| replace:: I\ :sup:`2`\ C

Summary
-------

The XMOS Asynchronous Sample Rate Conversion (ASRC) library allows audio to be streamed between systems running with asynchronous sample rates while maintaining "Hi-Res" audio quality. Practical examples where ASRC can provide significant benefits include:

- Allow systems to receive and mix multiple digital audio streams
- Remove the cost and hardware complexity of audio clock recovery using a PLL
- Allow systems to use the highest possible sample rate rather than the lowest common denominator
- Addition of interfaces and processing to existing audio architectures

In addition, the ASRC can perform major rate changes between the standard audio sample rates of 44.1, 48, 88.2, 96, 176.4 and 192KHz.

This application note demonstrates the use of the XMOS Sample Rate Conversion library in a system that receives samples from an SPDIF input and outputs them to a DAC over |i2s|. The |i2s| subsystem uses a fixed frequency local master clock oscillator and the SPDIF clock is encoded within the received stream. This means that, even if operating at the same nominal sample rate, the clocks will not be synchronized and consequently ASRC is required to properly stream audio between these two audio interfaces.

The sample rates supported for this demonstration are 44.1, 48, 88.2 and 96KHz. This restriction is applied by the SPDIF receiver which is only rated up to 96KHz for optical sources and the ASRC library which can currently only handle rates of 176.4/192KHz on one side. The restricted sample rates also allow significant chip resource usage optimization which is discussed later within this application note.

Using the XMOS |i2s| and SPDIF libraries this application note demonstrates practical usage of the ASRC library. The software includes additional functions to handle the serial to block conversion of the audio samples and a precise rate monitoring servo task which is required to calculate the fractional frequency ratios for the ASRC processing function.

Required tools and libraries
............................

 * xTIMEcomposer Tools - Version 14.2.0 or higher
 * XMOS |i2s|/TDM library - Version 2.1.0
 * XMOS GPIO library - Version 1.0.0
 * XMOS |i2c| library - Version 3.1.0
 * XMOS SPDIF library - Version 2.0.2
 * XMOS Logging library - Version 2.0.0
 * XMOS Assertion library - Version 2.0.0


Required hardware
.................

The example code provided with the application has been implemented and tested on the xCORE-200 Multichannel Audio Platform.

Prerequisites
..............

 * This document assumes familiarity with |i2s| and SPDIF interfaces, the principles of ASRC and a basic understanding of the XMOS xCORE architecture, the XMOS tool chain and the xC language. Documentation related to these aspects which are not specific to this application note are linked to in the references appendix.

 * For a description of XMOS related terms found in this document please see the XMOS Glossary [#]_.

.. [#] http://www.xmos.com/published/glossary
