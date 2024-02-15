Asynchronous FIFO
=================

An Asynchronous FIFO is a non-blocking data structure in which elements
gets pushed in on one side and pulled out on the other side. The key to
this component is the non-blocking nature, and the underlying assumption
that software on both sides rate-matches their requests. The asynchronous
FIFO has a PID control inside it that can be used to control either the
producer or the consumer.

Two typical use cases are shown in :ref:`asycnhronous_FIFO_use_cases`.
In te first use case we have an Asynchronous Sample Rate Converter (ASRC) in
front of the FIFO. The task of this ASRC is to dynamically introduce or
remove samples in order to match the rate of producer and consumer. In the
second use case we have a PLL (either hardware or software) that is used to
match the rate of the producer and consumer.

.. _asycnhronous_FIFO_use_cases:

.. figure:: images/use-cases.*
            :width: 75%

            Use cases for the asynchronous FIFO

In order to use the asynchronous FIFO one needs at least two threads: a
producer thread (on the left), and a consumer thread (on the right). These
threads are free running relative to each other, and the FIFO transports
data from the producer to the consumer. The FIFO has a fixed length, set on
creation, and the control algorithm inside the FIFO tries and keep the FIFO
half-full at all times. When the producer is slower than the consumer the
FIFO will drain a bit until the rates match again, and when the producer is
faster than the consumer the FIFO will grow until the rates match again. In
order to ensure that the FIFO stays half full, the control algorithm will
always slightly overshoot on a a step-change in rates.

Note that the FIFO is unaware whether it is the producer that is too fast,
or the consumer that is too slow. It does not attribute blame for a
rate-mismatch. The FIFO just observes the mismatch. 


Using the Asynchronous FIFO
---------------------------

An Asynchronous FIFO is allocated as an array of double-word integers::

  int64_t array[ASYNCHRONOUS_FIFO_INT64_ELEMENTS(ENTRIES, SAMPLE_SIZE)];

The ``ASYNCHRONOUS_FIFO_INT64_ELEMENTS()`` macro calculates the number of
double words required for the FIFO given the number of entries in the FIFO,
and the number of words that each sample occupies. For example, when
transferring stereo Audio through a fifo with 40 elements one would use
``ASYNCHRONOUS_FIFO_INT64_ELEMENTS(40, 2)``. Note that the two elements are
not interchangeable.

The number of elements in the FIFO is a trade-off that the system designer
makes. As the FIFO will always aim to be half-full, a large number of
elements will introduce a high latency in the system and occupy a large
amount of memory. A short FIFO wil contribute little latency but may easily
overflow and underflow.

The Asynchronous FIFO has four functions to control the FIFO:

* ``asynchronous_fifo_init()`` initialises the FIFO structure. It needs to
  know the number of integers that comprise a single sample, the maximum
  length that has been allocated for the FIFO.

* ``asynchronous_fifo_exit()`` uninitialises the FIFO structure.

* ``asynchronous_fifo_consume()`` takes one sample from the FIFO. It
  must be given a timestamp related to when this (or the previous) sample
  is (was) output. This timestamp is a value measured in 100 MHz ticks.

* ``asynchronous_fifo_produce()`` puts one sample into the FIFO. It should
  be given regular timestamps; a boolean arguments indicates whether the
  timestamp is valid or not.

The producer function returns the current rate-error observed between the
producer and consumer. The rate-error is measured as a 32-bit signed fixed point
value. For convenience, the fixed point value has a Q-value of 32, so the
desired rate is calculated as::

  unit_rate + ((error * (int64_t) unit_rate) >> 32)

Where ``unit_rate`` is the expected value that would make producer and
consumer match if they had no error and ``error`` is the value returned by
``asynchronous_fifo_consume()``. The number used for ``unit_rate`` may be a
PLL setting, or an ASRC ratio value. Note that the above maths can be
executed in a single instruction on XCORE.

Using the Asynchronous FIFO with an ASRC
++++++++++++++++++++++++++++++++++++++++

The producing side for use with an ASRC is as follows::

    int ideal_fs_ratio;
    fs_ratio = asrc_init(inputFsCode, outputFsCode, sASRCCtrl, SRC_CHANNELS_PER_INSTANCE, SRC_N_IN_SAMPLES, SRC_DITHER_SETTING);
    ideal_fs_ratio = fs_ratio >> 32;

    while(1) {
        int error;
        int now = hwtimer_get_time(tmr); 
        int  = asrc_convert_quad_input(out_samples, in_samples, now, &ts);
        int num_samples = asrc_process(samples, out_samples, fs_ratio, asrc_ctrl);
        int ts = asrc_timestamp_interpolation(now, asrc_ctrl, 48000);
        for(int j = 0; j < num_samples; j++) {
            error = asynchronous_fifo_produce(a, (int32_t *)&out_samples[j], ts,
                                                      j == num_samples - 1);
        }
        fs_ratio = (((int64_t)ideal_fs_ratio) << 32) + (error * (int64_t) ideal_fs_ratio);

Becasue the frequency ratio used by the ASRC is a 64-bit value it needs a
bit of casting and shifting to build the 32-bit value. In effect, we are
ignoring the bottom 32 bits produced by the ASRC initialisation function,
and use the Asynchronous FIFO to work out an optimal value for those bits.

This example shows how to create an interpolated time stamp. We measure the
time related to when four samples were converted, and then use a helper
function, ``asrc_timestamp_interpolation`` in order to work out where in
time the resampled value was taken. This interpolated time stamp is passed
on to the Asynchronous FIFO, noting that only the last sample value was
calculated.

The consuming side just picks a sample of the queue and provides a timestamp::

        int now = (void) hwtimer_get_time(tmr);
        asynchronous_fifo_consume(a, &output_data, now);

Ideally, the timestamp is taken when, for example, the previous sample was
put on the wire and is therefore as close as possible to the word clock of
the outgoing signal. Note that if the timestamp is related to the previous
sample, then a constant of one sample-time should be added to the time
stamp to make sure that the queue is exactly in the middle and not one off
the middle.

Using the Asynchronous FIFO with a PLL
++++++++++++++++++++++++++++++++++++++


Internal workings of the Asynchronous FIFO
------------------------------------------

Internally, the Asynchronous FIFO measures the phase difference between
producer and the consumer. It does so by storing the time samples of all
consumed samples, and comparing the sample consumed N/2 samples in the past
with the time sample 
TWhat we are going to measure is the phase-difference between samples going
into the FIFO and samples going out of the FIFO. In an ideal world, sample X
sample enters the FIFO at exactly the same time as sample X-N/2 leaving the
FIFO. N/2 is the ideal fill-level of the FIFO that is N items long. For
example, for a FIFO of 10 elements the ideal fill level is 5. If the
output is running slightly too fast then sample X will enter the FIFO just
after X-N/2 leaves the FIFO; if the output is running slightly too slow than
sample X will enter the FIFO just before X-N/2 leaves the FIFO. We will be
using the time between sample X entering and sample X-N/2 leaving the FIFO as
the guidance for our rate matching algorithm; this is the phase difference.

The phase difference is notionally a continuous value (a time stamp)
in practice we measure it with the reference clock which has a 10 ns
granularity. Unlike traditional phase differences that are measured in
radians and where the maximum phase difference is *+/- pi* , we measure the
phase difference as a time, and we allow the phase to be off by more than
half a sample. If the queue is overflowing badly (say there is 9 items in
it rather than 5), then the phase difference will account for the 4 extra
items in the FIFO, and the fractional item, somewhere between 4 and 4.999...

Note that the phase difference itself is an integral value; it is the
number of samples since the beginning of time that the ASRC is out by. The goal of
the rate converter is to make the phase difference stable (ie, it does not
move between subsequent samples), and zero (ie, the FIFO is exactly mid
level). Hence, we can see the differential of the phase error as a
proportional error, and the phase error itself as an integral error.


The three degrees of freedom
----------------------------

There are three degrees of freedom in this system:

* The length of the FIFO

* The time constant of the loop filter

* The jitter characteristics of the two clocks that can be sustained.

If you pick a long FIFO length, everything is great, but you introduce a
large delay between input-signal and output-signal. If you pick a short
time constant for the loop-filter, the adjustments of the ASRC will be
audible as harmonic distortion. If you only permit small changes between
the clocks, then you get away with a long time constant on the loop filter
and a short FIFO.

You can pick two and the third one has to match; given the jitter
characteristics and the time-constant the FIFO length follows.
Alternatively, given the jitter characteristics and the FIFO length the
maximum time constant for the loop-filter follows.

FIFO Implementation
---------------------

The FIFO straddles two threads; this is essential as the two threads
operate on different heart-beats. Hence, the FIFO is a shared-memory
element between those two threads. A readpointer (managed
by the outgoing thread) and a write-pointer (managed by the incoming thread)
are maintained independently. The read-pointer and write-pointer are
normally N/2 elements apart.

During normal operation the FIFO is not managed. Incoming and outgoing
traffic are rate-matched, and the read-pointer and write-pointer will be on
opposite ends of the circular buffer. There are three situations where
operation may be abnormal:

* Where the consumer is no longer consuming samples

* Where the producer is no longer producing samples

* Where a larger than expected change in the sample rates has caused the
  loop filter to require more than N/2 spaces away from the mid-point.

In the simplest case, we can treat all three as fatal, and restart the FIFO
freshly. That is, set the fifo to all zeroes, set the pointers
on opposite ends of the FIFO, reset the phase-error, reset the fs_ratio to
1.0, and fade in. If one side has permanently stopped this will just cause
the system to go quiet on the outside.

The last case could be simply ignored: the read and write pointer will wrap
on top of each other, but this will settle again after the rates are
matched. It will cause some odd sound effects where future and past samples
are mixed up for a period of time. However, this should not happen if the
loop filter, FIFO length, and expected jitter are matched.

Detecting these cases requires us to calculate the modulo difference
between the write-pointer and read-pointer; if that difference is close to
zero we're about to underflow; if it is close to N we're about to
overflow. The notion "close to" is used since the read- and write-pointer
are updated independently by different threads, so the pointer may be one
less than anticipated, and we may miss an update. Underflow is detected by
the thread on the output side, overflow is detected by the thread on the
input side. Differentiating overflow/underflow from too large a change in
the sample rate may be hard and not necessary if they are all treated in
the same way.

Comms protocol through the FIFO
-------------------------------

On the thread on the input side:

  * If the RESET flag is set or there is no room left in the FIFO:
    
    #. Set the write-pointer to half-way from the read-pointer

    #. Set fs_ratio to 1

    #. Clear the phase error and reset all other PID state.

    #. Clear the RESET flag (this is ok - it may go around the loop twice but
       never three times)
    
  * else there is room and no RESET:

    #. Copy one frame into the FIFO

    #. Increase the write-pointer

    #. Mark the time-stamp of the last one  (HOW??)

    #. If the sample_time_valid is True then:

       * Obtain values

       * Run PID.

       * Set sample_time_valid to False

On the thread on the output side:

  * Record the sampled time
    
  * If there is a sample left:

    #. Copy it out

    #. Increase the read-pointer.

    #. Increase the output sample number

    #. If sample_time_valid is False:
      
       * Write the output sample number

       * Write the sampled time

       * Set the sample_time_valid to True

  * If there is no sample left

    #. Set the RESET flag

    #. Copy out the previous sample

    #. Reset output sample number to zero.

Measuring ASRC performance
--------------------------

As a test setup we create a 2 kHz sine wave at a sample rate of notionally
48,000 Hz. That is, we have a sine wave with a periodicity of 24 samples.
We feed this sample rate into an interface that runs at exactly 48,012 Hz,
into the ASRC, into the FIFO, and out through a second interface that
initially runs at 48,012 Hz. The second interface switches to 47,993 Hz
after 24,000 samples.

Hence, the ASRC is initially set to have a ratio of 1.00000, and we expect
that on sample 24,000 a phase error starts to build, which will eventually
cause the ASRC to settle on a rate of 1.00039589. Eventually, the phase
error should come back down to 0. This shows the case where we have a
sudden clock change of nearly 400 ppm.

:ref:`extending_usb_audio_with_digital_signal_processing_dsp_basic_pipe`.
In this pipeline digital samples enter the pipeline (on the left-hand-side
in this case), flow through a series of DSP blocks, and eventually samples
leave the pipeline (on the right-hand-side in this case).

:ref:`controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_phase_error`
shows the phase error for three scenarios. The scenarios use a PID that is
slow (a time constant of 10s), a PID that is typical (a time constant of
0.5s) and a PID that is fast (a time constant of 12.5ms). One can see that
for the slowest PID the phase error tops out at around 13 samples, which
means that it wil need a FIFO of at least 26 elements long to cope with
this sudden kick to the clock. Note that the phase
error is a continuous metric; it is possible for a phase error to be 0.37 samples.

.. _controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_phase_error:

.. figure:: images/phase-error.*
            :width: 75%

            Phase error over time (measured in samples)

:ref:`controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_asrc_ratio`
shows the ASRC ratio of the same three scenarios, measured over time. One
can see that for the slowest PID it takes about 2 seconds for the sample
rate to be matched, and a gentle overshoot ensures that the phase error
eventually settles down to 0. For the fastest PID we can see that the ASRC
ratio never settles but is noisy.

.. _controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_asrc_ratio:

.. figure:: images/asrc-ratio.*
            :width: 75%
                    
            ASRC ratio over time


:ref:`controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_after_change`
shows the FFT of the sine-wave post ASRC right at the end of the test. At
this point we expect the sine wave to be stable; and the signal we see is a
product of the windowing function and the ASRC process.

.. _controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_after_change:

.. figure:: images/fft-after-change.*
            :width: 75%

            FFT after change


:ref:`controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_on_change`
shows the FFT of the sine-wave post ASRC during the change. Half the window
is before the change, half is when the frequency is moving. The error shown
here is the maximum error we can get.

.. _controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_on_change:

.. figure:: images/fft-on-change.*
            :width: 75%

            FFT during change


Finally,
:ref:`controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_medium_pid`
shows the FFT output of the medium PID before, during, and well after the
change. This shows how the harmonic distortion progresses over time.

.. _controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_medium_pid:

.. figure:: images/fft-medium-pid.*
            :width: 75%

            FFT of medium PID




Resetting
---------

There are for reasons why the ASRC may need to be reset:

* The consumer function is called too slowly, and the ASRC cannot keep up
  with it, this will gradually lead to a buffer overflow.

* The producer funciton is called too slowly, and the ASRC cannot keep up,
  this will gradually lead to a buffer underflow
  
* The consumer function is no longer called. This will dramatically lead to
  a buffer overflow
  
* The producer function is no longer called. This will dramatically lead to
  a buffer underflow.

These four cases lead to only two observable events a buffer overflow
(spotted by the producer as it is trying to push a sample in that won't
fit), or a buffer underflow (spotted by the consumer as it is trying to
pull a sample out that isn't available). Note that these events are not
necessarily permanent; in particular the first two causes where the ASRC
cannot keep up can cause temporary overflows and underflows.

In order to cater with all these eventualities we use the following reset
protocol:

* Resets are always requested by the consumer. The consumer requests a
  reset by setting a reset flag to '1', and does so because it has spotted
  an underflow condition. The underflow condition is spotted because the
  read pointer is getting too close to the write-pointer. Too close means
  that there is only 1 sample left in the FIFO.

  When the consumer has requested the reset, it will no longer operate
  until the reset request has been dealt with. In particular, it will no
  longer try and spot the underflow condition, and it will just return the
  current value until the reset has been dealt with. On requesting the
  reset it will reset all it's state to the initial condition, except for
  the read-pointer which is kept where it is (in order to not confuse the
  writing side when it tries and detect an overflow condition).

  This deals with underflow cases as far as the consumer is concerned

* When the producer spots a reset request it will take action. In
  particular, it will reset all the producer state to it's initial
  condition, which involves setting the write-pointer half way from the
  read-pointer. As the read pointer is stable this is safe to do. It will
  also reset any state that only the producing side can reset, and when
  it's all complete it will clear the reset flag, acknowledging that
  producer and consumer are in a good shape to restart.

  This completes the reset requested by the consumer.

* When the producer spots an overflow condition, it will stop the producer.
  Even though more and more samples may be passed in, they are discareded
  until such a point where the consumer has underflowed and requested a
  reset.

  What this step achieves is to wait for the consumer to become alive (in
  case it had stopped working), then spot that things are bad when it
  underflows, and then request a reset. When this reset request comes in
  the producer will act as normal, process the reset request, and come out
  clean.

Initialising is very similar to resetting; it calls the same two functions
for initialising the producer and consumer state. Prior to that it also
initialises the read pointer (as this is not initialised by the consumer),
and it resets the variables that are being set by the consumer and reset by
the producer.
