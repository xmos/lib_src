The ASRC algorithm also runs a series of cascaded FIR filters to perform the rate conversion. The final filter is different because it uses adaptive coefficients to handle the varying rate change between the input and the output. The adaptive coefficients must be computed for each output sample period, but can be shared amongst all channels within the ASRC instance. Consequently, the MHz usage of the ASRC is expressed as two tables; the first table enumerates the MHz required for the first channel with adaptive coefficients calculation and the second table specifies the MHz required for filtering of each additional channel processed by the ASRC instance.

.. tip::
  The below tables show the worst case MHz consumption per sample, using the minimum block size of 4 input samples. The MHz requirement can be reduced by around 8-12% by increasing the input block size to 16.

.. tip::
  Typically you will need to allow for performance headroom for buffering (especially if the system is sample orientated rather than block orientated) and inter-task communication. Please refer to the application notes for practical examples of usage.


.. list-table:: ASRC Processor Usage (MHz) for the First Channel in the ASRC Instance
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

.. caution:: Configurations requiring more than 100MHz cannot currently be run in real time on a single core. The performance limit for a single core on a 500MHz xCORE-200 device is 100MHz (500/5). Further optimization of the library, including assembler optimization and pipelining of the adaptive filter generation and FIR filter stages, is feasible to achieve higher sample rate operation within the constraints of a 100MHz logical core.

.. list-table:: ASRC Processor Usage (MHz) for Subsequent Channels in the ASRC Instance
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
