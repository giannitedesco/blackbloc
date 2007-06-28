/*
* This file is part of blackbloc
* Copyright (c) 2003 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Altivec enhanced vector operations.
*/
#ifndef __VECTOR_ALTIVEC_HEADER_INCLUDED__
#define __VECTOR_ALTIVEC_HEADER_INCLUDED__

/* Do we dare allow the compiler to optimize?
 * Doesn't seem to make any difference...
*/
#define v_asm asm volatile

/* We need to tell the CPU which vector registers we
 * are going to use, we currently only use the first
 * three. v0 and v1 for source operands and v2 for the
 * destination operand. We do this in the constructor.
*/
#define ALTIVEC_REGISTERS ((1<<0)|(1<<1)|(1<<2))

/* This is safe, even if other libraries are using altivec.
 * We just 'or' in what we are using. The only time this will
 * fuck up is with multi-threading.
*/
#define V_CTOR
static inline void __vfunc
v_ctor()
{
	unsigned long val = ALTIVEC_REGISTERS;

	v_asm (
	"mfvrsave 4	\n"
	"ori 4,4,%0	\n"
	"mtvrsave 4	\n"
	:: "r" (val) );
}

#define V_ADD
static inline void __vfunc
v_add(vector_t d, const vector_t v1, const vector_t v2)
{
	v_asm (
	"lvx 0, %1@ha, %1@l	\n"
	"lvx 1, %2@ha, %2@l	\n"
	"vaddfp 2,0,1		\n"
	"stvx 2, %0@ha,%0@l	\n"
	:: "r"(d), "r" (v1), "r" (v2) );
}

#define V_SUB
static inline void __vfunc
v_sub(vector_t d, const vector_t v1, const vector_t v2)
{
	v_asm (
	"lvx 0, %1@ha, %1@l	\n"
	"lvx 1, %2@ha, %2@l	\n"
	"vsubfp 2,0,1		\n"
	"stvx 2, %0@ha,%0@l	\n"
	:: "r"(d), "r" (v1), "r" (v2) );
}

#endif /* __VECTOR_ALTIVEC_HEADER_INCLUDED__ */
