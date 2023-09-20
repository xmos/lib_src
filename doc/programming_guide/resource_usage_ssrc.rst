The SSRC algorithm runs a series of cascaded FIR filters to perform the rate conversion. This includes interpolation, decimation and bandwidth limiting filters with a final polyphase FIR filter. The last stage supports the rational rate change of 147:160 or 160:147 allowing conversion between 44.1 kHz family of sample rates to the 48 kHz family of sample rates.

.. tip::
  :numref:`fig_ssrc_mhz` shows the worst case  MHz consumption at a given sample rate using the minimum block size of 4 input samples with dithering disabled. The MHz requirement can be reduced by around 8-12%, depending on sample rate, by increasing the input block size to 16. It is not usefully reduced by increasing block size beyond 16.


.. _fig_ssrc_mhz:
.. list-table:: SSRC Processor Usage per Channel (MHz)
     :header-rows: 2

     * -
       -
       - Output rate
       -
       -
       -
       -
       -
     * - 
       -
       - 44.1 kHz
       - 48 kHz
       - 88.2 kHz
       - 96 kHz
       - 176.4 kHz
       - 192 kHz
     * - Input rate
       - 44.1 kHz
       - 1 MHz
       - 23 MHz
       - 16 MHz
       - 26 MHz
       - 26 MHz
       - 46 MHz
     * -
       - 48 kHz
       - 26 MHz
       - 1 MHz
       - 28 MHz
       - 17 MHz
       - 48 MHz
       - 29 MHz
     * -
       - 88.2 kHz
       - 18 MHz
       - 43 MHz
       - 1 MHz
       - 46 MHz
       - 32 MHz
       - 53 MHz
     * -
       - 96 kHz
       - 48 MHz
       - 20 MHz
       - 52 MHz
       - 2 MHz
       - 56 MHz
       - 35 MHz
     * -
       - 176.4 kHz
       - 33 MHz
       - 61 MHz
       - 37 MHz
       - 67 MHz
       - 3 MHz
       - 76 MHz
     * -
       - 192 kHz
       - 66 MHz
       - 36 MHz
       - 70 MHz
       - 40 MHz
       - 80 MHz
       - 4 MHz


