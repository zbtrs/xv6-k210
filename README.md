# xv6-riscv

# dependency
- qemu-system-riscv64 version 7.0.0
- riscv64-linux-gnu toolchain

# doc
    AVX512OS.pdf in doc directory

# run on qemu
    make all
    make qemu-run

# debug
在一个终端中输入：

    make gdb-server

在另一个终端中输入：

    make gdb-client

即可进行调试
