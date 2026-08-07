#ifndef KOI_SHIM_ERRNO_H
#define KOI_SHIM_ERRNO_H
/* One variable, so code that assigns to it compiles. Nothing here sets it to
   anything meaningful: the system calls report failure by returning -1 and
   have no numbering for why. Saying so beats inventing values. */
extern int errno;
#define EINTR 4
#define EAGAIN 11
#endif
