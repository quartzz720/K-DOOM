#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"

#include "koi.h"
#include "i_system.h"
#include "stdio.h"

/* The screen, on Koi-DOS.
 *
 * DOOM draws into one 320x200 byte array where each byte is an index into a
 * 256-entry palette. Koi-DOS hands out a framebuffer of 32-bit pixels at
 * whatever size the firmware chose. So every frame is a translation and a
 * scale, and doing both in one pass over the destination is what keeps it
 * cheap: for each output pixel, look up which source pixel it came from and
 * which colour that index currently means.
 *
 * The scale is an integer. A 320x200 picture stretched by a non-integer factor
 * has to interpolate or it shimmers as things move, and interpolating means
 * reading three pixels per output pixel with no floating point to do it in.
 * Whole multiples are sharp, cost one lookup, and look like what DOOM looked
 * like on the hardware it was written for.
 */

#define PALETTE_ENTRIES 256

static KOI_SCREEN screen;
static int entered;

static koi_uint32 palette[PALETTE_ENTRIES];

static int scale;          /* whole-number multiplier */
static int origin_x;       /* where the scaled picture sits on the screen */
static int origin_y;
static int scaled_width;
static int scaled_height;

/* DOOM's own gamma table, applied the way the X11 version applied it: the
   game changes `usegamma` from the options menu and expects the palette to
   brighten. */
extern int usegamma;
extern byte gammatable[5][256];

void I_ShutdownGraphics(void) {
    if (!entered) return;
    koi_gfx_leave();
    entered = 0;
}

void I_InitGraphics(void) {
    int by_width;
    int by_height;

    if (entered) return;
    if (koi_gfx_enter(&screen) != 0)
        I_Error("This system has no framebuffer to draw on.");
    entered = 1;

    /* The largest whole multiple that still fits, and never zero: a screen
       too small for even 1:1 gets 1:1 and is clipped, which is better than
       refusing to start on a machine somebody is sitting in front of. */
    by_width = (int)screen.width / SCREENWIDTH;
    by_height = (int)screen.height / SCREENHEIGHT;
    scale = by_width < by_height ? by_width : by_height;
    if (scale < 1) scale = 1;

    scaled_width = SCREENWIDTH * scale;
    scaled_height = SCREENHEIGHT * scale;
    origin_x = ((int)screen.width - scaled_width) / 2;
    origin_y = ((int)screen.height - scaled_height) / 2;
    if (origin_x < 0) origin_x = 0;
    if (origin_y < 0) origin_y = 0;

    /* Black everything once. Only the picture is sent after this, and the
       console's text would otherwise still be sitting in the margins. */
    koi_gfx_clear(koi_gfx_color(0, 0, 0));
    koi_gfx_present();

    /* Until the game loads PLAYPAL, a grey ramp - so a failure before the
       first I_SetPalette shows something rather than a black screen that
       cannot be told from a hang. */
    for (int index = 0; index < PALETTE_ENTRIES; index++)
        palette[index] = koi_gfx_color(index, index, index);
}

void I_SetPalette(byte* doompalette) {
    byte* gamma = gammatable[usegamma];

    for (int index = 0; index < PALETTE_ENTRIES; index++) {
        int red = gamma[*doompalette++];
        int green = gamma[*doompalette++];
        int blue = gamma[*doompalette++];
        /* Never assembled by hand: the framebuffer's channel order differs
           between machines and code that guesses is wrong on half of them. */
        palette[index] = koi_gfx_color(red, green, blue);
    }
}

void I_UpdateNoBlit(void) {
}

void I_FinishUpdate(void) {
    const byte* source = screens[0];
    koi_uint8* base;


    if (!entered) return;
    base = (koi_uint8*)screen.pixels;

    /* One pass over the destination, writing straight into the buffer. Going
       through the drawing calls would be a system call per pixel, and there
       are up to two million of them a frame. */
    for (int y = 0; y < scaled_height; y++) {
        int target_y = origin_y + y;
        const byte* row;
        koi_uint32* out;

        if (target_y >= (int)screen.height) break;
        row = source + (y / scale) * SCREENWIDTH;
        out = (koi_uint32*)(base + (koi_uint64)target_y * screen.pitch)
              + origin_x;

        if (scale == 1) {
            for (int x = 0; x < SCREENWIDTH; x++) out[x] = palette[row[x]];
        } else {
            int at = 0;
            for (int x = 0; x < SCREENWIDTH; x++) {
                koi_uint32 colour = palette[row[x]];
                for (int repeat = 0; repeat < scale; repeat++) out[at++] = colour;
            }
        }
    }

    /* Only the picture, never the whole screen. The margins never change and
       sending them would cost more than drawing the frame did. */
    koi_gfx_present_rect(origin_x, origin_y, scaled_width, scaled_height);
}

void I_ReadScreen(byte* destination) {
    memcpy(destination, screens[0], SCREENWIDTH * SCREENHEIGHT);
}

void I_WaitVBL(int count) {
    /* There is no vertical blank to wait for - the firmware left a linear
       buffer and no way to ask the display where its beam is. Sleeping for
       the time a frame would have taken is what the call is used for anyway:
       the wipe effect paces itself with it. */
    koi_sleep(count * 1000 / 70);
}

void I_BeginRead(void) {
}

void I_EndRead(void) {
}

/* ---- Input --------------------------------------------------------------
 *
 * DOOM wants key down and key up, which is exactly what SYS_KEYEVENT reports
 * and exactly what a character stream could not. Its own key numbering is
 * mostly ASCII with named codes above 0x80, so this is a translation table
 * rather than a state machine.
 */
static int translate(int key) {
    switch (key) {
    case KOI_KEY_UP: return KEY_UPARROW;
    case KOI_KEY_DOWN: return KEY_DOWNARROW;
    case KOI_KEY_LEFT: return KEY_LEFTARROW;
    case KOI_KEY_RIGHT: return KEY_RIGHTARROW;
    case KOI_KEY_ESCAPE: return KEY_ESCAPE;
    case '\n': case '\r': return KEY_ENTER;
    case '\t': return KEY_TAB;
    case '\b': return KEY_BACKSPACE;
    case KOI_KEY_SHIFT: return KEY_RSHIFT;
    case KOI_KEY_CONTROL: return KEY_RCTRL;
    case KOI_KEY_ALT: return KEY_RALT;
    default: break;
    }
    if (key >= KOI_KEY_F1 && key <= KOI_KEY_F1 + 11)
        return KEY_F1 + (key - KOI_KEY_F1);
    if (key >= 32 && key < 127) return key;   /* already what DOOM expects */
    return 0;
}

void I_StartTic(void) {
    int raw;

    while ((raw = koi_keyevent()) != 0) {
        event_t event;
        int key = translate(KOI_KEY_CODE(raw));

        if (!key) continue;
        event.type = KOI_KEY_IS_RELEASE(raw) ? ev_keyup : ev_keydown;
        event.data1 = key;
        D_PostEvent(&event);
    }
}

void I_StartFrame(void) {
    /* No mouse and no joystick to gather. */
}
