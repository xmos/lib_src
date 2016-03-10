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
 * Optimised for xCORE-200 instruction set with dual-issue
 * Optional output dithering to 24 bit using Triangular Probability Density Function (TPDF)
 * No external memory required
 * Block based processing. Minimum 4 samples input per call, must be power of 2
 * Up to 10000 ppm sample rate ratio deviation (ASRC only)
 * Very high quality. SNR greater than 135db (ASRC) or 140db (SSRC), with THD of less than 0.0001% (reference 1KHz)

Typical Resource Usage
......................

SSRC implementations typically consume the following amount of device resources:

.. resusage::

  * - configuration: SSRC
    - target: XCORE-200-EXPLORER
    - globals: int in_buff[4]; int out_buff[20]; SSRCCtrl_t sSSRCCtrl; 
    - ports: 0
    - locals:
    - flags:
    - pins: 0
    - fn: unsafe{ssrc_init(0, 0, &sSSRCCtrl);ssrc_process(in_buff, out_buff, &sSSRCCtrl);}

The ASRC algorithm runs a series of FIR filters to perform the rate conversion. This includes interpolation, decimation and bandwidth limiting filters with a final polhyphase FIR filter to support the rational rate change of 147:160 allowing conversion between 44.1KHz family of sample rates to the 48KHz family of sample rates.

.. tip::
  The below table shows the worst case MHz consumption per sample, using the minimum block size of 4 input samples. The MHz requirement can be reduced by around 8-12% by increasing the input block size to 16. 

.. list-table:: SSRC Processor Usage per channel (MHz)
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
       - 1MHz
       - 23MHz
       - 16MHz
       - 26MHz
       - 26MHz
       - 46MHz
     * - 48KHz
       - 26MHz
       - 1MHz
       - 28MHz
       - 17MHz
       - 48MHz
       - 29MHz
     * - 88.2KHz
       - 18MHz
       - 43MHz
       - 1MHz
       - 46MHz
       - 32MHz
       - 53MHz
     * - 96KHz
       - 48MHz
       - 20MHz
       - 52MHz
       - 2MHz
       - 56MHz
       - 35MHz
     * - 176.4KHz
       - 33MHz
       - 61MHz
       - 37MHz
       - 67MHz
       - 3MHz
       - 76MHz
     * - 192KHz
       - 66MHz
       - 36MHz
       - 70MHz
       - 40MHz
       - 80MHz
       - 4MHz

ASRC implementations typically consume the following amount of device resources:


.. resusage::

  * - configuration: ASRC
    - target: XCORE-200-EXPLORER
    - globals: int in_buff[4]; int out_buff[20]; ASRCCtrl_t sASRCCtrl[2]; iASRCADFIRCoefs_t SiASRCADFIRCoefs;
    - locals:
    - ports: 0
    - flags:
    - pins: 0
    - fn: unsafe{asrc_process(in_buff, out_buff, 0, sASRCCtrl);}

The ASRC algorithm runs a series of FIR filters to perform the rate conversion. The final filter uses adaptive coefficients to handle the varying rate change between the input and the output. The adaptive coefficients must be computed for each output sample, but can be shared amongst all channels. The ASRC algorithm calculates the coefficients for the first channel in each instance only. Consequently, the MHz usage of the ASRC is expressed as two tables; the first table enumerates the MHz required for the first channels and adapative coefficients calculation and the second table specifies the MHz required for each additional channel processed by the ASRC instance.

.. tip::
  The below tables show the worst case MHz consumption per sample, using the minimum block size of 4 input samples. The MHz requirement can be reduced by around 8-12% by increasing the input block size to 16. 

.. list-table:: ASRC Processor Usage (MHz) for the First Channel in the ASRC Instance
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
       - 29MHz
       - 30MHz
       - 40MHz
       - 42MHz
       - 62MHz
       - 66MHz
     * - 48KHz
       - 33MHz
       - 32MHz
       - 42MHz
       - 43MHz
       - 63MHz
       - 66MHz
     * - 88.2KHz
       - 47MHz
       - 50MHz
       - 58MHz
       - 61MHz
       - 80MHz
       - 85MHz
     * - 96KHz
       - 55MHz
       - 51MHz
       - 67MHz
       - 64MHz
       - 84MHz
       - 87MHz
     * - 176.4KHz
       - 60MHz
       - 66MHz
       - 76MHz
       - 81MHz
       - 105MHz
       - 106MHz
     * - 192KHz
       - 69MHz
       - 66MHz
       - 82MHz
       - 82MHz
       - 109MHz
       - 115MHz

.. caution:: Configurations requiring more than 100MHz cannot currently be run in real time on a single core. The performance limit for a single core on a 500MHz xCORE-200 device is 100MHz (500/5). Further optimisation of the library including assembler optimisation and pipelining of the adaptive filter generation and FIR filter stages is feasbile to achieve higher sample rare operation within the constraints of a 100MHz logical core.

.. list-table:: ASRC Processor Usage (MHz) for Subsequent Channels in the ASRC Instance
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
       - 28MHz
       - 28MHz
       - 32MHz
       - 30MHz
       - 40MHz
       - 40MHz
     * - 48KHz
       - 39MHz
       - 31MHz
       - 33MHz
       - 36MHz
       - 40MHz
       - 45MHz
     * - 88.2KHz
       - 51MHz
       - 49MHz
       - 57MHz
       - 55MHz
       - 65MHz
       - 60MHz
     * - 96KHz
       - 51MHz
       - 56MHz
       - 57MHz
       - 62MHz
       - 66MHz
       - 71MHz
     * - 176.4KHz
       - 60MHz
       - 66MHz
       - 76MHz
       - 79MHz
       - 92MHz
       - 91MHz
     * - 192KHz
       - 69MHz
       - 66MHz
       - 76MHz
       - 82MHz
       - 90MHz
       - 100MHz

Software version and dependencies
.................................

.. libdeps::

Related application notes
.........................
The following application notes use this library:
    * AN00230 - [Adding Synchronous Sample Rate Conversion to the USB Audio reference design]
    * AN00231 - [SPDIF receive to I2S output using Asynchronous Sample Rate Conversion]
