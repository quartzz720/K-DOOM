#ifndef KOI_SHIM_MATH_H
#define KOI_SHIM_MATH_H
/* Deliberately empty.
 *
 * Two portable files include this and neither uses it: every floating-point
 * line in DOOM's portable code sits inside `#if 0` or an UNUSED branch, left
 * behind when id moved the tables into tables.c. There is no floating point
 * in this build at all - programs are compiled -mgeneral-regs-only, because
 * nothing configures SSE state after ExitBootServices - so a math.h that
 * declared anything would be promising what cannot be delivered. */
#endif
