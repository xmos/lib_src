Controlling Asynchronous Sample Rate Conversion by Measuring the Phase Error
============================================================================

An Asynchronous Sample Rate Converter needs to estimate the rate difference
between two sources, and then interpolate samples in order to create the
resampled signal. This will occasionally invent or remove a sample from a
stream.

In typical cases, an ASRC will match rates that are no more than a small
amount apart; for example 48,000 Hz and 48025 Hz which are 512 ppm (0.05%)
different. ASRCs may also be able to, at the same time, convert different
base frequencies, for example 192,000 to 44,100. We start by just looking
at a conversion factor that is very close to 1.

In a typical ASRC we need a short FIFO of samples that is used to
rate-match at a coarse level. We will get back to the length of the FIFO at
a later stage, but for now lets assume that the FIFO has 10 elements and is
ideally half-full, then in the normal operating sequence the length of the
FIFO will be something like::

  5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,etc

A sample gets taken out of the FIFO, and a sample gets pushed into the
FIFO. If the rate was very badly mismatched the FIFO length will slowly go
up::

  5,4,5,4,5,4,5,6,5,6,5,6,5,6,5,6,7,6,7,6,7,6,7,8,7,8,7,etc

Note that in the case where a signal is 500ppm off, the FIFO will get one
longer after every 2000 samples; the changes are very subtle when looking
at a sample by sample level.

There is a variety of methods to estimate the conversion rate. The method
we use here uses an analogue estimation of the phase difference.

Estimating the rate difference in the simple case: one sample in one sample out
-------------------------------------------------------------------------------

Our algorithm looks at a block that comprises a FIFO, an ASRC, and a rate
estimator, shown in
:ref:`controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_block_diagram`. The order of the FIFO and ASRC is
arbitrary. The way it is shown, the ASRC will receive samples (in our case
four samples each beat), the FIFO will receive samples from the ASRC pass
(3, 4, or 5 if matching similar input and output speeds), and samples are
removed from the FIFO one sample at a time. If the ASRC is set to upsample
it may push as many as 17 samples at a time. If the ASRC is set to
downsample it may push as little as 1 sample at a time.


.. _controlling_asycnhronous_sample_rate_conversion_by_measuring_the_phase_error_fft_block_diagram:

.. figure:: images/block-diagram.*
            :width: 75%

            Block diagram:

What we are going to measure is the phase-difference between samples going
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

Calculating the ASRC setting
----------------------------

The ASRC setting needs a ratio between the two clocks. This number is
calculated by correcting it with two constants depending on the phase and
proportional errors::

   fs_ratio = fs_ratio - Kp * proportional_error - Ki * integral_error


Calculating a phase error in a discrete world
---------------------------------------------

The above phase error works on a theoretical FIFO where the input and
output side of a FIFO have to match the speed (as the FIFO would otherwise
overflow or underflow). However, in real life the FIFO has the occasional
extra sample pushed in. If we look at the block comprising both the ASRC
and the FIFO, then we do have an ideal system, but here the sample rates
are systematically different on both sides.

In order to make this work we need to use precise knowledge of the ASRC;
even though the ASRC produces and consumes discrete samples, it progresses
internally by a fractional sample. We can use this fractional sample as a
mechanism to estimate the phase error.

We assume that the FIFO is behind the ASRC::

    Producer -> ASRC ->  FIFO ->  Consumer
    Domain A              ||      Domain B

The left half of the system is one clock domain, the right half the other
clock domain. The FIFO holds it all together
On each iteration, the ASRC will consume a fixed number of samples (say 4),
and for each *F* samples it will produce an output sample, where *F*
is a precise fractional number. In other words, for a large enough number
of samples the number of output samples times F is the number of input
samples. More precisely (given that F changes), the sum of all F values is
the number of samples that have been produced.

We calculate a fractional credit that the ASRC has built-up, this is the
number of fractional samples that the ASRC has consumed. As an example we
set *F* to 0.93; that is, on every iteration we consume one ASRC sample and
keep 0.07 sample behind. (Typically *F* is much closer to 1!). The iterations
of the ASRC will be as follows:

  ========== ================= ============
  Consumed   Samples awaiting  Produced
  ========== ================= ============
  4          0.36              4
  4          0.72              4
  4          0.07              5
  4          0.43              4
  4          0.79              4
  4          0.14              5
  4          0.50              4
  4          0.86              4
  4          0.21              5              
  4          0.57              4              
  4          0.93              4              
  4          0.28              5
  4          0.64              4
  4          1.00              4
  4          0.35              5
  4          0.71              4
  4          0.06              5
  ...        
  ========== ================= ============

Note that after the first iteration, 4 x 0.93 = 3.72 samples have been used
on the input, notionally leaving 0.36 samples behind. After the second
iteration, another 3.72 samples have been used, leaving 0.72 samples
behind. After the third iteration, 5 x 0.93 = 4.65 samples are being used,
leaving 4 + 0.72 - 4.65 = 0.07 samples behind.

This is shown below for a ratio of less than one::

   Input-samples   --0---------1---------2---------3---------4---------5----
   Output-samples  ---0-------1-------2-------3-------4-------5-------6-----
   Credit            0.9     0.1     0.3     0.5     0.7     0.9     0.1
   Matching inp sam  0.1     0.9     1.7     2.5     3.3     4.1     4.9

This is shown below for a ratio of greater than one::

   Input-samples   --0-------1-------2-------3-------4-------5-------6-----
   Output-samples  ---0---------1---------2---------3---------4---------5----
   Credit            0.875     0.625     0.375     0.125     0.875     0.625
   Matching inp sam  0.125     1.375     2.625     3.875     5.125     6.475

We have a precise time that a sample is being pulled out of the queue, and
using the fractional credit we can match that back on a precise time when
the input samples where pushed in.

The process is:

* Push four samples in at time T

* Run the ASRC, which produces 3, 4, or 5 samples; each of these samples
  can be attributed a time relative to T

* Push these samples into the FIFO

* When a sample is pulled out from the FIFO we can look at the current time
  and the attributed time in the FIFO, and work out the phase error.

The challenge is to not store all the times in the FIFO as that is rather
wasteful in both time and memory.

There are two cases to distinguish here - the case where the output is
ahead or behind.

* Where the output is running behind (too slow), the FIFO is more than
  half-full. The phase error is easy to compute for we time stamp any of
  the samples that is pushed into the FIFO, remember the number of the
  sample (X) and the time stamp (T), and when the sample halfway on (X+N)
  is pulled out we can calculate the phase error by subtracting the current
  time from the time-stamp T. It requires us to remember one sample number and
  one time-stamp.

* Where the output is running ahead (too fast), the FIFO is less than
  half-full. The phase error is now defined the other way; when we pull a
  sample Y out of the FIFO, we wait for sample Y-N to be pushed into the
  FIFO. However, this is hard, as sample Y-N may not be pushed into the
  FIFO as they are pushed in in clumps of 3, 4, or 5 samples.

One solution is therefore to always measure forward, and simply
extrapolate the number of whole samples (forward or backward) that we
need to compensate for. We calculate timestamp Tx for sample X by
interpolating using the fractional credit, and measure timestamp Ty for
sample Y that is pulled out, and we calculate::

    (Ty-Tx) + (Y - X) * samplerate

The sample rate should be the actual sample rate on the output side but it
is fine if it is one or two time ticks off. At 48 kHz the sample rate is
2083 timer ticks, so a single timer tick off is a 500 ppm error. Hence, we
may as well use the ideal sample rate. We can improve this by using an
actual measured number here.

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

Implementing the FIFO
---------------------

The ASRC block needs a FIFO after it. The FIFO fulfils three purposes:

* When rates change, the loop filter will spread this rate change out over
  time. During this time, samples will accumulate in the FIFO, or the FIFO
  will be depleted to cope with a mismatch in rate. Over time, the FIFO
  will balance back to its mid-point.

* The ASRC operates on blocks of four samples; and produces blocks of
  between three and five samples. the FIFO smoothes these
  blocks out.

* The FIFO acts as an asynchronous handover between two halves of the
  system: the half that operates on clock A and the half that operates on
  clock B. Each of these halves can operate synchronously, but between the
  two an asynchronous handover is necessary

For this reason, a FIFO of N elements is created; where N/2 is the ideal
number of elements in the FIFO (the mid point). Each element in the FIFO is
capable of holding a frame of data; one value for each channel. The FIFO
notionally operates on the output frequency.

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


