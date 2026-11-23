The SSRC uses cascaded FIR filters, including a polyphase filter for conversions
between the 44.1 kHz and 48 kHz rate families. The tables show peak estimated
MHz per channel for a two-channel instance.

Measurements use the simulator, four input samples per block, 256 input samples
per channel and dithering disabled. The estimates use a 100 MHz reference timer,
a 600 MHz processor and five logical cores; values are rounded up to whole MHz.
Scalar results are measured on XS3 and are indicative, not exact VX4 scalar
measurements. Allow additional headroom for buffering and communication.

.. BEGIN GENERATED TABLES

.. _fig_ssrc_mhz:

.. list-table:: SSRC usage per channel (MHz): Scalar (xcore-200, XS3 and VX4 without VPU; XS3 measurements)
   :header-rows: 1

   * - Input / output rate
     - 44.1 kHz
     - 48 kHz
     - 88.2 kHz
     - 96 kHz
     - 176.4 kHz
     - 192 kHz
   * - 44.1 kHz
     - 2
     - 24
     - 17
     - 27
     - 28
     - 47
   * - 48 kHz
     - 27
     - 2
     - 29
     - 18
     - 49
     - 30
   * - 88.2 kHz
     - 19
     - 44
     - 3
     - 48
     - 33
     - 54
   * - 96 kHz
     - 49
     - 21
     - 53
     - 3
     - 57
     - 36
   * - 176.4 kHz
     - 34
     - 62
     - 38
     - 68
     - 5
     - 78
   * - 192 kHz
     - 67
     - 37
     - 71
     - 41
     - 81
     - 6

.. _fig_ssrc_xs3a_vpu_mhz:

.. list-table:: SSRC usage per channel (MHz): XS3 VPU
   :header-rows: 1

   * - Input / output rate
     - 44.1 kHz
     - 48 kHz
     - 88.2 kHz
     - 96 kHz
     - 176.4 kHz
     - 192 kHz
   * - 44.1 kHz
     - 2
     - 10
     - 5
     - 12
     - 10
     - 23
   * - 48 kHz
     - 11
     - 2
     - 13
     - 6
     - 24
     - 11
   * - 88.2 kHz
     - 7
     - 17
     - 3
     - 20
     - 10
     - 24
   * - 96 kHz
     - 18
     - 8
     - 22
     - 3
     - 25
     - 11
   * - 176.4 kHz
     - 17
     - 28
     - 14
     - 31
     - 5
     - 38
   * - 192 kHz
     - 30
     - 18
     - 32
     - 15
     - 40
     - 5

.. _fig_ssrc_vx4b_vpu_mhz:

.. list-table:: SSRC usage per channel (MHz): VX4 VPU
   :header-rows: 1

   * - Input / output rate
     - 44.1 kHz
     - 48 kHz
     - 88.2 kHz
     - 96 kHz
     - 176.4 kHz
     - 192 kHz
   * - 44.1 kHz
     - 1
     - 9
     - 5
     - 11
     - 10
     - 21
   * - 48 kHz
     - 10
     - 2
     - 12
     - 6
     - 22
     - 11
   * - 88.2 kHz
     - 7
     - 16
     - 2
     - 18
     - 10
     - 22
   * - 96 kHz
     - 17
     - 8
     - 19
     - 3
     - 23
     - 11
   * - 176.4 kHz
     - 16
     - 27
     - 13
     - 28
     - 4
     - 34
   * - 192 kHz
     - 28
     - 17
     - 29
     - 15
     - 35
     - 5

.. END GENERATED TABLES
