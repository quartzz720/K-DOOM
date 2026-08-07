#ifndef KOI_SHIM_FCNTL_H
#define KOI_SHIM_FCNTL_H

/* Only the three combinations DOOM opens files with. The numbers are the
   usual ones so that code writing O_RDONLY|O_BINARY still means what it says;
   there is no text mode here, so O_BINARY is zero. */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  0100
#define O_TRUNC  01000
#define O_BINARY 0

#include "unistd.h"

#endif
