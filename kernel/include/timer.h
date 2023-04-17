#ifndef __TIMER_H
#define __TIMER_H

#include "types.h"
#include "spinlock.h"

extern struct spinlock tickslock;
extern uint ticks;

typedef struct TimeSpec {
    uint64 second;
    uint64 microSecond;    
} TimeSpec;

void timerinit();
void set_next_timeout();
void timer_tick();

#endif
