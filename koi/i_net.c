#include "doomdef.h"
#include "doomstat.h"
#include "i_net.h"
#include "d_net.h"
#include "m_argv.h"

/* Networking, once there is any.
 *
 * Koi-DOS has no network stack, so this reports one player and stops. That is
 * not a stub in the sense of pretending: `netgame` staying false is the honest
 * answer and the game plays a single-player game correctly on it.
 *
 * The one thing worth being careful about is what happens if somebody passes
 * -net anyway. Quietly ignoring it would start a single-player game while the
 * player waited for a friend who could never arrive, so it says so.
 */

void I_InitNetwork(void) {
    doomcom = malloc(sizeof(*doomcom));
    memset(doomcom, 0, sizeof(*doomcom));

    netgame = false;
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = 1;
    doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
    doomcom->ticdup = 1;
    doomcom->extratics = 0;

    if (M_CheckParm("-net"))
        I_Error("This system has no network yet, so -net cannot work.");
}

void I_NetCmd(void) {
    /* Reached only when doomcom->command is set, which needs more than one
       node - and there is never more than one. */
    I_Error("Tried to send a network packet with no network.");
}
