#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"

#include "koi.h"

/* The entry point, on Koi-DOS.
 *
 * A Koi-DOS program is handed the rest of the command line as one string
 * rather than an argument vector, so this splits it - DOOM reads its options
 * with M_CheckParm and M_CheckParmWithArgs, both of which walk myargv.
 *
 * argv[0] is the program's own name because DOOM checks argc against 1 in
 * places, and a vector that started at the first real option would make every
 * one of those tests off by one.
 */

#define ARGUMENT_MAX 32
#define COMMAND_LINE_MAX 512

static char command_line[COMMAND_LINE_MAX];
static char* arguments[ARGUMENT_MAX];

static void split(const char* tail) {
    koi_uint64 length = 0;
    int count = 0;

    arguments[count++] = (char*)"doom";

    if (tail) {
        while (tail[length] && length + 1 < COMMAND_LINE_MAX) {
            command_line[length] = tail[length];
            length++;
        }
    }
    command_line[length] = 0;

    for (koi_uint64 index = 0; index < length && count < ARGUMENT_MAX; ) {
        while (index < length && command_line[index] == ' ') index++;
        if (index >= length) break;
        arguments[count++] = &command_line[index];
        while (index < length && command_line[index] != ' ') index++;
        if (index < length) command_line[index++] = 0;
    }

    myargc = count;
    myargv = arguments;
}

int main(const char* tail) {
    split(tail);
    D_DoomMain();
    return 0;
}
