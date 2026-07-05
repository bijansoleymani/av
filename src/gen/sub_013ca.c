/* 0x013ca sub_013ca - draw edge/wall sprites when a clamped (x,y) point nears the four borders */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_013ca(int a0, int a1)   /* a0=[bp+8], a1=[bp+0xa] */
{
    int si;   /* 013cf: si = a0 - 4  */
    int di;   /* 013d5: di = a1 - 5  */

    si = a0 - 4;
    di = a1 - 5;

    /* 013db: clamp si to [4, 0x110] */
    if (si < 4)      si = 4;        /* jge 0x13e3 */
    if (si > 0x110)  si = 0x110;    /* jle 0x13ec */

    /* 013ec: clamp di to [0xb, 0xa7] */
    if (di < 0xb)    di = 0xb;      /* jge 0x13f4 */
    if (di > 0xa7)   di = 0xa7;     /* jle 0x13fd */

    /* 013fd: near left edge -> draw at x=0 */
    if (a0 < 8) {                   /* jge 0x1410 skips */
        draw_sprite(0, di, IMG(0x9da));
    }

    /* 01410: near right edge -> draw at x=0x138 */
    if (a0 > 0x117) {               /* jle 0x1425 skips */
        draw_sprite(0x138, di, IMG(0xa38));
    }

    /* 01425: near top edge -> draw at y=0xb */
    if (a1 < 0x11) {                /* jge 0x1439 skips */
        draw_sprite(si, 0xb, IMG(0x9ba));
    }

    /* 01439: near bottom edge -> draw at y=0xc7 */
    if (a1 > 0xab) {                /* jle 0x144e skips */
        draw_sprite(si, 0xc7, IMG(0x9ba));
    }

    return 0;   /* 01451: ret (no meaningful AX) */
}
