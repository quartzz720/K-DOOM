#ifndef KOI_SHIM_STDIO_H
#define KOI_SHIM_STDIO_H

/* Enough stdio for DOOM, and no more.
 *
 * The portable code uses fprintf (47 times, nearly all to stderr), fopen,
 * fclose, fread, fseek, ftell, feof and one fscanf. Everything else that looks
 * like stdio here does not exist, on purpose: a header that declared the whole
 * of stdio and implemented a tenth of it would turn a link error into a
 * runtime surprise.
 *
 * There are no streams underneath. A FILE is a Koi-DOS file handle with a flag
 * saying whether the end has been reached, and stdout and stderr are two
 * sentinels that make writes go to the console.
 */

#include "koi.h"
#include "stdarg.h"
#include "stdlib.h"

#define EOF (-1)
#define SEEK_SET KOI_SEEK_SET
#define SEEK_CUR KOI_SEEK_CURRENT
#define SEEK_END KOI_SEEK_END

typedef struct {
    long handle;      /* -1 for the console sentinels */
    int console;
    int ended;
} FILE;

extern FILE* stdout;
extern FILE* stderr;
extern FILE* stdin;

FILE* fopen(const char* path, const char* mode);
int fclose(FILE* file);
koi_uint64 fread(void* buffer, koi_uint64 size, koi_uint64 count, FILE* file);
koi_uint64 fwrite(const void* buffer, koi_uint64 size, koi_uint64 count,
                  FILE* file);
int fseek(FILE* file, long offset, int whence);
long ftell(FILE* file);
int feof(FILE* file);
int fflush(FILE* file);

int fprintf(FILE* file, const char* format, ...);
int printf(const char* format, ...);
int sprintf(char* out, const char* format, ...);
int snprintf(char* out, koi_uint64 size, const char* format, ...);
int vsprintf(char* out, const char* format, va_list arguments);
int vfprintf(FILE* file, const char* format, va_list arguments);
int fputs(const char* text, FILE* file);
int puts(const char* text);
int putchar(int character);

/* One call site, in m_misc.c, reading two integers out of the config file.
   Only the conversions that call site uses are honoured. */
int fscanf(FILE* file, const char* format, ...);

int remove(const char* path);
int rename(const char* from, const char* to);
int getchar(void);
void setbuf(FILE* file, char* buffer);
int sscanf(const char* text, const char* format, ...);

#endif
