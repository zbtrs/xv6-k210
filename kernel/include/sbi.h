/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef _ASM_RISCV_SBI_H
#define _ASM_RISCV_SBI_H



#define SBI_TIMER_EXTION 0x54494D45
#define SBI_SET_TIMER 0

#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_IPI_EXTION 0x735049
#define SBI_SEND_IPI 0
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

#define SBI_HSM_EXTION 0x48534D
#define SBI_HART_START 0

#define SBI_CALL(eid, fid, arg0, arg1, arg2, arg3) ({		\
	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);	\
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);	\
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);	\
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);	\
	register uintptr_t a7 asm ("a7") = (uintptr_t)(eid);	\
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);	\
	asm volatile ("ecall"					\
		      : "+r" (a0), "+r" (a1)				\
		      : "r" (a6), "r" (a2), "r" (a3), "r" (a7)	\
		      : "memory");				\
	a0;							\
})

/* Lazy implementations until SBI is finalized */
#define SBI_CALL_0(eid, fid) SBI_CALL(eid, fid, 0, 0, 0, 0)
#define SBI_CALL_1(eid, fid, arg0) SBI_CALL(eid, fid, arg0, 0, 0, 0)
#define SBI_CALL_2(eid, fid, arg0, arg1) SBI_CALL(eid, fid, arg0, arg1, 0, 0)
#define SBI_CALL_3(eid, fid, arg0, arg1, arg2) SBI_CALL(eid, fid, arg0, arg1, arg2, 0)
#define SBI_CALL_4(eid, fid, arg0, arg1, arg2, arg3) SBI_CALL(eif, fid, arg0, arg1, arg2, arg3)

static inline void sbi_hart_start(unsigned long hartid, unsigned long start_addr, unsigned long opaque)
{
	SBI_CALL_3(SBI_HSM_EXTION, SBI_HART_START, hartid, start_addr, opaque);
}

static inline void sbi_console_putchar(int ch)
{
	SBI_CALL_1(SBI_CONSOLE_PUTCHAR, 0, ch);
}

static inline int sbi_console_getchar(void)
{
	return SBI_CALL_0(SBI_CONSOLE_GETCHAR, 0);
}

static inline void sbi_set_timer(uint64 stime_value)
{
	SBI_CALL_1(SBI_TIMER_EXTION, SBI_SET_TIMER, stime_value);
}

// static inline void sbi_shutdown(void)
// {
// 	SBI_CALL_0(SBI_SHUTDOWN);
// }

// static inline void sbi_clear_ipi(void)
// {
// 	SBI_CALL_0(SBI_CLEAR_IPI);
// }

static inline void sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
	SBI_CALL_2(SBI_IPI_EXTION, SBI_SEND_IPI, hart_mask, hart_mask_base);
}

// static inline void sbi_remote_fence_i(const unsigned long *hart_mask)
// {
// 	SBI_CALL_1(SBI_REMOTE_FENCE_I, hart_mask);
// }

// static inline void sbi_remote_sfence_vma(const unsigned long *hart_mask,
// 					 unsigned long start,
// 					 unsigned long size)
// {
// 	SBI_CALL_3(SBI_REMOTE_SFENCE_VMA, hart_mask, start, size);
// }

// static inline void sbi_remote_sfence_vma_asid(const unsigned long *hart_mask,
// 					      unsigned long start,
// 					      unsigned long size,
// 					      unsigned long asid)
// {
// 	SBI_CALL_4(SBI_REMOTE_SFENCE_VMA_ASID, hart_mask, start, size, asid);
// }

// static inline void sbi_set_extern_interrupt(unsigned long func_pointer) {
// 	asm volatile("mv a6, %0" : : "r" (0x210));
// 	SBI_CALL_1(0x0A000004, func_pointer);
// }

// static inline void sbi_set_mie(void) {
// 	SBI_CALL_0(0x0A000005);
// }

#endif
