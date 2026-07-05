/* 0x00965 sub_00965 - format a value via sub_03c3b, draw its low byte as a
 *                     1-char string at x=200,y=arg, return the high byte. */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* RTL helper at 0x3c3b (outside the decompiled game range); takes one int arg,
 * returns a 16-bit value in AX. Declared extern; provided elsewhere. */
extern int sub_03c3b(int a0);

/* The original keeps a 2-byte string buffer on the stack (SS:[bp-2]).  Our
 * memory model only exposes DS[], and bgi_outtextxy() takes a dsptr, so we
 * reserve a private 2-byte scratch at the very top of DS (well above the
 * DGROUP data/heap) to hold that buffer. */
#define S965_BUF 0xfffcu

int sub_00965(int a0)
{
    int si;
    dsptr buf = S965_BUF;

    /* 0x0978: movedata copies the word at DS:0x2b0 (value 0x0000) into the
     * local buffer — i.e. zero-initialise both bytes. */
    movedata_ds(0x2b0, buf, 2);          /* buf[0]=0, buf[1]=0 */

    /* 0x0977..0x0985: si = sub_03c3b(0) */
    si = sub_03c3b(0);

    /* 0x0987..0x098c: store low byte (al) of si into buf[0] */
    B(buf) = (unsigned char)(si & 0xff);

    /* 0x098f..0x099b: putimage -> bgi_outtextxy(x=0xc8, y=a0, str=buf) */
    bgi_outtextxy(0xc8, a0, buf);        /* draws the 1-char NUL-terminated string */

    /* 0x09a3..0x09a8: return si >> 8 (signed arithmetic shift, i.e. high byte) */
    return (int)(short)si >> 8;
}
