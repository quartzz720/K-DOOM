#ifndef KOI_SHIM_TIME_H
#define KOI_SHIM_TIME_H
#include "koi.h"
/* DOOM only wants a clock to seed its random number generator and to pace
   itself; koi_uptime is both, and I_GetTime is what the game actually calls. */
#endif
