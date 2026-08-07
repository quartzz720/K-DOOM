#ifndef KOI_SHIM_STDLIB_H
#define KOI_SHIM_STDLIB_H

/* malloc, free, realloc, calloc, atoi, strtol and abs are all in the Koi-DOS
   library under those names already, so this only has to declare them and add
   the two the library has no reason to carry. */
#include "koi.h"

#define NULL ((void*)0)

typedef koi_uint64 size_t;

/* exit() is koi_exit(). DOOM calls it from I_Quit and from its error path,
   and the shell is what it returns to. */
void exit(int code);

/* DOOM asks for a random number in exactly one place - the sound code, which
   is stubbed - but m_random.c has its own generator and does not use this. It
   is here so that a file including stdlib.h and mentioning rand still builds. */
int rand(void);
void srand(unsigned int seed);

char* getenv(const char* name);

#endif
