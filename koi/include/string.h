#ifndef KOI_SHIM_STRING_H
#define KOI_SHIM_STRING_H
/* The Koi-DOS library already uses these names, so this only declares them. */
#include "koi.h"

int strcasecmp(const char* left, const char* right);
int strncasecmp(const char* left, const char* right, koi_uint64 count);
#endif
