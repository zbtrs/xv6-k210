#include "include/param.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/elf.h"
#include "include/riscv.h"
#include "include/vm.h"
#include "include/vma.h"
#include "include/kalloc.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/riscv.h"

// 初始化进程的vma,在proc.c中调用
struct vma *vma_init(struct proc *p)
{
    if (NULL == p) {
        printf("p is not existing\n");
        return NULL;
    }
    struct vma *vma = (struct vma*)kalloc();
    if (NULL == vma) {
        printf("vma kalloc failed\n");
        return NULL;
    }

    vma->type = NONE;
    vma->prev = vma->next = vma;
    p->vma = vma;

    // TODO:分配vma并且在有异常的时候清空


}

struct vma* alloc_vma(struct proc *p,enum segtype type,uint64 addr, uint64 sz,int perm,int alloc,uint64 pa) {
    uint64 start = PGROUNDDOWN(addr);
    uint64 end = addr + sz;
    end = PGROUNDUP(end);

    struct vma* find_vma = p->vma->next;
    while (find_vma != p->vma) {
        if (end <= find_vma->addr) 
            break;
        else if(start >= find_vma->end)
            find_vma = find_vma->next;
        else {
            printf("vma address overflow\n");
            return NULL;
        }
    }
    struct vma* vma = (struct vma*)kalloc();
    if (NULL == vma) {
        printf("vma kalloc failed\n");
        return NULL;
    }
    if (0 != sz) {
        if (alloc) {
            if (0 != uvmalloc(p->pagetable,start,end,perm)) {
                printf("uvmalloc failed\n");
                kfree(vma);
                return NULL;
            }
        } else if (pa != 0) {
            if (0 != mappages(p->pagetable,start,sz,pa,perm)) {
                printf("mappages failed\n");
                kfree(vma);
                return NULL;
            }
        }
    }
    vma->addr = start;
    vma->sz = sz;
    vma->perm = perm;
    vma->end = end;
    vma->fd = -1;
    vma->f_off = 0;
    vma->type = type;
    vma->prev = find_vma->prev;
    vma->next = find_vma;
    find_vma->prev->next = vma;
    find_vma->prev = vma;
    
    return vma;
}

