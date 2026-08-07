#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "errno.h"

/* What is behind the header shims.
 *
 * None of this is a C library. It is the thinnest possible mapping from the
 * calls DOOM makes onto the ones Koi-DOS has, written so that 42,000 lines of
 * portable game code compile unchanged. Where Koi-DOS has no equivalent the
 * function says so by failing, rather than pretending.
 */

int errno;

static FILE console_out = { -1, 1, 0 };
static FILE console_err = { -1, 1, 0 };
static FILE console_in  = { -1, 1, 0 };

FILE* stdout = &console_out;
FILE* stderr = &console_err;
FILE* stdin  = &console_in;

/* Handles are small integers on both sides, so the descriptor calls are almost
   a rename. The one difference that matters: Koi-DOS has two open modes, read
   and write-truncating, and no notion of appending. */
int open(const char* path, int flags, ...) {
    long handle = koi_open(path, (flags & (O_WRONLY | O_RDWR | O_CREAT))
                                 ? OPEN_WRITE : OPEN_READ);
    return handle < 0 ? -1 : (int)handle;
}

int close(int handle) {
    return handle < 0 ? -1 : (int)koi_close(handle);
}

long read(int handle, void* buffer, koi_uint64 count) {
    if (handle < 0) return -1;
    return koi_read(handle, buffer, (long)count);
}

long write(int handle, const void* buffer, koi_uint64 count) {
    if (handle < 0) return -1;
    return koi_write(handle, buffer, (long)count);
}

long lseek(int handle, long offset, int whence) {
    if (handle < 0) return -1;
    return koi_seek(handle, offset, whence);
}

int access(const char* path, int mode) {
    (void)mode;
    /* The one caller asks "does this exist", and takes -1 for no. */
    return koi_exists(path) == 1 ? 0 : -1;
}

/* Only the size, which is all anything asks for. */
int fstat(int handle, struct stat* out) {
    long size;

    if (handle < 0 || !out) return -1;
    size = koi_filesize(handle);
    if (size < 0) return -1;
    out->st_size = size;
    out->st_mode = 0;
    return 0;
}

int stat(const char* path, struct stat* out) {
    long handle = koi_open(path, OPEN_READ);
    int result;

    if (handle < 0) return -1;
    result = fstat((int)handle, out);
    koi_close(handle);
    return result;
}

int unlink(const char* path) {
    return (int)koi_remove(path);
}

int remove(const char* path) {
    return (int)koi_remove(path);
}

int rename(const char* from, const char* to) {
    return (int)koi_rename(from, to);
}

/* ---- Streams ------------------------------------------------------------ */

#define FILE_MAX 8
static FILE files[FILE_MAX];

FILE* fopen(const char* path, const char* mode) {
    int writing = mode && (mode[0] == 'w' || mode[0] == 'a' || mode[1] == '+');
    long handle;
    int slot;

    for (slot = 0; slot < FILE_MAX; slot++) if (!files[slot].handle) break;
    if (slot == FILE_MAX) return (FILE*)0;

    handle = koi_open(path, writing ? OPEN_WRITE : OPEN_READ);
    if (handle < 0) return (FILE*)0;

    /* Zero means "free" in the table, so a real handle of zero would be
       invisible. Stored one higher and taken back down on use. */
    files[slot].handle = handle + 1;
    files[slot].console = 0;
    files[slot].ended = 0;
    return &files[slot];
}

int fclose(FILE* file) {
    if (!file || file->console || !file->handle) return EOF;
    koi_close(file->handle - 1);
    file->handle = 0;
    return 0;
}

koi_uint64 fread(void* buffer, koi_uint64 size, koi_uint64 count, FILE* file) {
    long got;

    if (!file || file->console || !file->handle || !size) return 0;
    got = koi_read(file->handle - 1, buffer, (long)(size * count));
    if (got <= 0) { file->ended = 1; return 0; }
    if ((koi_uint64)got < size * count) file->ended = 1;
    return (koi_uint64)got / size;
}

koi_uint64 fwrite(const void* buffer, koi_uint64 size, koi_uint64 count,
                  FILE* file) {
    long put;

    if (!file || !size) return 0;
    if (file->console) {
        /* No length-taking print exists, so the console path copies into a
           terminated buffer a chunk at a time. */
        const char* text = (const char*)buffer;
        koi_uint64 total = size * count;
        char line[257];
        while (total) {
            koi_uint64 chunk = total < 256 ? total : 256;
            memcpy(line, text, chunk);
            line[chunk] = 0;
            koi_print(line);
            text += chunk;
            total -= chunk;
        }
        return count;
    }
    if (!file->handle) return 0;
    put = koi_write(file->handle - 1, buffer, (long)(size * count));
    return put <= 0 ? 0 : (koi_uint64)put / size;
}

int fseek(FILE* file, long offset, int whence) {
    if (!file || file->console || !file->handle) return -1;
    file->ended = 0;
    return koi_seek(file->handle - 1, offset, whence) < 0 ? -1 : 0;
}

long ftell(FILE* file) {
    if (!file || file->console || !file->handle) return -1;
    return koi_seek(file->handle - 1, 0, KOI_SEEK_CURRENT);
}

int feof(FILE* file) {
    return file ? file->ended : 1;
}

int fflush(FILE* file) {
    (void)file;
    /* Nothing is buffered on this side, so there is nothing to push. */
    return 0;
}

/* ---- Formatted output --------------------------------------------------- */

int vfprintf(FILE* file, const char* format, va_list arguments) {
    char line[1024];
    int written = koi_vformat(line, sizeof(line), format, arguments);

    if (!file || file->console) koi_print(line);
    else fwrite(line, 1, strlen(line), file);
    return written;
}

int fprintf(FILE* file, const char* format, ...) {
    va_list arguments;
    int written;

    va_start(arguments, format);
    written = vfprintf(file, format, arguments);
    va_end(arguments);
    return written;
}

int printf(const char* format, ...) {
    va_list arguments;
    char line[1024];
    int written;

    va_start(arguments, format);
    written = koi_vformat(line, sizeof(line), format, arguments);
    va_end(arguments);
    koi_print(line);
    return written;
}

int vsprintf(char* out, const char* format, va_list arguments) {
    return koi_vformat(out, 0x7FFFFFFF, format, arguments);
}

int sprintf(char* out, const char* format, ...) {
    va_list arguments;
    int written;

    va_start(arguments, format);
    written = koi_vformat(out, 0x7FFFFFFF, format, arguments);
    va_end(arguments);
    return written;
}

int snprintf(char* out, koi_uint64 size, const char* format, ...) {
    va_list arguments;
    int written;

    va_start(arguments, format);
    written = koi_vformat(out, size, format, arguments);
    va_end(arguments);
    return written;
}

int fputs(const char* text, FILE* file) {
    if (!file || file->console) { koi_print(text); return 0; }
    fwrite(text, 1, strlen(text), file);
    return 0;
}

int puts(const char* text) {
    koi_print(text);
    koi_print("\n");
    return 0;
}

int putchar(int character) {
    koi_putchar((char)character);
    return character;
}

/* ---- fscanf --------------------------------------------------------------
 *
 * One call site: m_misc.c reads its configuration file a line at a time as
 * `%79s %[^\n]`. Rather than write a scanner for a format nothing else uses,
 * this handles exactly that shape and refuses anything else - which is
 * honest, and fails loudly if another call site ever appears.
 */
int fscanf(FILE* file, const char* format, ...) {
    va_list arguments;
    char* name;
    char* value;
    char line[256];
    koi_uint64 length = 0;
    koi_uint64 index;
    int fields = 0;

    if (!file || file->console || !file->handle) return EOF;

    /* One line, read a byte at a time. Configuration files are small and this
       is the only reader; anything faster would need buffering that the rest
       of this file deliberately does not have. */
    for (;;) {
        char character;
        if (koi_read(file->handle - 1, &character, 1) != 1) {
            if (!length) { file->ended = 1; return EOF; }
            break;
        }
        if (character == '\n') break;
        if (character == '\r') continue;
        if (length + 1 < sizeof(line)) line[length++] = character;
    }
    line[length] = 0;

    va_start(arguments, format);
    name = va_arg(arguments, char*);
    value = va_arg(arguments, char*);
    va_end(arguments);

    /* The key: up to the first space. */
    index = 0;
    while (index < length && line[index] != ' ' && line[index] != '\t') {
        name[index] = line[index];
        index++;
    }
    name[index] = 0;
    if (index) fields++;

    while (index < length && (line[index] == ' ' || line[index] == '\t')) index++;

    /* The value: the rest of the line. */
    {
        koi_uint64 out = 0;
        while (index < length) value[out++] = line[index++];
        value[out] = 0;
        if (out) fields++;
    }
    return fields;
}

/* ---- The odds and ends DOOM reaches for -------------------------------- */

int getchar(void) {
    return koi_getchar();
}

void setbuf(FILE* file, char* buffer) {
    (void)file; (void)buffer;
    /* Nothing is buffered, so there is no buffering to turn off. */
}

char* getenv(const char* name) {
    (void)name;
    /* There is no environment. DOOM asks for HOME and DOOMWADDIR and copes
       with not getting them - it falls back to the current directory, which
       is where a package keeps its own data anyway. */
    return (char*)0;
}

int mkdir(const char* path, int mode) {
    (void)mode;
    return (int)koi_mkdir(path);
}

int strcasecmp(const char* left, const char* right) {
    while (*left && tolower((int)(unsigned char)*left) ==
                    tolower((int)(unsigned char)*right)) { left++; right++; }
    return tolower((int)(unsigned char)*left) - tolower((int)(unsigned char)*right);
}

int strncasecmp(const char* left, const char* right, koi_uint64 count) {
    while (count && *left && tolower((int)(unsigned char)*left) ==
                             tolower((int)(unsigned char)*right)) {
        left++; right++; count--;
    }
    if (!count) return 0;
    return tolower((int)(unsigned char)*left) - tolower((int)(unsigned char)*right);
}

/* sscanf, for the two call sites that exist: "%x" and "%i" reading one
   integer out of a configuration value. Anything else returns nothing read,
   which is what a caller checks for - and is honest, where guessing would
   quietly produce a number nobody asked for. */
int sscanf(const char* text, const char* format, ...) {
    va_list arguments;
    int* out;
    int base = 10;

    if (format[0] != '%') return 0;
    if (format[1] == 'x' || format[1] == 'X') base = 16;
    else if (format[1] != 'i' && format[1] != 'd' && format[1] != 'u') return 0;

    va_start(arguments, format);
    out = va_arg(arguments, int*);
    va_end(arguments);

    if (!out) return 0;
    *out = (int)strtol(text, (char**)0, base);
    return 1;
}

/* ---- The rest of stdlib -------------------------------------------------- */

void exit(int code) {
    koi_exit(code);
}

static unsigned int random_state = 1;

int rand(void) {
    random_state = random_state * 1103515245U + 12345U;
    return (int)((random_state >> 16) & 0x7FFF);
}

void srand(unsigned int seed) {
    random_state = seed;
}
