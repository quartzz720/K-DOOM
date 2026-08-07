#include "doomdef.h"
#include "doomstat.h"
#include "i_sound.h"
#include "i_system.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_argv.h"

#include "koi.h"
#include "stdio.h"

/* Sound.
 *
 * Koi-DOS mixes; this file only decides what to hand it. That division is why
 * the file is short: there is no ring buffer here, no resampling, no clipping
 * and no callback, because all of that is in the kernel where every program
 * gets it. What is left is the part that is actually about DOOM - where a
 * sound lives in the WAD, what its bytes mean, and how the game's idea of
 * volume, stereo separation and pitch maps onto the mixer's.
 *
 * The original of this file had to do the mixing itself, in software, into a
 * buffer it wrote to /dev/dsp. That is where its eight-channel limit and its
 * hand-built step table came from. The step table is the one part worth
 * keeping.
 */

/* How much memory DOOM gets for its zone. Six megabytes is what the DOS build
   used and what the registered WAD wants; -mb on the command line overrides
   it. Defined here because the original i_sound.c defined it, which is where
   the configuration table still expects to find it. */
int mb_used = 6;

static int have_audio;

/* A sound effect in a WAD is a DMX lump: eight bytes of header and then
   unsigned 8-bit samples. Almost all of them are 11025 Hz; two in the
   registered WAD are 22050, which is exactly why the rate is read from each
   lump rather than assumed once and hardcoded. */
#define DMX_HEADER 8
#define DMX_FORMAT 3

/* DOOM's pitch, as a 16.16 multiplier on the sample rate.
 *
 * The game varies the pitch of nearly every sound by a random amount - the
 * chainsaw most obviously, but also every shot and every grunt - and without
 * it a firefight is the same four samples over and over. The original built
 * this with pow(2, (pitch-128)/64) at start-up; there is no floating point
 * here, so it is a table. 128 is 1.0, and the two ends are quarter speed and
 * just under four times.
 */
static const koi_uint32 steptable[256] = {
      16384,   16562,   16743,   16925,   17109,   17296,
      17484,   17674,   17867,   18061,   18258,   18457,
      18658,   18861,   19066,   19274,   19484,   19696,
      19911,   20127,   20347,   20568,   20792,   21019,
      21247,   21479,   21713,   21949,   22188,   22430,
      22674,   22921,   23170,   23423,   23678,   23936,
      24196,   24460,   24726,   24995,   25268,   25543,
      25821,   26102,   26386,   26674,   26964,   27258,
      27554,   27855,   28158,   28464,   28774,   29088,
      29405,   29725,   30048,   30376,   30706,   31041,
      31379,   31720,   32066,   32415,   32768,   33125,
      33486,   33850,   34219,   34591,   34968,   35349,
      35734,   36123,   36516,   36914,   37316,   37722,
      38133,   38548,   38968,   39392,   39821,   40255,
      40693,   41136,   41584,   42037,   42495,   42958,
      43425,   43898,   44376,   44859,   45348,   45842,
      46341,   46846,   47356,   47871,   48393,   48920,
      49452,   49991,   50535,   51085,   51642,   52204,
      52773,   53347,   53928,   54515,   55109,   55709,
      56316,   56929,   57549,   58176,   58809,   59449,
      60097,   60751,   61413,   62081,   62757,   63441,
      64132,   64830,   65536,   66250,   66971,   67700,
      68438,   69183,   69936,   70698,   71468,   72246,
      73032,   73828,   74632,   75444,   76266,   77096,
      77936,   78785,   79642,   80510,   81386,   82273,
      83169,   84074,   84990,   85915,   86851,   87796,
      88752,   89719,   90696,   91684,   92682,   93691,
      94711,   95743,   96785,   97839,   98905,   99982,
     101070,  102171,  103283,  104408,  105545,  106694,
     107856,  109031,  110218,  111418,  112631,  113858,
     115098,  116351,  117618,  118899,  120194,  121502,
     122825,  124163,  125515,  126882,  128263,  129660,
     131072,  132499,  133942,  135401,  136875,  138366,
     139872,  141395,  142935,  144491,  146065,  147655,
     149263,  150889,  152532,  154193,  155872,  157569,
     159285,  161019,  162773,  164545,  166337,  168148,
     169979,  171830,  173701,  175593,  177505,  179438,
     181392,  183367,  185364,  187382,  189423,  191485,
     193571,  195678,  197809,  199963,  202141,  204342,
     206567,  208816,  211090,  213389,  215712,  218061,
     220436,  222836,  225263,  227716,  230195,  232702,
     235236,  237798,  240387,  243005,  245651,  248326,
     251030,  253763,  256527,  259320
};

void I_InitSound(void) {
    have_audio = (int)koi_sysinfo(KOI_INFO_AUDIO, 0);
    if (!have_audio) {
        koi_print("Sound: no audio device, running silent.\n");
        return;
    }
    /* The sounds themselves are not loaded here. The original read all 122 of
       them up front - 1.2 MB into a 6 MB zone before the first level - because
       it needed them converted into its own mixing buffer's format. Nothing
       needs that now: a lump is cached the first time the game asks for it and
       then stays, which costs nothing at all for the sounds a session never
       happens to play. */
}

void I_ShutdownSound(void) {
    if (have_audio) koi_sound_stop(-1);
}

void I_SetChannels(void) {
    /* Nothing to set up. The mixer has its own voices and hands one out per
       sound; DOOM's channel bookkeeping above this is unaffected. */
}

void I_UpdateSound(void) {
    /* The mixer runs on the timer interrupt. There is nothing to submit and
       nothing to keep fed, which is the whole point of it being there. */
}

void I_SubmitSound(void) {
}

void I_SetSfxVolume(int volume) {
    snd_SfxVolume = volume;
}

void I_SetMusicVolume(int volume) {
    snd_MusicVolume = volume;
}

/* The lump for a sound, and its data alongside.
 *
 * Caching here rather than in I_StartSound is deliberate and not tidiness:
 * s_sound.c looks the lump up, then complains to stderr if `data` is still
 * null, and only then starts the sound. Filling both in one place means that
 * complaint never fires - otherwise every sound in the game prints a line of
 * text over whatever is on screen the first time it is heard.
 *
 * A missing sound falls back to the pistol rather than being fatal: the
 * shareware WAD does not contain every lump the sound table names, and
 * refusing to start over a sound effect would be a poor trade. */
int I_GetSfxLumpNum(sfxinfo_t* sfx) {
    char name[16];
    int lump;

    koi_snprintf(name, sizeof(name), "ds%s",
                 sfx->link ? sfx->link->name : sfx->name);
    lump = W_CheckNumForName(name);
    if (lump < 0) lump = W_GetNumForName("dspistol");

    if (!sfx->data && W_LumpLength(lump) > DMX_HEADER)
        sfx->data = W_CacheLumpNum(lump, PU_STATIC);
    return lump;
}

/* DOOM's volume, on the scale the mixer wants.
 *
 * The menu slider is 0 to 15 and everything below the mixer expects 0 to 127,
 * and this source release has the multiplication that bridges them commented
 * out. It is still there in d_main.c, as a commented-out times-eight inside
 * the call to S_Init, and again in m_menu.c beside S_SetSfxVolume. The effect
 * is a game eight times quieter than it should be:
 * measured, the pistol came out at 3.5% of full scale instead of 28%.
 *
 * Restoring it here rather than in id's tree, because snd_SfxVolume is also
 * what the options screen draws its thermometer from: multiply it there and
 * the slider reads 120 out of 16 and the bar runs off the end of the box.
 * This is the one place where the number is used as a volume rather than as a
 * position, so this is where the scale belongs.
 *
 * Separation needs no conversion at all: DOOM's 0 left to 255 right is the
 * mixer's pan already. */
static int scale_volume(int volume) {
    if (volume < 0) return 0;
    volume *= 8;
    return volume > 127 ? 127 : volume;
}

int I_StartSound(int id, int volume, int separation, int pitch,
                 int priority) {
    const unsigned char* lump;
    KOI_SOUND sound;
    unsigned int rate;
    unsigned int frames;
    unsigned int available;

    (void)priority;
    if (!have_audio) return -1;
    if (id < 1 || id >= NUMSFX) return -1;

    lump = (const unsigned char*)S_sfx[id].data;
    if (!lump) return -1;
    if ((lump[0] | (lump[1] << 8)) != DMX_FORMAT) return -1;

    rate = (unsigned int)(lump[2] | (lump[3] << 8));
    frames = (unsigned int)lump[4] | ((unsigned int)lump[5] << 8)
           | ((unsigned int)lump[6] << 16) | ((unsigned int)lump[7] << 24);

    /* The length in the header is not always the length of the lump. Trusting
       it would read past the end of one that had been truncated, and the
       mixer would go on playing whatever is next in the zone. */
    available = (unsigned int)W_LumpLength(S_sfx[id].lumpnum);
    if (available <= DMX_HEADER) return -1;
    available -= DMX_HEADER;
    if (!frames || frames > available) frames = available;

    if (pitch < 0) pitch = 0;
    if (pitch > 255) pitch = 255;
    rate = (unsigned int)(((koi_uint64)rate * steptable[pitch]) >> 16);
    if (!rate) return -1;

    volume = scale_volume(volume);
    if (separation < 0) separation = 0;
    if (separation > 255) separation = 255;

    sound.samples = lump + DMX_HEADER;
    sound.frames = frames;
    sound.rate = rate;
    sound.bits = KOI_SOUND_U8;
    sound.channels = 1;
    sound.volume = (unsigned short)(volume * 255 / 127);
    sound.pan = (unsigned short)separation;
    sound.loop = 0;
    sound.reserved = 0;
    return koi_sound_play(&sound);
}

void I_StopSound(int handle) {
    if (have_audio && handle >= 0) koi_sound_stop(handle);
}

int I_SoundIsPlaying(int handle) {
    if (!have_audio || handle < 0) return 0;
    return koi_sound_active(handle);
}

/* Called every tic for every sound whose source has moved relative to the
   player. Without it a rocket stays where it was fired. The pitch of a sound
   already playing is left alone - the mixer could retune a voice, but doing it
   mid-shot is a slide rather than a pitch, and DOOM only ever passes back the
   pitch it started with anyway. */
void I_UpdateSoundParams(int handle, int volume, int separation, int pitch) {
    (void)pitch;
    if (!have_audio || handle < 0) return;
    if (separation < 0) separation = 0;
    if (separation > 255) separation = 255;
    koi_sound_params(handle, scale_volume(volume) * 255 / 127, separation);
}

/* ---- Music ---------------------------------------------------------------
 *
 * Still silent, and not for want of a device. DOOM's music is MUS: a MIDI-like
 * event stream written for an OPL2 chip or a wavetable card, so playing it
 * needs a synthesiser rather than a mixer - an OPL emulator or a soundfont.
 * Neither is a small job, and neither is made any easier by pretending here to
 * have started it.
 */
void I_InitMusic(void) {
}

void I_ShutdownMusic(void) {
}

void I_PlaySong(int handle, int looping) {
    (void)handle; (void)looping;
}

void I_PauseSong(int handle) {
    (void)handle;
}

void I_ResumeSong(int handle) {
    (void)handle;
}

void I_StopSong(int handle) {
    (void)handle;
}

void I_UnRegisterSong(int handle) {
    (void)handle;
}

int I_RegisterSong(void* data) {
    (void)data;
    return 1;
}

int I_QrySongPlaying(int handle) {
    (void)handle;
    return 0;
}
