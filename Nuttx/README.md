How to use Nuttx

In fedora install:
 Now we need to install the dependencies
  - bison
  - flex
  - ncurses-devel
  - arm-none-eabi-gcc-cs-c++
  - qemu-user
  - gcc-c++-arm-linux-gnu
  - gperf

Download:
- kconfig-frontends-4.9.0.0.tar.xz
http://ymorin.sis-a-geek.org/download/kconfig-frontends/kconfig-frontends-4.9.0.0.tar.xz
- The actual Nuttx
https://sourceforge.net/projects/nuttx/
 - apps-7.20.tar.gz
 - nuttx-7.20.tar.gz


Create the next folder structure:
~/nx/misc <- expand here  kconfig-frontends-4.9.0.0.tar.xz
~/nx/apps <- expand here  apps-7.20.tar.gz
~/nx/nuttx <- expand here nuttx-7.20.tar.gz

expand kconfig-frontends-4.9.0.0.tar.xz in ~/nx/misc/
And run the next commands:
```
./configure --enable-mconf
make
sudo make install
```

If the frontends were installed something like this shall appear:
```
libtool: finish: PATH="/sbin:/bin:/usr/sbin:/usr/bin:/sbin" ldconfig -n /usr/local/lib
----------------------------------------------------------------------
Libraries have been installed in:
   /usr/local/lib

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the '-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the 'LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the 'LD_RUN_PATH' environment variable
     during linking
   - use the '-Wl,-rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to '/etc/ld.so.conf'
```

## Configure the OS

The available boards are listed as directories in
`~/nx/nuttx/configs/*`
```
cd ~/nx/nuttx/tools
./configure.sh samv71-xult/nsh
```

## Configure the toolchain:

```
cd ~/nx/nuttx/
make menuconfig
~/nx/nuttx/setenv.sh
```

## Programming the execution
https://github.com/yhoazk/edbg

Compiling the project:
```
make all
```

A binary `edbg` shall be created.
Now move the device rules to `/etc/udev/rules.d `
```
sudo cp 90-atmel-edbg.rules  /etc/udev/rules.d/
```


# Testing the `edbg`
```
./edbg -r -t atmel_cm7  -f ./read_prog.bin
```
