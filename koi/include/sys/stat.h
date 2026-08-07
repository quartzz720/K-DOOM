#ifndef KOI_SHIM_SYS_STAT_H
#define KOI_SHIM_SYS_STAT_H

#include "koi.h"

/* The mode bits, which this build ignores: FAT has no permissions to set. */
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IROTH 0004
#define S_IWOTH 0002

/* Only the field anything here reads. DOOM calls fstat in two places and both
   want st_size and nothing else - the WAD reader to size its directory, and
   the screenshot code to check a file exists. Declaring the rest of a real
   struct stat would be promising values nothing can fill in: FAT has no inode,
   no link count and no owner. */
struct stat {
    long st_size;
    long st_mode;
};

int fstat(int handle, struct stat* out);
int stat(const char* path, struct stat* out);
int mkdir(const char* path, int mode);

#endif
