#!/usr/bin/env bash
# Build DOOM.EXE for Koi-DOS.
#
# id's tree in linuxdoom-1.10/ is compiled as it stands, minus the four files
# that talk to Unix - i_video, i_sound, i_net and i_main - which koi/ replaces.
# Keeping the split that way means `git diff` against id's history shows the
# port rather than burying it in 45,000 lines of untouched game code.
#
# The compiler is invoked directly rather than through sdk/koicc. koicc copies
# its sources into one directory and is meant for a program of a few files;
# 130 of them across three trees is not what it is for. The flags below are
# the ones it uses, spelled out.
set -euo pipefail

cd "$(dirname "$0")"

DOOM=linuxdoom-1.10
PORT=koi
CC=${CC:-gcc}

# The four Unix-specific files, replaced rather than patched.
REPLACED="i_video.c i_sound.c i_net.c i_main.c i_system.c"

SOURCES=()
for source in "$DOOM"/*.c; do
    name=$(basename "$source")
    case " $REPLACED " in *" $name "*) continue;; esac
    SOURCES+=("$source")
done
SOURCES+=("$PORT"/i_video.c "$PORT"/i_sound.c "$PORT"/i_net.c "$PORT"/i_main.c)
SOURCES+=("$PORT"/i_system.c)
SOURCES+=("$PORT"/k_stdio.c)
SOURCES+=(sdk/start.c sdk/koilib.c)

echo "Building ${#SOURCES[@]} source files"

# Include order matters. koi/include holds the standard-header shims, which
# have to be found before anything else; sdk/ holds koi.h, which those shims
# include; linuxdoom-1.10 holds the game's own headers.
INCLUDES="-I $PORT/include -I sdk -I $DOOM"

# 1997 C compiled by a 2026 compiler. The warnings are about style that was
# ordinary then - implicit int, missing prototypes between translation units,
# parentheses around assignments - and changing 45,000 lines to quieten them
# would bury the port in noise and risk breaking a game that works. The port's
# own files are held to the usual standard; see the note in koi/README.
QUIET="-Wno-implicit-function-declaration -Wno-implicit-int
       -Wno-parentheses -Wno-unused-variable -Wno-unused-but-set-variable
       -Wno-misleading-indentation -Wno-format-overflow -Wno-stringop-overflow
       -Wno-stringop-truncation -Wno-array-bounds -Wno-sign-compare
       -Wno-builtin-declaration-mismatch -Wno-discarded-qualifiers
       -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types"

# -mgeneral-regs-only forbids SSE, because nothing configures SSE state after
# ExitBootServices. That is why FixedDiv2 uses the integer path id left in it.
$CC \
    -ffreestanding -fno-stack-protector -fno-stack-check -fno-builtin \
    -fno-pie -mno-red-zone -mgeneral-regs-only \
    -fno-asynchronous-unwind-tables \
    -ffunction-sections -fdata-sections \
    -std=gnu99 -O2 \
    -DNORMALUNIX -Dalloca=__builtin_alloca \
    $QUIET \
    $INCLUDES \
    -nostdlib -no-pie -Wl,-T,sdk/program.ld -Wl,--build-id=none \
    -Wl,-z,max-page-size=0x1000 -Wl,-z,noexecstack -Wl,--gc-sections \
    -o DOOM.EXE "${SOURCES[@]}"

ABI=$(sed -n 's/^#define KOI_ABI_VERSION \([0-9]*\).*/\1/p' sdk/syscall.h)
SIZE=$(stat -c %s DOOM.EXE)

echo
echo "DOOM.EXE  ${SIZE} bytes  (interface version $ABI)"
echo
echo "It needs a WAD, which is not in this repository and never will be: the"
echo "source is GPL-2.0, the game data is not. Put your own next to it."
echo
echo "    mcopy -o -i esp.img DOOM.EXE ::/BIN/"
echo "    mcopy -o -i esp.img DOOM.WAD ::/"
