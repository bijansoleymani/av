/* 0x01006 sub_01006 - advance ball one step: clamp velocity, integrate position, bounce off walls/net, return whether ball hit the floor region */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_01006(void)
{
    int di;              /* si-saved / velocity component from [0x988] */
    int si;              /* di-saved / velocity component from [0x9b6] */
    int ax;
    int loc2;            /* [bp-2] : local result word */

    di = W(0x988);       /* 0100d */
    si = W(0x9b6);       /* 01011 */

    /* clamp di into [-0x13f, 0x13f] (0xfec1 == -0x13f) */
    if (di > 0x13f)                 /* 01015 cmp/jle */
        di = 0x13f;                 /* 0101b */
    if (di < (short)0xfec1)         /* 0101e cmp/jge (signed) */
        di = (short)0xfec1;         /* 01024 */

    /* clamp si into [-0x13f, 0x13f] */
    if (si > 0x13f)                 /* 01027 cmp/jle */
        si = 0x13f;                 /* 0102d */
    if (si < (short)0xfec1)         /* 01030 cmp/jge (signed) */
        si = (short)0xfec1;         /* 01036 */

    W(0xa3a) = W(0x9c6);            /* 01039..0103c */
    W(0x9c4) = W(0x98a);            /* 0103f..01042 */

    W(0xa40) += di;                 /* 01045 add [0xa40], di */
    W(0x9ec) += si;                 /* 01049 add [0x9ec], si */

    /* left wall: if x-subpos < 0x140 clamp and reflect (0.9375 damping via -x>>4) */
    if (W(0xa40) < 0x140) {         /* 0104d cmp/jge -> skip */
        W(0xa40) = 0x140;           /* 01055 */
        ax = -di;                   /* 0105b..0105d neg */
        di = ax;                    /* 0105f */
        ax = di >> 4;               /* 01061..01066 sar ax,4 */
        di -= ax;                   /* 01068 */
        ax = si >> 4;               /* 0106a..0106c sar ax,4 (cl still 4) */
        si -= ax;                   /* 0106e */
        if (W(0x9d6) == 1) {        /* 01070 cmp [server],1 / jne */
            W(0x9d6) = 2;           /* 01077 server = 2 */
            W(0x9dc) = 0;           /* 0107d */
        }
    }
    /* L1083 */

    /* right wall: if x-subpos > 0x46c0 clamp and reflect */
    if (W(0xa40) > 0x46c0) {        /* 01083 cmp/jle -> skip */
        W(0xa40) = 0x46c0;          /* 0108b */
        ax = -di;                   /* 01091..01093 neg */
        di = ax;                    /* 01095 */
        ax = di >> 4;               /* 01097..0109c sar ax,4 */
        di -= ax;                   /* 0109e */
        ax = si >> 4;               /* 010a0..010a2 sar ax,4 */
        si -= ax;                   /* 010a4 */
        if (W(0x9d6) == 0) {        /* 010a6 cmp [server],0 / jne */
            W(0x9d6) = 2;           /* 010ad server = 2 */
            W(0x9dc) = 0;           /* 010b3 */
        }
    }
    /* L10b9 */

    /* top: if y-subpos < 0x340 clamp and reflect */
    if (W(0x9ec) < 0x340) {         /* 010b9 cmp/jge -> skip */
        W(0x9ec) = 0x340;           /* 010c1 */
        ax = -si;                   /* 010c7..010c9 neg */
        si = ax;                    /* 010cb */
        ax = di >> 4;               /* 010cd..010d2 sar ax,4 */
        di -= ax;                   /* 010d4 */
        ax = si >> 4;               /* 010d6..010d8 sar ax,4 */
        si -= ax;                   /* 010da */
    }
    /* L10dc */

    /* bottom: if y-subpos > 0x2c80 clamp, reflect, result=0; else result=1 */
    if (W(0x9ec) > 0x2c80) {        /* 010dc cmp/jle -> else */
        W(0x9ec) = 0x2c80;          /* 010e4 */
        ax = -si;                   /* 010ea..010ec neg */
        si = ax;                    /* 010ee */
        loc2 = 0;                   /* 010f0 mov [bp-2],0 */
    } else {
        loc2 = 1;                   /* 010f7 mov [bp-2],1 */
    }
    /* L10fc */

    si++;                           /* 010fc inc si */

    ax = W(0xa40) >> 6;             /* 010fd..01103 sar ax,6 */
    W(0x9c6) = ax;                  /* 01105 */
    ax = W(0x9ec) >> 6;             /* 01108..0110b sar ax,6 */
    W(0x98a) = ax;                  /* 0110d */

    W(0x988) = di;                  /* 01110 */
    W(0x9b6) = si;                  /* 01114 */

    return loc2;                    /* 01118 mov ax,[bp-2]; ret */
}
