# MyOS2.x

MyOS2.x is a freestanding 32-bit x86 operating-system kernel.

## Build on Linux

Required tools are a C++ compiler with 32-bit x86 support, GNU binutils,
a Java JDK, `dosfstools`, and standard Unix utilities.

```sh
./build-linux.sh
./build-floppy.sh
```

The kernel and bootable FAT12 floppy image are written to `build/debug/`.
Compilation uses all available CPU cores by default. Override this with, for
example, `JOBS=4 ./build-linux.sh`.

To run the floppy with Bochs and its RFB display:

```sh
bochs -q -f bochsrc-linux.txt
vncviewer -Shared localhost:0
```
