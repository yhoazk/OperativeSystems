# Amoeba OS

[src](https://www.cs.vu.nl/pub/amoeba)

## Intro

Amoeba is a general-purpose distributed OS. Designed to act as a single integrated
system running on a collection of machines. To the user, amoeba looks loke a single
time-sharing system. Amoeba is ongoing research project and it is not a plug in 
compaible with UNIX, but provides NIX emulation. 

It is intended for _distributed_ (multiple users working on different projects)
and _parallel_ computing ( One user multiple processes ). Amoeba provides mechanisms
for both, but the policy is entirely determined by user-level programs.


## Amoeba design goals

### Distribution

_Connecting together many machines_. These machines need not all be of the same
kind. The machines can be spread on a LAN. Amoeba uses the protocol FLIP for LAN
communication. If an amoeba machine has more than one network interface it will
automatically act as a FLIP router connecting various LANs.

### Parallelism

_Allowing individial jobs to use multiple CPUs easily_. For example, a branch
and bond problem such as the Traveling Salesman Problem can use tens or hundreds
of CPUs, all working together to solve the problem. Large _back end_ multiprocessors
can be harnessed this way as big "compute engines".

### Transparency

_Having the collection of computers act like a single system_. The user need not to
know the number or location of CPUs, not the place where the data is stored. Issues
like file replication are handled largely automatically.

### Performance

_Achieving all of the above in an efficient manner_. In particular, the basic
communication mechanism has been optimized to allow messages to be sent and replies
received with a minimum of delay, and allow large blocks of data to be shipped from
machine to machine at high bandwidth.


## System Architecture

An Amoeba system will consist of three functional classes of machines:
- User workstation for running the X window system, dedicated to running the UI and
does not have to do other computing.
- Pool of processors that are dynamically allocated to users as required. Usually 
each processor has several megabytes of private memorz, that is, pool processors
need not to have anz shared memory, but is not forbidden. Communiation is performed
with LAN packets. In this pool is where all the heavi processing is done
- Specialized servers, such as file servers and directory servers. They may run on
processor pool or dedicated HW.

All these components must be connected by a fast LAN.

## Fundamental concepts in Amoeba

### Microkernel + Server Arch

Amoeba is a microkernel, this means that every machine runs a small, identical piece
of software called the kernel. The kernel supports the basic process, communication,
and object primitives. Handles I/O and memory management. Everything else is built
on top of these fundamentals, usually by user-space server processes.

The system is structured as a collection of independent processes. Some if these
are user processes, running application programs, this process are called _clients_.
Other processes, like the file server, are called _servers_. The basic function of
the microkernel is to provide an environment in which the clients and servers can
comm with each other. This modular design allows users to implement new file servers
for specalized purposes (NFS, Database, ...). This is not feasible in UNIX.

### Threads

In Amoeba, each process has its own address space, but it may contain multiple
_threads of control_. Each tread has its own program counter and its own stack
but share code and global data with all the other threads in its process.

Despite their independent control all the threads can access a common block cache,
using semaphores to provide inter-thread synchronization. 

Not only the user processes are structured as a collection of threads communicating
by RPC, but the kernel is as well. In particular, threads in the kernel provide
access to memory management services.

### Remote Procedure Call

This is the basic mechanism to communicate between threads. Threads within a single 
process can communicate via the shared memory, but threads located in different
process need a different mechanism. Communication consists of a client thread 
sending a message to a server thread, then blocking until the server thread sends
back a return message, at which time teh client is unblocked.

### Group Communiation

For applications where a one-to-many communication is needed, Amoeba provides a
basic facility for reliable, totally-ordered group communication, in which all
receivers are guaranteed to get all group messages in the same order. This mechanism
simplifies distributed and parallel programming problems.

### Objects and Capabilities

All the services and communication are built around objects and capabilities.

An object is an abstract structure in which certain operations are defined. For
example a directory is an object to which certain operations can be applied, such
as _enter name_ and _look up name_.

Amoeba primarily supports software objects, but hardware objects also exist. Each
object is managed by a server process to which RPCs can be sent. Each RPC specifies
the object to be used, the operation to be performed and any parameters to be passed.

### Memory management

A process address space consists of one or more segments mapped onto user-specified
virtual-addresses. When a process is executing, all its segments are in memory. 
There is no swapping or paging, thus Amoeba can only run programs that fit in the
physical memory. It simplifies the os and gives performance, but only programms that
fit in memory can be executed.

### I/O

I/O is handled by kernel threads  via RPCs to a system thread with I/O capabilities.
The caller s not aware that the server is actually a kernel thread, since the API
to kernel and user threads is identical. Generally speaking, only the file server
and similar systems communicate with kernel IO threads.

## Software outside the kernel

The job of the Amoeba kernel is to support threads, RPC, memory management and IO.
Everything else is built on top of these primitives.

### Bullet file server

The bullet server is the std file server, it has been designed for high performance.
It stores files contiguously on disk, and caches whole files contiguosly in core.
Except for large files, when a user program needs a file, it will request that the
bullet server send it the entire file in a single RPC. A dedicated machine is needed
for the bullet server. The more RAM the better. The maximum file size is also
limited by the amount of physical memory available to the bullet server.

### Directory server

In Amoeba file management and file namig are separated. The bullet server only
handles files, not naming. It simply reads and writes files, specified by
capabilities. A capability can be thought of as a kind of handle for an object,
such as a file. A directory server maps ASCII strings onto Capabilities.
Directories contain (ASCII string, capability) pairs; these capabilities will
be for files, directories, and other objects.

A directory entry may contain either a single capability or a set of them, to
allow a file name to map onto a set of replicated files. When a user looks up
a name in a directory, the entire set of capabilities is returned. These replicas
may be on different file servers, potentially far apart. (the dir server has no
idea about what kinf of objects it has capabilities for or where are they located).
Amoeba provides operations for managing replicated files in a consistent way.

### Compilers

The GNU C compiler can be used.

### Parallel Programming

The Orca language was designed to work along with the Amoeba OS. The Orca run-time
system uses Amoeba IPC facilities to make sharing objects over the network efficient.


## Summary

### Archittectural

* Transparent distributed computing using large number of processors
* Parallel computing supported as well as distributed
* Microkernel + server architecture
* Hihg preformace RPC using the FLIP protocol
* Reliable, totally ordered group communication
* Support for heterogeneus systems
* Automatic, transparent network configuration

### User level Software

* Object based
* Mutliple threads per address space
* File an directory servers provided
* File replication included
* X windows supported
* TCP/I supported
* ANSI C supported

### UNIX

* Good integration with existing UNIX systems
* Amoeba can tal to unix via TCP/IP
* Driver available for SUN UNIX
* Partial UNIX emulation
* Over 100 UNIX.like utilities supported

## Weak points

* Over 1000 pages of documentation
* Not binary compatible with UNIX
* No virtual memory suppoted
* Works poorly when there is insufficient memory
* No NFS support
