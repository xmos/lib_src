The SSRC algorithm runs a series of cascaded FIR filters to perform the rate conversion. This includes interpolation, decimation and bandwidth limiting filters with a final polyphase FIR filter. The last stage supports the rational rate change of 147:160 or 160:147 allowing conversion between 44.1KHz family of sample rates to the 48KHz family of sample rates.

.. tip::
  The below table shows the worst case MHz consumption at a given sample rate using the minimum block size of 4 input samples with dithering disabled. The MHz requirement can be reduced by around 8-12%, depending on sample rate, by increasing the input block size to 16. It is not usefully reduced by increasing block size beyond 16.

.. list-table:: SSRC Processor Usage per Channel (MHz)
     :header-rows: 1

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


