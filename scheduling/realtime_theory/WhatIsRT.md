# What is real time and why do I need it?

In a nutshell RTOS allows developers to control how long a system will take to
perform a task or respond to critical events. Deadlines can be met within
predictable, and wholly consisten, timelines, even under heavy system loads.

## What, exactly, is real time?

A real-time system is one in whixh the correctnes of the computation not only
depends upon the logical correctness of the computation but also upon the time
at whih the result is produced. If the timing constraints of the system are not
met, system failure is said to have occurred.

Problems arise when many activities compete for a system's resource; in fact,
this is where we begin to apply real-time property to operating the system.
Any real-time system will comprise different types of activities, those that
can be scheduled, those that cannot be scheduled (interrupts, opeating system
facilities) and non real-time  activities. If non-schedulable activities can
execute in preference to schedulable activietes, they will affect the ability
of the system to handle time constraints.

## Hard vs Soft real time

A hard real-time constraint is one for which there is no value to a computation
if it is late.

Soft real time is a property of the timeliness of a computation where the value
diminishes according to its tardines. A soft realtime system can tolerate some
late answers to soft realtime computations, as long as the value hasn't
diminished to  zero. Deadlines may be missed, but the number and frequency of
such misses must typically comply with Quality of Service metrics.

Soft real time is erroneously applied to OSs that cannot guarantee computations
on time. Put simply, soft real time should not be confused with non real time.

## A working definitoon

A hard RTOS must guarantee that a feasible schedule can be executed given
sufficient computational capacity if external factors are discounted.
In other words if a system designed controls the environment, the operating
system itself will not be the cause of any tardy computations. To provide such
guarantees the Os must satisfy the following conditions:

1. Higher priority tasks always execute in preference to lower-priority tasks
2. Priority inversions, are bounded
3. Non-scheduleable activities, including both non real-time and OS activities
   do not exceed the remining capacity in any particular division

## Operating System requirements for real time

* OSR1:
  * The OS must support fixed-priority preemptyve scheduling for tasks.
    both, threads and provesses as applicable
* OSR2;
  * The OS must provide prority inversion inheritance or priority-cieling
    emulation for synchronization primitives. This prevents cases of unbounded
    priority inversion, whera a higher-priority task cannot obtain a resource
    from a lower-priority task
* OSR3:
  * The OS kernel must be preemptible
* OSR4
  * Interrupts must have a fixed upper bound on latency. By extension, support
    for nested interrupts is required
* OSR5:
  Operating systems services must execute at a priority determined by the client
  of the service. All services on which the client depends must inherit that
  priority. Priority inversion avoidance must be applied to all shared resources
  used by the service.





- - -

<sub> By Steve Furr from Military emmbeded</sub>