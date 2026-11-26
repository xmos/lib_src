The ASRC uses cascaded FIR filters and computes adaptive coefficients for the
final filter. These coefficients are shared between the channels of an ASRC
instance. Consequently, each additional channel generally requires less
processing than the first, because coefficient calculation is not repeated.
For the :ref:`ASRC task <asrc_task_header>`, this is estimated to reduce the
processing requirement by about 10% per additional channel per worker thread.

The tables show the peak estimated MHz per channel averaged over a two-channel
instance; they do not separately measure the first channel or the cost of an
additional channel. For each rate pair, the largest result from ratio deviations
0.990099, 1.000000 and 1.009999 is shown.

Measurements use the simulator, four input samples per block, 256 input samples
per channel and dithering disabled. The estimates use a 100 MHz reference timer,
a 600 MHz processor and five logical cores; values are rounded up to whole MHz.
Scalar results are measured on XS3 and are indicative, not exact VX4 scalar
measurements. Allow additional headroom for buffering and communication.

.. BEGIN GENERATED TABLES

.. _fig_asrc_mhz:

.. list-table:: ASRC usage per channel (MHz): Scalar (xcore-200, XS3 and VX4 without VPU; XS3 measurements)
   :header-rows: 1
   :width: 100%
   :widths: 22 13 13 13 13 13 13

   * - Input / output rate
     - 44.1 kHz
     - 48 kHz
     - 88.2 kHz
     - 96 kHz
     - 176.4 kHz
     - 192 kHz
   * - 44.1 kHz
     - 30
     - 30
     - 37
     - 37
     - 52
     - 54
   * - 48 kHz
     - 32
     - 32
     - 38
     - 40
     - 52
     - 56
   * - 88.2 kHz
     - 50
     - 50
     - 59
     - 59
     - 74
     - 74
   * - 96 kHz
     - 54
     - 54
     - 63
     - 64
     - 76
     - 80
   * - 176.4 kHz
     - 69
     - 69
     - 80
     - 80
     - 100
     - 100
   * - 192 kHz
     - 71
     - 75
     - 79
     - 87
     - 100
     - 109

.. _fig_asrc_xs3a_vpu_mhz:

.. list-table:: ASRC usage per channel (MHz): XS3 VPU
   :header-rows: 1
   :width: 100%
   :widths: 22 13 13 13 13 13 13

   * - Input / output rate
     - 44.1 kHz
     - 48 kHz
     - 88.2 kHz
     - 96 kHz
     - 176.4 kHz
     - 192 kHz
   * - 44.1 kHz
     - 13
     - 13
     - 17
     - 17
     - 25
     - 26
   * - 48 kHz
     - 14
     - 15
     - 18
     - 19
     - 25
     - 27
   * - 88.2 kHz
     - 20
     - 20
     - 26
     - 26
     - 34
     - 34
   * - 96 kHz
     - 20
     - 22
     - 27
     - 29
     - 35
     - 37
   * - 176.4 kHz
     - 31
     - 32
     - 36
     - 36
     - 50
     - 50
   * - 192 kHz
     - 31
     - 34
     - 36
     - 39
     - 50
     - 55

.. _fig_asrc_vx4b_vpu_mhz:

.. list-table:: ASRC usage per channel (MHz): VX4 VPU
   :header-rows: 1
   :width: 100%
   :widths: 22 13 13 13 13 13 13

   * - Input / output rate
     - 44.1 kHz
     - 48 kHz
     - 88.2 kHz
     - 96 kHz
     - 176.4 kHz
     - 192 kHz
   * - 44.1 kHz
     - 12
     - 12
     - 15
     - 15
     - 22
     - 23
   * - 48 kHz
     - 12
     - 13
     - 16
     - 17
     - 23
     - 24
   * - 88.2 kHz
     - 17
     - 17
     - 23
     - 23
     - 30
     - 30
   * - 96 kHz
     - 18
     - 19
     - 24
     - 25
     - 31
     - 33
   * - 176.4 kHz
     - 28
     - 28
     - 31
     - 32
     - 43
     - 43
   * - 192 kHz
     - 27
     - 31
     - 31
     - 34
     - 44
     - 47

.. END GENERATED TABLES
