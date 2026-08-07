#include "doomdef.h"
#include "d_ticcmd.h"
#include "d_event.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "d_net.h"
#include "g_game.h"
#include "m_misc.h"
#include "m_argv.h"

#include "koi.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"

/* Time, memory and stopping. */

extern int mb_used;

void I_Init(void) {
    I_InitSound();
}

/* The zone.
 *
 * DOOM takes one block at startup and does all its own allocation inside it -
 * which is why Koi-DOS needs no malloc for this to work, only a way to ask for
 * several megabytes at once. Six is the default the DOS version used and is
 * enough for the shareware and registered WADs.
 *
 * Asked of the system rather than of the library heap: the heap would take the
 * same pages and add a header, and this is the one allocation whose size is
 * known in advance and never freed. */
byte* I_ZoneBase(int* size) {
    byte* block;

    int megabytes = mb_used;
    int parameter = M_CheckParm("-mb");

    /* -mb, which the original parsed here and which is the only way to give a
       big WAD more room. */
    if (parameter && parameter < myargc - 1) {
        int asked = atoi(myargv[parameter + 1]);
        if (asked >= 2 && asked <= 64) megabytes = asked;
    }

    *size = megabytes * 1024 * 1024;
    block = (byte*)koi_alloc(*size);

    /* Ask for less rather than refuse. A machine that cannot spare six
       megabytes can still spare two, and DOOM runs in two - slowly, but it
       runs, which beats a message about memory on a system with 2 GB of it. */
    while (!block && *size > 2 * 1024 * 1024) {
        *size /= 2;
        block = (byte*)koi_alloc(*size);
    }
    if (!block) I_Error("Could not get %d bytes for the zone.", *size);
    printf("I_ZoneBase: %d MB at %p\n", *size / (1024 * 1024), block);
    return block;
}

/* The tick count, at DOOM's 35 per second.
 *
 * Taken from the system's millisecond clock, which is interrupt-driven and
 * monotonic. The whole game is paced off this: a clock that stalls makes the
 * game stall with it, and one that jumps makes it skip. */
int I_GetTime(void) {
    static koi_uint64 base;
    koi_uint64 now = koi_uptime();

    if (!base) base = now;
    return (int)(((now - base) * TICRATE) / 1000);
}

void I_StartFrame(void);
void I_StartTic(void);

ticcmd_t* I_BaseTiccmd(void) {
    static ticcmd_t empty;
    return &empty;
}

void I_Quit(void) {
    D_QuitNetGame();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults();
    I_ShutdownGraphics();
    exit(0);
}

/* A low allocation, which meant "below the first megabyte" on a DOS machine
   because that was where the sound hardware could reach. Nothing here has that
   constraint, so it is an ordinary allocation with the zeroing the callers
   expect. */
byte* I_AllocLow(int length) {
    byte* memory = (byte*)malloc(length);
    if (memory) memset(memory, 0, length);
    return memory;
}

void I_Tactile(int on, int off, int total) {
    (void)on; (void)off; (void)total;
    /* Force feedback, for hardware that never shipped. */
}

void I_Error(char* error, ...) {
    va_list arguments;
    char message[512];

    va_start(arguments, error);
    koi_vformat(message, sizeof(message), error, arguments);
    va_end(arguments);

    /* The screen comes back before the message goes out, or the message is
       printed onto a console nobody can see. */
    I_ShutdownGraphics();

    koi_print("\nDOOM stopped: ");
    koi_print(message);
    koi_print("\n");

    D_QuitNetGame();
    I_ShutdownSound();
    exit(-1);
}
