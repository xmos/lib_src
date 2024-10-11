.. _asrc_latency_header:


The ASRC algorithm runs a series of cascaded FIR filters to perform the rate conversion including a final adaptive filter to handle the varying rate change between the input and the output. The latency or group delay through the filter depends on the input rate and output rate and the input block size. :numref:`fig_asrc_delay` quantifies the amount of latency in milliseconds seen from the filtering. For input block sizes of greater than four please add `1000 * (INPUT_BLOCK_SIZE - 4) / INPUT_SAMPLE_RATE` milliseconds to the numbers in the table.

|beginfullwidth|

.. _fig_asrc_delay:
.. list-table:: ASRC total filter delay by sample rate in milliseconds for input block size of four
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
       - 0.907
       - 0.899
       - 0.896
       - 0.899
       - 0.896
       - 0.894
     * -
       - 48 kHz
       - 0.910
       - 0.833
       - 0.830
       - 0.823
       - 0.825
       - 0.823
     * -
       - 88.2 kHz
       - 0.907
       - 0.908
       - 0.454
       - 0.450
       - 0.448
       - 0.450
     * -
       - 96 kHz
       - 0.931
       - 0.833
       - 0.455
       - 0.417
       - 0.415
       - 0.411
     * - 
       - 176.4 kHz
       - 0.998
       - 0.996
       - 0.317
       - 0.319
       - 0.159
       - 0.157
     * -
       - 192 kHz
       - 0.998
       - 0.917
       - 0.295
       - 0.292
       - 0.148
       - 0.146

|endfullwidth|

