/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

/* TODO: There are no code/functions to (re)set these variables. Odd? */
static unsigned CurrentCursorX;
static unsigned CurrentCursorY;
static unsigned CurrentAttr = 0x0f;

VOID
XboxConsPutChar(int c)
{
    ULONG Height, Unused, Width;

    XboxVideoGetDisplaySize(&Width, &Height, &Unused);

    if (CurrentCursorY >= Height)
    {
        /* HACK: Check what Windows does. (Ignore character? Scroll the screen?) */
        XboxVideoPutChar('.', CurrentAttr, Width - 3, Height - 1);
        XboxVideoPutChar('.', CurrentAttr, Width - 2, Height - 1);
        XboxVideoPutChar('.', CurrentAttr, Width - 1, Height - 1);
//          ASSERTMSG("Trying to print text out of screen\n", FALSE);
//          FIXME("Trying to print text out of screen\n");
        return;
    }

    /* Special cases: formatting characters, not to be printed as is. */
    /* TODO: Should any of the 3 special cases call XboxVideoPutChar()? Or print some spaces? */
    /* TODO: Check what Windows does. */
    if (c == '\r')
    {
        CurrentCursorX = 0;
        return;
    }

    if (c == '\n')
    {
        CurrentCursorX = 0;
        CurrentCursorY++;
        return;
    }

    if (c == '\t')
    {
        CurrentCursorX = (CurrentCursorX + 8) & ~7;
    }
    else
    {
        XboxVideoPutChar(c, CurrentAttr, CurrentCursorX++, CurrentCursorY);
    }

    if (CurrentCursorX >= Width)
    {
        /* HACK: Check what Windows does. (Ignore character? '\r'?) */
        CurrentCursorX = 0;
        CurrentCursorY++;
    }
//    // FIXME: Implement vertical screen scrolling if we are at the end of the screen.
}

BOOLEAN
XboxConsKbHit(VOID)
{
    /* No keyboard support yet */
    return FALSE;
}

int
XboxConsGetCh(void)
{
    /* No keyboard support yet */
    while (1) ;

    return 0;
}

/* EOF */
