# MSP for linux

Install the next packets:
* msp430-binutils
* msp430-gcc
* msp430-libc
* msp430mcu
* mspdebug


Normally should appear in fedora when installing any `msp*` pkg with dnf.

To start the debugger use `mspdebug  uif` if using the ezfet card.

The minimum example code is:
```

```

For the error:
```
/usr/lib/gcc/msp430/4.6.3/../../../../msp430/bin/ld: cannot open
     linker script file memory.x: No such file or directory
     collect2: ld returned 1 exit status
```

We need to specify which ecu is being used, so it can select the correct linker
script. This is done with the parameter `-mmcu=msp430f2274` in the `msp430-gcc` comand.

For the error:
```
/usr/lib64/gcc/msp430/4.6.4/../../../../msp430/bin/ld: cannot find -lgcc
```
