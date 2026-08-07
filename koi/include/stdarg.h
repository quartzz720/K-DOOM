#ifndef KOI_SHIM_STDARG_H
#define KOI_SHIM_STDARG_H
/* The compiler provides these; there is nothing for a library to do. */
typedef __builtin_va_list va_list;
#define va_start(list, last) __builtin_va_start(list, last)
#define va_arg(list, type) __builtin_va_arg(list, type)
#define va_end(list) __builtin_va_end(list)
#define va_copy(to, from) __builtin_va_copy(to, from)
#endif
