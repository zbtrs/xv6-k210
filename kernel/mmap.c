#include "include/vma.h"
#include "include/proc.h"
#include "include/cpu.h"
#include "include/printf.h"
#include "include/mmap.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/vm.h"
#include "include/kalloc.h"
#include "include/string.h"

uint64 mmap(uint64 start,uint64 len,int prot,int flags,int fs,off_t offset)
{
    struct proc *p = myproc();
    if (fd < 0 || fd > NOFILEMAX || offset < 0 || start % PGSIZE != 0) {
        return -1;
    }
    int perm = PTE_U;
    if(prot & PROT_READ) 
        perm  |= (PTE_R | PTE_A);
    if(prot & PROT_WRITE)
        perm  |= (PTE_W | PTE_D);
    struct file *f = fd == -1 ? NULL : p->ofile[fd];
    if(fd != -1 && f == NULL)
        return -1;
    struct vma *vma = alloc_mmap_vma(p, flags, start, len, perm, fd, offset);
    start = vma->addr;
    if (NULL == vma) {
        return -1;
    }
    uint64 mmap_size = 0;
    if (-1 != fd) {
        mmap_size = f->ep->file_size - offset;
        if (len < mmap_size)
            mmap_size = len;
        f->off = offset;
    } else {
        return start;
    }
    uint64 end_pagespace = mmap_size % PGSIZE;
    int page_n = PGROUNDUP(mmap_size) >> PGSHIFT;
    uint64 va = start;
    for (int i = 0; i < page_n; i++) {
        uint64 pa = experm(p->pagetable, va, perm);
        if (NULL == pa) {
            return -1;
        }
        if (i != page_n - 1)
            fileread(f,va,PGSIZE);
        else {
            fileread(f,va,end_pagespace);
            memset((void*)(pa + end_pagespace),0,PGSIZE-end_pagespace);
        }
        va += PGSIZE;
    }

    filedup(f);
    return start;
}