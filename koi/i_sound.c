#include "doomdef.h"
#include "doomstat.h"
#include "i_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"

#include "koi.h"

/* Sound, once there is somewhere to send it.
 *
 * Koi-DOS has no audio driver yet, so every one of these does nothing. They
 * are written out in full rather than left to a linker error because the game
 * calls them constantly - on every weapon fire, every door, every step - and a
 * port that cannot start because a stub is missing tells you nothing about
 * whether the rest works.
 *
 * The shape is kept honest: handles are still issued and still distinguishable,
 * so the game's own bookkeeping about which channel is playing what behaves
 * exactly as it will when there is a driver underneath. When HD Audio or AC'97
 * arrives, this file is where it goes and nothing above it changes.
 */

/* How much memory DOOM gets for its zone. Six megabytes is what the DOS build
   used and what the registered WAD wants; -mb on the command line overrides
   it. Defined here because the original i_sound.c defined it, which is where
   the configuration table still expects to find it. */
int mb_used = 6;

/* snd_SfxVolume and snd_MusicVolume live in s_sound.c, which is portable and
   compiled as it stands. */

void I_InitSound(void) {
    koi_print("Sound: no audio device on this system, running silent.\n");
}

void I_ShutdownSound(void) {
}

void I_SetChannels(void) {
}

void I_UpdateSound(void) {
}

void I_SubmitSound(void) {
}

void I_SetSfxVolume(int volume) {
    snd_SfxVolume = volume;
}

void I_SetMusicVolume(int volume) {
    snd_MusicVolume = volume;
}

/* The lump number is real even with no device: the game looks it up to decide
   whether a sound exists at all, and answering wrongly changes behaviour. */
int I_GetSfxLumpNum(sfxinfo_t* sfx) {
    char name[9];
    koi_snprintf(name, sizeof(name), "ds%s", sfx->name);
    return W_GetNumForName(name);
}

static int next_handle = 1;

int I_StartSound(int id, int volume, int separation, int pitch, int priority) {
    (void)id; (void)volume; (void)separation; (void)pitch; (void)priority;
    /* A handle that is never the same twice, so the caller's "is this still
       the sound I started" test means what it will mean later. */
    return next_handle++;
}

void I_StopSound(int handle) {
    (void)handle;
}

int I_SoundIsPlaying(int handle) {
    (void)handle;
    /* Nothing is playing, ever. Saying otherwise would leave channels the
       game thinks are busy for the rest of the level. */
    return 0;
}

void I_UpdateSoundParams(int handle, int volume, int separation, int pitch) {
    (void)handle; (void)volume; (void)separation; (void)pitch;
}

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
