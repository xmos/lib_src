.. _ff3_example:

Fixed factor of 3 example
=========================

The ``examples/app_ff3`` application demonstrates the HiFi fixed-factor
OS3 and DS3 APIs on xcore-200 and xcore.ai. Three C tasks run concurrently on one tile,
connected in a loop by blocking channels:

.. uml::
   :caption: Fixed-factor example threads and data flow
   :alt: The application thread generates input, sends it through OS3 and DS3 threads, and writes the input and returned output to WAV files.

   @startuml
   left to right direction
   hide stereotype

   !include thread_diagram_style.puml

   rectangle "Application thread" <<thread>> {
     rectangle "Sine generator" as sine
     rectangle "App loop" as app
     rectangle "WAV writer" as wav
   }
   rectangle "Upsampling thread" <<thread>> {
     rectangle "OS3\n16 to 48 kHz" as os3
   }
   rectangle "Downsampling thread" <<thread>> {
     rectangle "DS3\n48 to 16 kHz" as ds3
   }

   sine --> app : input buffer
   app --> os3 : channel\n4 channels, 16 kHz
   os3 --> ds3 : channel\n4 channels, 48 kHz
   ds3 -[norank]-> app : channel\n4 channels, 16 kHz
   app --> wav : input and output buffers
   @enduml

The app generates 20 ms of four-channel Q1.31 audio. The channels contain
1, 2, 3 and 4 kHz sine waves at 90% full scale, using the sine and WAV
utilities shared in ``examples/shared_utils``. For every input frame, the
OS3 task sends three frames to DS3, which returns one frame to the app. Each
channel has its own filter state and delay line. OS3 accepts one sample with
``src_os3_input`` and produces three samples with ``src_os3_proc``; DS3
accepts those three samples together with ``src_ds3_proc``.

The ``PAR_JOBS`` block starts the application and filter tasks concurrently:

.. literalinclude:: ../../examples/app_ff3/src/main.c
   :language: c
   :start-at: PAR_JOBS(
   :end-at: );

The upsampling task initializes each channel, accepts one input sample per
channel and processes each of the three output phases:

.. literalinclude:: ../../examples/app_ff3/src/main.c
   :language: c
   :start-at: void upsample_task(
   :end-before: void downsample_task(

The downsampling task initializes each channel, collects three samples per
channel, and processes and sends the result back:

.. literalinclude:: ../../examples/app_ff3/src/main.c
   :language: c
   :start-at: void downsample_task(
   :end-before: int main(void)

Build and simulate
------------------

Activate XTC Tools 15.3.1 and build for an xcore.ai or xcore-200 target:

.. code-block:: console

    cd examples/app_ff3
    cmake -G "Unix Makefiles" -B build -DAPP_HW_TARGET=XK-EVK-XU316
    xmake -C build
    xsim bin/app_ff3.xe

Use ``XCORE-200-EXPLORER`` for xcore-200.

The simulator writes ``input.wav`` and ``output.wav`` in its current working
directory, overwriting existing files. Both are 20 ms, four-channel, 32-bit
PCM at 16 kHz. Import them into an audio editor such as Audacity and zoom in
to compare the corresponding channels. The output shows the filters' startup
and group delay from both the OS3 and DS3 filters; no filter tail is flushed.
The 48 kHz intermediate stream is passed between the tasks, not written to a file.
