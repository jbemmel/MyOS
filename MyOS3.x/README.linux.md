# Linux-native build

MyOS remains a freestanding 32-bit x86 kernel, but it can now be compiled and
linked entirely with Linux-hosted GNU tools. The old Ant/Cygwin build remains
available for historical reference.

Required commands: GNU Make, `g++`, GNU binutils, Python 3, and gzip. A 32-bit
userspace runtime is not required because MyOS is freestanding and does not link
against the host C or C++ libraries.

Build with all available CPU cores:

```sh
make -j"$(nproc)"
```

Outputs are written under `build/linux`, including `kernel.elf`, `module.elf`,
`kernel.asm`, and the bootable kernel payload `kernel.bin`.

For an optimized build:

```sh
make -j"$(nproc)" release
```

Use `make check -j"$(nproc)"` to compile every source without linking.

Generate the Doxygen HTML reference under `build/doc/html` with:

```sh
make docs
```
