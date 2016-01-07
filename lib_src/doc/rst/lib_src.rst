.. include:: ../../../README.rst

Connecting external signals to multibit ports
---------------------------------------------

Multi-bit ports can be connected to independent signals in either an
all output configuration (see :ref:`lib_gpio_n_bit_output`) or an all
input configuration (see :ref:`lib_gpio_n_bit_input`). This implies
two important restrictions:

  * **Bi-directional signals cannot use this library**
  * **The signals on the same port must go in the same direction**

To use bi-directional signals, a dedicated 1-bit hardware port needs
to be used.

.. _lib_gpio_n_bit_input:

.. figure:: images/n_bit_input.*

   Input configuration

.. _lib_gpio_n_bit_output:

.. figure:: images/n_bit_output.*

   Output configuration

Performance restrictions
........................

This library allows independent access to the pins of mulit-bit ports
by multiplexing the port output or input in software. This means that
there are some performance implications to using the ports.

  * The internal buffering, serializing and de-serializing features of
    the port are not available.
  * The software locking and multiplexing between individual bits of
    the port limits performance in line with the performance of the speed
    of the logical core that is driving the port. As such, toggling
    pins at speed above 1Mhz, for example, is not achievable (on a
    62.5Mhz logical core). Lower speeds will depend on the other
    calculation on the core and the use of other pins of the port.

As such, sharing multi-bit ports is most suitable for slow I/O such as
LEDs, buttons, reset lines *etc.*

Usage
-----

Output GPIO usage
.................

Output GPIO components are instantiated as parallel tasks that run in a
``par`` statement. These components connect to the hardware ports of
the xCORE device. The application
can connect via an interface connection using an array of the ``output_gpio_if``
interface type:

.. figure:: images/output_gpio_task_diag.*

   Output GPIO task diagram

For example, the following code instantiates an output GPIO component
for the first 3 pins of a port and connects to it::

  port p = XS1_PORT_4C;
   
  int main(void) {
    output_gpio_if i_gpio[3];
    par {
      output_gpio(i_gpio, 3, p, null);
      task1(i_gpio[0], i_gpio[1]);
      task2(i_gpio[2]);
    }
    return 0;
  }

Note that the connection is an array of interfaces, so several tasks
can connect to the same component instance, each controlling a
different pin of the port.

The application can use the client end of the interface connection to
perform GPIO operations e.g.::

  void task1(client output_gpio_if gpio1, client output_gpio_if gpio2)
  {
    ...
    gpio1.output(1);
    gpio2.output(0);
    delay_milliseconds(200);
    gpio1.output(0);
    gpio2.output(1);
    ...
  }

More information on interfaces and tasks can be be found in
the :ref:`XMOS Programming Guide<programming_guide>`. By default the
output GPIIO component does not use any logical cores of its
own. It is a *distributed* task which means it will perform its
function on the logical core of the application task connected to
it (provided the application task is on the same tile).

Input GPIO usage
................

There are two types of input GPIO component: those that support events
and those that do not support events. In both cases,
input GPIO components are instantiated as parallel tasks that run in a
``par`` statement. These components connect to the hardware ports of
the xCORE device. The application
can connect via an interface connection using an array of the ``input_gpio_if``
interface type:

.. figure:: images/input_gpio_task_diag.*

   Input GPIO task diagram

For example, the following code instantiates an input GPIO component
for the first 3 pins of a port and connects to it::

  port p = XS1_PORT_4C;
   
  int main(void) {
    input_gpio_if i_gpio[3];
    par {
      input_gpio(i_gpio, 3, p, null);
      task1(i_gpio[0], i_gpio[1]);
      task2(i_gpio[2]);
    }
    return 0;
  }

Note that the connection is an array of interfaces, so several tasks
can connect to the same component instance, each controlling a
different pin of the port.

|newpage|

The application can use the client end of the interface connection to
perform GPIO operations e.g.::

  void task1(client input_gpio_if gpio1, client input_gpio_if gpio2)
  {
    ...
    val1 = gpio1.input();
    val2 = gpio2.input();
    ...
    ...
    val1 = gpio1.input();
    val2 = gpio2.input();
    ...
  }

More information on interfaces and tasks can be be found in
the :ref:`XMOS Programming Guide<programming_guide>`. By default the
output GPIIO component does not use any logical cores of its
own. It is a *distributed* task which means it will perform its
function on the logical core of the application task connected to
it (provided the application task is on the same tile).

Using events
............

The :c:func:`input_gpio_with_events` and
:c:func:`input_gpio_1bit_with_events` functions support the event
based functions of the input GPIO interface::

  port p = XS1_PORT_4C;
   
  int main(void) {
    input_gpio_if i_gpio[3];
    par {
      input_gpio_with_events(i_gpio, 3, p, null);
      task1(i_gpio[0], i_gpio[1]);
      task2(i_gpio[2]);
    }
    return 0;
  }

In this case the application can request an event on a pin change and
then select on the event happening e.g.::

  gpio.event_when_pins_eq(1);
  select {
    case gpio.event():
      // This event was caused by the pin value being 1
      ...
      break;
  }

|newpage|

Pin maps
........

The GPIO tasks all take a ``pin_map`` argument. If this is ``null``
then the elements of the inteface array will correspond with the a bit
of the port based on the array element index. So the first element of
the array will control bit 0, the second with control bit 1 and so on.

Alternatively an array can be provided mapping array elements to
pins. For example, the following will map the array indices to pins 3,
2 and 7 of the port::

  char pin_map[3] = {3, 2, 7};

  int main() {
    ...
    par {
      output_gpio(i_gpio, 3, p, pin_map);
      ...

Output GPIO API
---------------

All gpio functions can be accessed via the ``gpio.h`` header::

  #include <gpio.h>

You will also have to add ``lib_gpio`` to the
``USED_MODULES`` field of your application Makefile.

Output GPIO components
......................

.. doxygenfunction:: output_gpio

|newpage|

Output GPIO interface
.....................

.. doxygeninterface:: output_gpio_if

Input GPIO API
--------------

Input GPIO components
......................

.. doxygenfunction:: input_gpio
.. doxygenfunction:: input_gpio_with_events

|newpage|

.. doxygenfunction:: input_gpio_1bit_with_events

|newpage|

Input GPIO interface
....................

.. doxygeninterface:: input_gpio_if


|appendix|

Known Issues
------------

No known issues.

.. include:: ../../../CHANGELOG.rst
