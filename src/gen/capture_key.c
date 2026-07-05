/* 0x00965 capture_key - format a value via dos_bioskey, draw its low byte as a
 *                     1-char string at x=200,y=arg, return the high byte. */
#include "dos.h"
#include "game_protos.h"

/* RTL helper at 0x3c3b (outside the decompiled game range); takes one int arg,
 * returns a 16-bit value in AX. Declared extern; provided elsewhere. */
extern int dos_bioskey(int a0);

/* The original keeps a 2-byte string buffer on the stack (SS:[bp-2]).  Our
 * memory model only exposes DS[], and bgi_outtextxy() takes a dsptr, so we
 * reserve a private 2-byte scratch at the very top of DS (well above the
 * DGROUP data/heap) to hold that buffer. */
#define S965_BUF 0xfffcu

int capture_key(int y)
{
    dsptr buf = S965_BUF;
    int value;

    /* Zero-initialise both bytes of the buffer by copying the word constant
     * (0x0000) that lives at DS:0x2b0. */
    movedata_ds(0x2b0, buf, 2);

    value = dos_bioskey(0);

    /* Drop the low byte in as a single NUL-terminated character and draw it. */
    B(buf) = (unsigned char)(value & 0xff);
    bgi_outtextxy(0xc8, y, buf);

    /* Return the high byte (arithmetic >>8, matching the signed SAR). */
    return (int)(short)value >> 8;
}
