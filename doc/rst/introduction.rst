
************
Introduction
************

lib_src components
==================

``lib_src`` provides both synchronous and asynchronous audio sample rate conversion functions.

``lib_src`` includes the following components:

  * HiFi quality multi-rate sample rate conversion:

    * Synchronous Sample Rate Converter (SSRC) function
    * Asynchronous Sample Rate Converter (ASRC) function

    Both are accelerated by the Vector Processing Unit (VPU) on VPU-equipped xcore devices.

  * HiFi quality fixed factor sample rate conversion:

    * Synchronous factor of 3 downsample function (``src_ds3``)
    * Synchronous factor of 3 oversample function (``src_os3``)

  * Voice quality fixed factor sample rate conversion:

    * Synchronous factor of 3 downsample function (``src_ds3_voice``)
    * Synchronous factor of 3 oversample function (``src_us3_voice``)

  * Voice quality fixed factor sample rate conversion optimised for XS3:

    * Synchronous factor of 3 downsample function (``src_ff3_96t_ds``)
    * Synchronous factor of 3 oversample function (``src_ff3_96t_us``)
    * Synchronous factor of 3/2 downsample function (``src_rat_2_3_96t_ds``)
    * Synchronous factor of 3/2 oversample function (``src_rat_3_2_96t_us``)

  * Integration support:

    * Asynchronous FIFO with controller for use with ASRC

The component listing above includes three different component options that support fixed factor of
3 up/downsampling. To order to choose which one to use follow these steps:

  #. If HiFi quality (130 dB SNR) up/downsampling is required, use ``src_ds3`` or ``src_os3``.
  #. If voice quality (65 dB SNR) is required on xcore-200, use ``src_ds3_voice`` or ``src_us3_voice``.
  #. If voice quality (75 dB SNR) is required on VPU-equipped XS3 devices, use ``src_ff3_96t_ds`` or ``src_ff3_96t_us``.

API support by architecture
===========================

For filter APIs, the columns indicate which execution modes the filter
implements. The asynchronous FIFO and ASRC task are available on all
architectures.

.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 40 15 15 15 15

   * - API
     - XS2/XS3 scalar
     - XS3 VPU
     - VX4B scalar
     - VX4B VPU
   * - ASRC
     - Yes
     - Yes
     - Yes
     - Yes
   * - SSRC
     - Yes
     - Yes
     - Yes
     - Yes
   * - ``src_ds3``
     - Yes
     - No
     - No
     - No
   * - ``src_os3``
     - Yes
     - No
     - No
     - No
   * - ``src_ds3_voice``
     - Yes
     - No
     - No
     - No
   * - ``src_us3_voice``
     - Yes
     - No
     - No
     - No
   * - ``src_ff3_96t_ds``
     - No
     - Yes
     - No
     - No
   * - ``src_ff3_96t_us``
     - No
     - Yes
     - No
     - No
   * - ``src_rat_2_3_96t_ds``
     - No
     - Yes
     - No
     - No
   * - ``src_rat_3_2_96t_us``
     - No
     - Yes
     - No
     - No
   * - Asynchronous FIFO
     - Yes
     - Yes
     - Yes
     - Yes
   * - ASRC task
     - Yes
     - Yes
     - Yes
     - Yes

Using lib_src
=============

``lib_src`` is intended to be used with the `XCommon CMake <https://www.xmos.com/file/xcommon-cmake-documentation/?version=latest>`_
, the `XMOS` application build and dependency management system.

To use this library, include ``lib_src`` in the application's ``APP_DEPENDENT_MODULES`` list, for example::

    set(APP_DEPENDENT_MODULES "lib_src")

Applications should then include the ``src.h`` header file.

.. note::

    Inputting non-sinusoidal 0 dB FS signals into filters with saturation support (including ASRC and SSRC)
    can potentially cause clipping in the filter banks due to the `Gibbs phenomenon <https://en.wikipedia.org/wiki/Gibbs_phenomenon>`_.
    It is recommended to keep the input signal below -1 dB FS (about 10% headroom) to avoid potential clipping.
