/* 0x01452 sub_01452 - per-frame render: erase/redraw ball spike trail, animate players and ball, draw net/scenery */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_01452(void)
{
    int si, di;          /* si=ball_x, di=ball_y (register vars) */
    int ax, dx;
    unsigned bx;
    int hit;             /* [bp-2] local */

    si = W(0x98c);       /* 01459: ball_x */
    di = W(0x98e);       /* 0145d: ball_y */

    /* ---- decide hit flag ---- */
    ax = W(0x9c6) - si;                  /* 01461/01464 */
    if (ax < 0) ax = -ax;                /* 01466..0146a */
    if (ax >= 0x29) goto L1483;          /* 0146c/0146f */
    ax = W(0x98a) - W(0x9e4);            /* 01471/01474: - p1_y */
    if (ax < 0) ax = -ax;                /* 01478..0147c */
    if (ax < 0x21) goto L14ad;           /* 0147e/01481 */
L1483:
    ax = W(0x9c6) - di;                  /* 01483/01486 */
    if (ax < 0) ax = -ax;                /* 01488..0148c */
    if (ax >= 0x29) goto L14a5;          /* 0148e/01491 */
    ax = W(0x98a) - W(0x9e6);            /* 01493/01496: - p2_y */
    if (ax < 0) ax = -ax;                /* 0149a..0149e */
    if (ax < 0x21) goto L14ad;           /* 014a0/014a3 */
L14a5:
    if (W(0x9c4) <= 0xab) goto L14b2;    /* 014a5/014ab */
L14ad:
    ax = 1;                              /* 014ad */
    goto L14b4;                          /* 014b0 */
L14b2:
    ax = 0;                              /* 014b2: xor ax,ax */
L14b4:
    hit = ax;                            /* 014b4: [bp-2] = ax */

    wait_vsync();                        /* 014b7: call 0x22bf */

    /* ---- erase previous spike sprite ---- */
    if (W(0xa0e) == 0) goto L14e9;       /* 014ba/014bf */
    bx = (unsigned)W(0xa3c) << 3;        /* 014c1..014c9: *8 */
    ax = W(0xa3a) & 3;                   /* 014cb/014ce */
    ax <<= 1;                            /* 014d1 */
    bx += ax;                            /* 014d3 */
    draw_sprite2(W(0xa3a), W(0x9c4), UW(bx + 0x966)); /* 014d5..014e1 */
    goto L152c;                          /* 014e7 */
L14e9:
    if (hit == 0) goto L152c;            /* 014e9/014ed */
    bx = (unsigned)W(0xa3c) << 3;        /* 014ef..014f7: *8 */
    ax = (short)(W(0xa3a) - 4) & 3;      /* 014f9..014ff: add 0xfffc = -4, then &3 */
    ax <<= 1;                            /* 01502 */
    bx += ax;                            /* 01504 */
    draw_sprite2((short)(W(0xa3a) - 4), (short)(W(0x9c4) - 5), UW(bx + 0xa18)); /* 01506..01518 (0xfffc=-4, 0xfffb=-5) */
    sub_013ca(W(0xa3a), W(0x9c4));       /* 0151e..01526 */
L152c:
    W(0xa0e) = hit;                      /* 0152c/0152f */

    /* ---- countdown timer / animation phase advance ---- */
    ax = W(0x986);                       /* 01532 */
    W(0x986) = W(0x986) - 1;             /* 01535: dec [0x986] */
    if (ax != 0) goto L1573;             /* 01539/0153b (test old value) */
    ax = W(0x988);                       /* 0153d */
    if (ax < 0) ax = -ax;                /* 01540..01544 */
    dx = W(0x9b6);                       /* 01546 */
    if (dx < 0) dx = -dx;                /* 0154a..0154e */
    ax = ax + dx;                        /* 01550 */
    ax = ax >> 6;                        /* 01552..01555: sar ax,6 (signed) */
    dx = 0xc - ax;                       /* 01557/0155a */
    if (dx < 0) dx = -dx;                /* 0155c..01560 */
    W(0x986) = dx;                       /* 01562 */
    W(0xa3c) = W(0xa3c) + 1;             /* 01566: inc [0xa3c] */
    W(0xa3c) = W(0xa3c) & 3;             /* 0156a..01570 */
L1573:
    /* ---- draw player 1 ---- */
    bx = (unsigned)W(0x9be) << 3;        /* 01573..0157b: p1_frame*8 */
    ax = si & 3;                         /* 0157d/0157f */
    ax <<= 1;                            /* 01582 */
    bx += ax;                            /* 01584 */
    draw_sprite(si, W(0x9e4), UW(bx + 0x994)); /* 01586..0158f: p1_y */

    /* ---- draw player 2 ---- */
    bx = (unsigned)W(0x9c0) << 3;        /* 01595..0159d: p2_frame*8 */
    ax = di & 3;                         /* 0159f/015a1 */
    ax <<= 1;                            /* 015a4 */
    bx += ax;                            /* 015a6 */
    draw_sprite(di, W(0x9e6), UW(bx + 0x9ee)); /* 015a8..015b1: p2_y */

    /* ---- draw current spike sprite ---- */
    if (W(0xa0e) == 0) goto L15e6;       /* 015b7/015bc */
    bx = (unsigned)W(0xa3c) << 3;        /* 015be..015c6: *8 */
    ax = W(0x9c6) & 3;                   /* 015c8/015cb */
    ax <<= 1;                            /* 015ce */
    bx += ax;                            /* 015d0 */
    sub_0234e(W(0x9c6), W(0x98a), UW(bx + 0x966)); /* 015d2..015de */
    goto L1615;                          /* 015e4 */
L15e6:
    bx = (unsigned)W(0xa3c) << 3;        /* 015e6..015ee: *8 */
    ax = (short)(W(0x9c6) - 4) & 3;      /* 015f0..015f6: add 0xfffc = -4, then &3 */
    ax <<= 1;                            /* 015f9 */
    bx += ax;                            /* 015fb */
    draw_sprite((short)(W(0x9c6) - 4), (short)(W(0x98a) - 5), UW(bx + 0xa18)); /* 015fd..0160f (0xfffc=-4, 0xfffb=-5) */
L1615:
    sub_013ca(W(0x9c6), W(0x98a));       /* 01615..0161d */

    /* ---- draw left player floor shadow / marker when ball near ---- */
    if (si >= 6) goto L163c;             /* 01623/01626 */
    draw_sprite(0, (short)(W(0x9e4) - 2), IMG(0x9da)); /* 01628..01636: p1_y + 0xfffe = p1_y-2 */
L163c:
    if (di <= 0x113) goto L1657;         /* 0163c/01640 */
    draw_sprite(0x138, (short)(W(0x9e6) - 2), IMG(0xa38)); /* 01642..01651: p2_y + 0xfffe = p2_y-2 */
L1657:
    /* ---- draw net / center post ---- */
    draw_sprite(0x9e, 0x67, IMG(0x9b4)); /* 01657..01663 */

    /* 0166e: ret with AX = leftover from draw_sprite (void in our model);
       no meaningful return value -> 0 per contract. */
    return 0;
}
