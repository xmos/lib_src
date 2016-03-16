SPDIF Receive to I2S output using Asynchronous Sample Rate Conversion
=====================================================================

.. appnote:: AN00231

.. version:: 1.0.0

Summary
-------

The XMOS Asynchronous Sample Rate Conversion (ASRC) library allows audio to be streamed between systems running from with asynchronous while maintaining "Hi-Res" audio quality. Practical examples where ASRC can provide significant benefits include:

- Allowing systems to receive and mix multiple digital audio streams
- Removing the cost and hardware complexity of audio clock recovery using a PLL
- Allowing systems to use the highest possible sample rate rather than the lowest common denominator

In addition, the ASRC can perform major rate changes between the standard audio sample rates of 44.1, 48, 88.2, 96, 176.4 and 192KHz. 

This application note demonstrates the use of the XMOS Sample Rate Conversion library in system that receives samples from an S/PDIF and outputs them to a DAC over I2S. The I2S subsystem uses a fixed frequency local master clock oscillator and the S/PDIF clock is encoded within the received stream. This means that, even if operating at the same nominal sample rate, the clocks will not be synchronized and consequently ASRC is required to properly stream audio between these two audio interfaces. 

The sample rates supported for this demonstration are 44.1, 48, 88.2 and 96KHz. This restriction is applied by the SPDIF receiver which is only rated up to 96KHz for optical sources,and the ASRC library which can only handle 176.4/192KHz on one side currently.

Using the XMOS I2S and S/PDIF libraries this application note demonstrates use of the ASRC library, along with additional software to handle the serial to block sample and precise rate monitoring with servo, to perform high quality audio stereo sample rate conversion. 

Required tools and libraries
............................

 * xTIMEcomposer Tools - Version 14.1.2 or higher
 * XMOS I2S/TDM library - Version 2.1.0
 * XMOS GPIO library - Version 1.0.0
 * XMOS I2C library - Version 3.1.0
 * XMOS S/PDIF library - Version 2.0.0
 * XMOS Logging library - Version 2.0.0
 * XMOS Assertion library - Version 2.0.0



Required hardware
.................
The example code provided with the application has been implemented and tested on the xCORE-200 Multichannel Audio Platform.

Prerequisites
..............
 * This document assumes familiarity with I2S and SPDIF interfaces, the principals of ASRC the XMOS xCORE architecture, the XMOS tool chain and the xC language. Documentation related to these aspects which are not specific to this application note are linked to in the references appendix.

 * For a description of XMOS related terms found in this document please see the XMOS Glossary [#]_.

.. [#] http://www.xmos.com/published/glossary
