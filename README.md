# Embedded Operating systems ITESO 2017


### Definitions.

- _*TCB*_: Task control block
- _*PCB*_: Process control block


### Process states:
A process is a program which is currently in excecution. Differs fom a program in
that a program is also a passive entity stored in disk.
Each process includes its own process stack to store temporary data:
  - Local variables
  - function parameteres
  - return address

A process also includes a data section to contain global variables and heap-memory.
A process changes its state durgin its excecution to one of the following states.
* New
  * The process just got created.
* Running
  * The process instructions are being excecuted.
* Waiting
  * The process is idle waiting for an external event such as I/O.
* Ready
  * The process is waiting for the processor.
* Terminated
  * There are no more instructions to excecute.

![](./TaskFlow.png)


- - -

### Resources

[http://www.hexainclude.com/process-state-diagram-and-pcb/](http://www.hexainclude.com/process-state-diagram-and-pcb/)
