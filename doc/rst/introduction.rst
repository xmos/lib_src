
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
  #. If voice quality (65 dB SNR) is required running on `xcore-200`, use ``src_ds3_voice`` or ``src_us3_voice``.
  #. If voice quality (75 dB SNR) is required running `xcore-ai`, use ``src_ff3_96t_ds`` or ``src_ff3_96t_us``.

Using lib_src
=============

``lib_src`` is intended to be used with the `XCommon CMake <https://www.xmos.com/file/xcommon-cmake-documentation/?version=latest>`_
, the `XMOS` application build and dependency management system.

To use this library, include ``lib_src`` in the application's ``APP_DEPENDENT_MODULES`` list, for example::

    set(APP_DEPENDENT_MODULES "lib_src")

Applications should then include the ``src.h`` header file.

