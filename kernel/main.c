// Copyright (c) 2006-2019 Frans Kaashoek, Robert Morris, Russ Cox,
//                         Massachusetts Institute of Technology

#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/sbi.h"
#include "include/console.h"
#include "include/printf.h"
#include "include/kalloc.h"
#include "include/timer.h"
#include "include/trap.h"
#include "include/proc.h"
#include "include/plic.h"
#include "include/vm.h"
#include "include/disk.h"
#include "include/buf.h"
#ifndef QEMU
#include "include/sdcard.h"
#include "include/fpioa.h"
#include "include/dmac.h"
extern void _start(void);
#endif
extern void _entry(void);

static inline void inithartid(unsigned long hartid) {
  asm volatile("mv tp, %0" : : "r" (hartid & 0x1));
}

volatile static int started = 0;
static int first = 0;
extern void boot_stack(void);
extern void boot_stack_top(void);

void
main(unsigned long hartid, unsigned long dtb_pa)
{
  inithartid(hartid);
  
  if (first == 0) {
    first = 1;
    consoleinit();
    printfinit();   // init a lock for printf 
    print_logo();
    #ifdef DEBUG
    printf("hart %d enter main()...\n", hartid);
    #endif
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    timerinit();     // init a lock for timer
    trapinithart();  // install kernel trap vector, including interrupt handler
    procinit();
    plicinit();
    plicinithart();
    #ifdef k210
    fpioa_pin_init();
    dmac_init();
    #endif 
    disk_init();
    binit();         // buffer cache
    fileinit();      // file table
    userinit();      // first user process
    printf("hart %d init done\n", hartid);
    
    for(int i = 0; i < NCPU; i++) {
      if(i == hartid)
        continue;
      //sbi_send_ipi(mask, 1);
#ifdef QEMU
      sbi_hart_start(i, (unsigned long)_entry, 0);
#else
      sbi_hart_start(i, (unsigned long)_start, 0);
#endif
    }
    __sync_synchronize();
    started = 1;
  }
  else
  {
    // other hart 
    while (started == 0)
      ;
    __sync_synchronize();
    #ifdef DEBUG
    printf("hart %d enter main()...\n", hartid);
    #endif
    kvminithart();
    trapinithart();
    plicinithart();  // ask PLIC for device interrupts
    printf("hart 1 init done\n");
  }
  scheduler();
}
