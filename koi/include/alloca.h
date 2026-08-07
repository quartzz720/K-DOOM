#ifndef KOI_SHIM_ALLOCA_H
#define KOI_SHIM_ALLOCA_H
/* GCC's own, which needs no library behind it. */
#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif
#endif
