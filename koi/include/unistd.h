#ifndef KOI_SHIM_UNISTD_H
#define KOI_SHIM_UNISTD_H

/* The file calls DOOM makes by descriptor rather than by stream: the WAD
   reader uses open/lseek/read directly, which is why seeking had to exist
   before any of this could work. */
#include "koi.h"

/* Where a data file is, given its bare name: the current directory first, then
   beside the program. Not a unix call - it lives here because this is the
   header every file that opens a data file already includes. */
char* k_find_data(const char* name);
#include "stdlib.h"

#define SEEK_SET KOI_SEEK_SET
#define SEEK_CUR KOI_SEEK_CURRENT
#define SEEK_END KOI_SEEK_END

/* access() modes. Only existence can be answered - FAT keeps no permissions -
   so all four ask the same question. */
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

int open(const char* path, int flags, ...);
int close(int handle);
long read(int handle, void* buffer, koi_uint64 count);
long write(int handle, const void* buffer, koi_uint64 count);
long lseek(int handle, long offset, int whence);
int access(const char* path, int mode);
int unlink(const char* path);

#endif
