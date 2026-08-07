# K-DOOM

DOOM, running on [Koi-DOS](https://github.com/quartzz720/Koi-DOS).

id Software's source release, in `linuxdoom-1.10/`, with the four files that
talk to Unix replaced by four that talk to Koi-DOS. The game code is otherwise
id's, unchanged.

## ⚠ The WAD is not here and never will be

**`DOOM.WAD` is not in this repository.** The source is GPL-2.0; the game data
is not. It is still sold, and being easy to find is not the same as being free
to redistribute.

Bring your own. Put it next to `DOOM.EXE` on the Koi-DOS drive:

```
\DOOM\DOOM.EXE
\DOOM\DOOM.WAD
```

`.gitignore` refuses `*.WAD` so it cannot be committed by accident.

## Building

```bash
./build.sh          # produces DOOM.EXE
```

An ordinary x86-64 GCC. `sdk/` is a copy of the Koi-DOS SDK, so this builds
without the kernel checked out; refresh it with `cp ../Koi-DOS/sdk/* sdk/`.

## Running

Koi-DOS resolves a relative path from wherever the shell is standing, so a
program's data sits next to it and there is nothing to configure:

```
Z:\> cd \DOOM
Z:\DOOM> doom
```

`-warp 1 1` starts a level directly. `-mb 8` gives the zone more room.

## What the port is

`koi/` holds five files. Four replace id's Unix layer; the fifth is the piece
of C library the game expects and Koi-DOS does not have.

| | |
|---|---|
| `i_video.c` | the screen and the keyboard |
| `i_system.c` | time, the zone, and stopping |
| `i_sound.c` | stubs — there is no audio device yet |
| `i_net.c` | one player, and an honest refusal for `-net` |
| `k_stdio.c` | `stdio`, `unistd` and the odd corners of `stdlib` |
| `include/` | the standard headers, thin enough to read in one sitting |

**The screen.** DOOM draws 320×200 bytes of palette index; Koi-DOS hands out
32-bit pixels at whatever size the firmware chose. Every frame is a translation
and a scale in one pass over the destination. The scale is a whole number —
1280×800 gives exactly 4× — because a non-integer scale has to interpolate, and
interpolating means three reads per pixel with no floating point to do it in.
Only the picture is sent to the display, never the whole screen.

**No floating point at all.** Programs are compiled `-mgeneral-regs-only`,
because nothing configures SSE state after `ExitBootServices`. That turned out
to cost one line: every floating-point line in DOOM's portable code sits inside
`#if 0` or an `UNUSED` branch, abandoned when id moved the trigonometry into
`tables.c` — except `FixedDiv2`, where id had already written the 64-bit
integer version and left it switched off. Switching it on is both necessary and
better: integer division is exact where a double loses the low bits.

## What had to change in id's tree

Four things, each with the reason in a comment beside it.

**`m_fixed.c`** — `FixedDiv2` uses the integer path, as above.

**`r_data.c`** — two 64-bit bugs, and they present identically. Seven arrays
were allocated at four bytes an element; four of them hold pointers, so each
came out half the size it needed and filling one ran into the next zone block's
header. And `maptexture_t` is read straight out of the WAD but declared with a
pointer in it, which on a 64-bit machine moved `patchcount` and every patch
after it four bytes along.

**`m_misc.c`** — the configuration table stores either a small number or a
string literal's address in one `int` field, and an `int` holds half of the
second on a 64-bit machine. The field is pointer-sized now, and which of the
two an entry holds is decided by one function rather than by the same magnitude
test written out twice.

**`d_main.c`** — no environment to read `$HOME` or `$DOOMWADDIR` from, and a
forward slash is not a path separator on Koi-DOS. Plain names in the current
directory.

**`doomdef.h`** — `SNDSERV` off. It named a sound server that does not exist,
and it also put `mb_used` into the configuration table with a default of 2,
which quietly halved the zone before `Z_Init` ever ran.

## What this found in Koi-DOS

The port was meant to be a test of the system's program interface before it was
a game, and it was. In order:

**No way to seek in a file.** Every lump read in a WAD begins by jumping to an
offset. A file could be read front to back or not at all.

**No `printf` precision.** DOOM builds its font lump names with `"%.3d"` and
expects `STCFN033`. A formatter without precision produces `STCFN%.3d`, which
no WAD has ever contained.

**No delete, rename or `mkdir`.** The filesystem could do all three; none was
reachable from a program.

**A program's exit code shared a value with the loader's errors**, so a program
exiting `-1` — which DOOM does when it fails — was reported as "not a valid
Koi-DOS program".

**And the system call stub did not preserve the registers the ABI promised.**
The dispatcher is an ordinary C function and may clobber RCX, RDX, RSI, RDI and
R8–R11; none were saved. Nothing had noticed, because no program had ever kept
a live value in one of them across a call — until this one, compiled at `-O2`,
held a loop's destination pointer in R8, came back to find it holding 8, wrote
its palette to address 8 and never left the loop. That one is the reason a
port like this is worth doing.

## Licence

**GPL-2.0**, because id's source is. See [LICENSE.TXT](LICENSE.TXT) — the
relicensing is id's own commit, in the history of this repository.

That is not the licence Koi-DOS itself uses, and it does not need to be: the
Koi-DOS licence says in as many words that programs written for it are not
derived works of it. So a program may carry whatever licence its own code
requires, and this one's code requires GPL.
