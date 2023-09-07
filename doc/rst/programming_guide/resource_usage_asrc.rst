The ASRC algorithm also runs a series of cascaded FIR filters to perform the rate conversion. The final filter is different because it uses adaptive coefficients to handle the varying rate change between the input and the output. The adaptive coefficients must be computed for each output sample period, but can be shared amongst all channels within the ASRC instance. Consequently, the MHz usage of the ASRC is expressed as two tables; :numref:`fig_asrc_mhz` quantifies the MHz required for the first channel with adaptive coefficients calculation and :numref:`fig_asrc_mhz2` specifies the MHz required for filtering of each additional channel processed by the ASRC instance.

.. tip::
  The below tables show the worst case  MHz consumption per sample, using the minimum block size of 4 input samples. The  MHz requirement can be reduced by around 8-12% by increasing the input block size to 16.

.. tip::
  Typically some performance headroom is needed for buffering (especially if the system is sample orientated rather than block orientated) and inter-task communication.

.. _fig_asrc_mhz:
.. list-table:: ASRC Processor Usage ( MHz) for the First Channel in the ASRC Instance
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
       - 29 MHz
       - 30 MHz
       - 40 MHz
       - 42 MHz
       - 62 MHz
       - 66 MHz
     * -
       - 48 kHz
       - 33 MHz
       - 32 MHz
       - 42 MHz
       - 43 MHz
       - 63 MHz
       - 66 MHz
     * -
       - 88.2 kHz
       - 47 MHz
       - 50 MHz
       - 58 MHz
       - 61 MHz
       - 80 MHz
       - 85 MHz
     * -
       - 96 kHz
       - 55 MHz
       - 51 MHz
       - 67 MHz
       - 64 MHz
       - 84 MHz
       - 87 MHz
     * - 
       - 176.4 kHz
       - 60 MHz
       - 66 MHz
       - 76 MHz
       - 81 MHz
       - 105 MHz
       - 106 MHz
     * -
       - 192 kHz
       - 69 MHz
       - 66 MHz
       - 82 MHz
       - 82 MHz
       - 109 MHz
       - 115 MHz

.. caution:: Configurations requiring more than 100 MHz may not be able run in real time on a single logical core. The performance limit for a single core on a 500 MHz xCORE-200 device is 100 MHz (500/5) however an XCORE-AI device running at 600 MHz can provide 120 MHz logical cores.

.. _fig_asrc_mhz2:
.. list-table:: ASRC Processor Usage (MHz) for Subsequent Channels in the ASRC Instance
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
       - 28 MHz
       - 28 MHz
       - 32 MHz
       - 30 MHz
       - 40 MHz
       - 40 MHz
     * - 
       - 48 kHz
       - 39 MHz
       - 31 MHz
       - 33 MHz
       - 36 MHz
       - 40 MHz
       - 45 MHz
     * -
       - 88.2 kHz
       - 51 MHz
       - 49 MHz
       - 57 MHz
       - 55 MHz
       - 65 MHz
       - 60 MHz
     * -
       - 96 kHz
       - 51 MHz
       - 56 MHz
       - 57 MHz
       - 62 MHz
       - 66 MHz
       - 71 MHz
     * -
       - 176.4 kHz
       - 60 MHz
       - 66 MHz
       - 76 MHz
       - 79 MHz
       - 92 MHz
       - 91 MHz
     * -
       - 192 kHz
       - 69 MHz
       - 66 MHz
       - 76 MHz
       - 82 MHz
       - 90 MHz
       - 100 MHz
