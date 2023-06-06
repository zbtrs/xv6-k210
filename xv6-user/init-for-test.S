# Initial process that execs all.
# This code runs in user space.

#include "include/sysnum.h"

# exec(init, argv)
.globl start
start:
        li a0, 2    //O_RDWR
        la a1, 1    //console
        li a2, 0
        li a7, SYS_dev
        ecall	# dev(O_RDWR, CONSOLE, 0);
        
        li a0, 0
        li a7, SYS_dup
        ecall	# dup(0);  // stdout

        li a0, 0
        li a7, SYS_dup
        ecall	# dup(0);  // stderr
        
        li a7, SYS_fork
        ecall
        bne a0, zero, after_brk
        la a0, brk
        la a1, argv_brk
        li a7, SYS_exec
        ecall
after_brk:
        li a0, 0
        li a7, SYS_wait
        ecall

        li a7, SYS_fork
        ecall
        bne a0, zero, after_close
        la a0, close
        la a1, argv_close
        li a7, SYS_exec
        ecall
after_close:
        li a0, 0
        li a7, SYS_wait
        ecall

        li a7, SYS_fork
        ecall
        bne a0, zero, after_execve
        la a0, execve
        la a1, argv_execve
        li a7, SYS_exec
        ecall
after_execve:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_fstat
	la a0, fstat
	la a1, argv_fstat
	li a7, SYS_exec
	ecall
after_fstat:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_getpid
	la a0, getpid
	la a1, argv_getpid
	li a7, SYS_exec
	ecall
after_getpid:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_mkdir_
	la a0, mkdir_
	la a1, argv_mkdir_
	li a7, SYS_exec
	ecall
after_mkdir_:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_uname
	la a0, uname
	la a1, argv_uname
	li a7, SYS_exec
	ecall
after_uname:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_waitpid
	la a0, waitpid
	la a1, argv_waitpid
	li a7, SYS_exec
	ecall
after_waitpid:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_chdir
	la a0, chdir
	la a1, argv_chdir
	li a7, SYS_exec
	ecall
after_chdir:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_dup
	la a0, dup
	la a1, argv_dup
	li a7, SYS_exec
	ecall
after_dup:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_exit
	la a0, exit
	la a1, argv_exit
	li a7, SYS_exec
	ecall
after_exit:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_getcwd
	la a0, getcwd
	la a1, argv_getcwd
	li a7, SYS_exec
	ecall
after_getcwd:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_getppid
	la a0, getppid
	la a1, argv_getppid
	li a7, SYS_exec
	ecall
after_getppid:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_mmap
	la a0, mmap
	la a1, argv_mmap
	li a7, SYS_exec
	ecall
after_mmap:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_munmap
	la a0, munmap
	la a1, argv_munmap
	li a7, SYS_exec
	ecall
after_munmap:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_pipe
	la a0, pipe
	la a1, argv_pipe
	li a7, SYS_exec
	ecall
after_pipe:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_sleep
	la a0, sleep
	la a1, argv_sleep
	li a7, SYS_exec
	ecall
after_sleep:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_times
	la a0, times
	la a1, argv_times
	li a7, SYS_exec
	ecall
after_times:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_unlink
	la a0, unlink
	la a1, argv_unlink
	li a7, SYS_exec
	ecall
after_unlink:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_write
	la a0, write
	la a1, argv_write
	li a7, SYS_exec
	ecall
after_write:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_clone
	la a0, clone
	la a1, argv_clone
	li a7, SYS_exec
	ecall
after_clone:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_dup2
	la a0, dup2
	la a1, argv_dup2
	li a7, SYS_exec
	ecall
after_dup2:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_fork
	la a0, fork
	la a1, argv_fork
	li a7, SYS_exec
	ecall
after_fork:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_getdents
	la a0, getdents
	la a1, argv_getdents
	li a7, SYS_exec
	ecall
after_getdents:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_gettimeofday
	la a0, gettimeofday
	la a1, argv_gettimeofday
	li a7, SYS_exec
	ecall
after_gettimeofday:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_open
	la a0, open
	la a1, argv_open
	li a7, SYS_exec
	ecall
after_open:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_read
	la a0, read
	la a1, argv_read
	li a7, SYS_exec
	ecall
after_read:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_test_echo
	la a0, test_echo
	la a1, argv_test_echo
	li a7, SYS_exec
	ecall
after_test_echo:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_umount
	la a0, umount
	la a1, argv_umount
	li a7, SYS_exec
	ecall
after_umount:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_wait
	la a0, wait
	la a1, argv_wait
	li a7, SYS_exec
	ecall
after_wait:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_yield
	la a0, yield
	la a1, argv_yield
	li a7, SYS_exec
	ecall
after_yield:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_mount
	la a0, mount
	la a1, argv_mount
	li a7, SYS_exec
	ecall
after_mount:
        li a0, 0
        li a7, SYS_wait
        ecall

	li a7, SYS_fork
	ecall
	bne a0, zero, after_openat
	la a0, openat
	la a1, argv_openat
	li a7, SYS_exec
	ecall
after_openat:
        li a0, 0
        li a7, SYS_wait
        ecall

loop:
        jal loop

# for(;;) exit();
program_exit:
        li a7, SYS_exit
        ecall
        jal program_exit

brk:
        .string "/brk\0"
argv_brk:
        .long brk
        .zero 12 //不知道为什么要填充12个字节，实际上才能让编译出来的代码填充了8个0字节。

close:
        .string "/close\0"
argv_close:
        .long close
        .zero 12

execve:
        .string "/execve"
argv_execve:
        .long execve
        .zero 12

fstat:
        .string "/fstat\0"
argv_fstat:
        .long fstat
        .zero 12

getpid:
        .string "/getpid\0"
argv_getpid:
        .long getpid
        .zero 12

mkdir_:
        .string "/mkdir_\0"
argv_mkdir_:
        .long mkdir_
        .zero 12

mount:
        .string "/mount\0"
argv_mount:
        .long mount
        .zero 12

openat:
        .string "/openat\0"
argv_openat:
        .long openat
        .zero 12

uname:
        .string "/uname\0"
argv_uname:
        .long uname
        .zero 12

waitpid:
        .string "/waitpid\0"
argv_waitpid:
        .long waitpid
        .zero 12

chdir:
        .string "/chdir\0"
argv_chdir:
        .long chdir
        .zero 12

dup:
        .string "/dup\0"
argv_dup:
        .long dup
        .zero 12

exit:
        .string "/exit\0"
argv_exit:
        .long exit
        .zero 12

getcwd:
        .string "/getcwd\0"
argv_getcwd:
        .long getcwd
        .zero 12

getppid:
        .string "/getppid\0"
argv_getppid:
        .long getppid
        .zero 12

mmap:
        .string "/mmap\0"
argv_mmap:
        .long mmap
        .zero 12

munmap:
        .string "/munmap\0"
argv_munmap:
        .long munmap
        .zero 12

pipe:
        .string "/pipe\0"
argv_pipe:
        .long pipe
        .zero 12

sleep:
        .string "/sleep\0"
argv_sleep:
        .long sleep
        .zero 12

times:
        .string "/times\0"
argv_times:
        .long times
        .zero 12

unlink:
        .string "/unlink\0"
argv_unlink:
        .long unlink
        .zero 12

write:
        .string "/write\0"
argv_write:
        .long write
        .zero 12

clone:
        .string "/clone\0"
argv_clone:
        .long clone
        .zero 12

dup2:
        .string "/dup2\0"
argv_dup2:
        .long dup2
        .zero 12

fork:
        .string "/fork\0"
argv_fork:
        .long fork
        .zero 12

getdents:
        .string "/getdents\0"
argv_getdents:
        .long getdents
        .zero 12

gettimeofday:
        .string "/gettimeofday\0"
argv_gettimeofday:
        .long gettimeofday
        .zero 12

open:
        .string "/open\0"
argv_open:
        .long open
        .zero 12

read:
        .string "/read\0"
argv_read:
        .long read
        .zero 12

test_echo:
        .string "/test_echo\0"
argv_test_echo:
        .long test_echo
        .zero 12

umount:
        .string "/umount\0"
argv_umount:
        .long umount
        .zero 12

wait:
        .string "/wait\0"
argv_wait:
        .long wait
        .zero 12

yield:
        .string "/yield\0"
argv_yield:
        .long yield
        .zero 12
