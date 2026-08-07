// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Fixed point implementation.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";

#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#ifdef __GNUG__
#pragma implementation "m_fixed.h"
#endif
#include "m_fixed.h"




// Fixme. __USE_C_FIXED__ or something.

fixed_t
FixedMul
( fixed_t	a,
  fixed_t	b )
{
    return ((long long) a * (long long) b) >> FRACBITS;
}



//
// FixedDiv, C version.
//

fixed_t
FixedDiv
( fixed_t	a,
  fixed_t	b )
{
    if ( (abs(a)>>14) >= abs(b))
	return (a^b)<0 ? MININT : MAXINT;
    return FixedDiv2 (a,b);
}



fixed_t
FixedDiv2
( fixed_t	a,
  fixed_t	b )
{
    // Koi-DOS: the integer path, which id wrote here and left switched off.
    //
    // This is the only floating point left anywhere in DOOM's portable code -
    // everything else sits inside #if 0 or an UNUSED branch, abandoned when
    // the tables moved into tables.c. Programs here are compiled
    // -mgeneral-regs-only, because nothing configures SSE state after
    // ExitBootServices, so the double version cannot be built at all.
    //
    // The two are not merely equivalent, this one is better: 64-bit integer
    // division is exact where a double loses the low bits of a large fixed_t.
    long long c;

    if (!b)
	I_Error("FixedDiv: divide by zero");
    c = ((long long)a<<16) / ((long long)b);
    return (fixed_t) c;
}
