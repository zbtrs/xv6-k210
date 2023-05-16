#ifndef __VMA_H
#define __VMA_H

#include "types.h"
#include "proc.h"

enum segtype {NONE, MMAP};

struct vma {
    enum segtype type;  //分配的vma的作用是干什么
    int perm;   //这个vma的权限是什么
    uint64 addr;   //vma映射的内存地址是什么
    uint64 sz;   //vma映射的大小是什么
    uint64 end;   //vma映射的结束地址
    int flags;   
    int fd;  
    uint64 f_off;
    struct vma *prev;   //链表结构，按照addr排序
    struct vma *next;
};

struct vma *vma_init(struct proc *p);
struct vma *alloc_vma(struct proc *p, enum segtype type, uint64 addr, uint64 sz, int perm, int alloc, uint64 pa);
#endif

