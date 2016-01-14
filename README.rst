XMOS Sample Rate Conversion Library
===================================

.. rheader::

   Sample Rate Convertor |version|

Overview
--------

The XMOS Sample Rate Conversion (SRC) library provides both synchronous and asynchronous audio sample rate conversion functions for use on xCORE-200 multicore microcontrollers.

In systems where there is a rational ratio between the input and output sample rates, synchronous sample rate conversion (SSRC) provides efficient and audiophile quality rate conversion. Where the input and output rates are not locked by a common clock or clocks with exact rational frequency ratios, the asynchronous sample converter (ASRC) provides a way of streaming high quality audio between the two clock domains, at the cost of higher processing resource usage. ASRC can ease interfacing in cases where the are multiple digital audio inputs or allow cost saving by removing the need for physical clock recovery using a PLL.


Features
........

 * Conversion between 44.1, 48, 88.2, 96, 176.4 and 192KHz input and output sample rates
 * 32 bit PCM data in 1.31 format
 * Optimised for xCORE-200 instruction set
 * Optional output dithering to 24 bit using Triangular Probability Density Function (TPDF)
 * No external memory required
 * Block based processing. Minimum 4 samples input, must be power of 2.
 * Up to 10000 ppm sample rate ratio deviation (ASRC only)
 * Very high quality. SNR greater than 135db (ASRC) or 140db (SSRC), with THD of less than 0.0001%

Typical Resource Usage
......................

SSRC implementations typically consume the following amount of RAM:

.. resusage::

  * - configuration: SSRC
    - target: XCORE-200-EXPLORER
    - globals: int in_buff[4]; int out_buff[20]; SSRCCtrl_t sSSRCCtrl; 
    - ports: 0
    - locals:
    - flags:
    - pins: 0
    - fn: unsafe{ssrc_process(in_buff, out_buff, &sSSRCCtrl);}

.. list-table:: SSRC Processor MHz Usage MHz
     :header-rows: 1

     * - configuration 
       - Processor usage
     * - SSRC 172.4KHz to 192KHz
       - 75MHz per channel
     * - SSRC 48KHz to 192KHz. 
       - 30MHz per channel
     * - SSRC 192KHz to 48KHz.  
       - 40MHz per channel
     * - SSRC 44.1KHz to 48KHz.  
       - 25MHz per channel


ASRC implementations typically take XXKB of code, coefficient, and buffer memory and around XBytes of stack. Specific examples are shown below.

.. resusage::

  * - configuration: ASRC
    - target: XCORE-200-EXPLORER
    - globals: int in_buff[4]; int out_buff[20]; ASRCCtrl_t sASRCCtrl[2]; iASRCADFIRCoefs_t SiASRCADFIRCoefs;
    - locals:
    - ports: 0
    - flags:
    - pins: 0
    - fn: unsafe{asrc_process(in_buff, out_buff, 0, sASRCCtrl);}

.. list-table:: ASRC Processor Usage (MHz)
     :header-rows: 1

     * - configuration 
       - Processor usage
     * - ASRC 176.4KHz to 192KHz.  
       - 95MHz per channel
     * - ASRC 48KHz to 48KHz.   
       - 25MHz per channel


Software version and dependencies
.................................

.. libdeps::

Related application notes
.........................
The following application notes use this library:
    * ANxxxx - [SPDIF to I2S Synchronous Sample Rate Conversion
    * ANxxxx - [USB Audio Synchronous Sample Rate Conversion]
    * ANxxxx - [SPDIF to I2S Asynchronous Sample Rate Conversion]
    * ANxxxx - [USB Audio with Asynchronous Sample Rate Converted SPDIF Receive]
