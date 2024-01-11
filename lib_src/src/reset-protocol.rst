Resetting
=========

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
