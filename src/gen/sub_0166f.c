/* 0x0166f sub_0166f - redraw frame: erase old ball, draw both players, net box, and center sprite */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_0166f(void)
{
    int bx;
    int ax;

    wait_vsync();                              /* 0166f: call 0x22bf */

    if (W(0xa0e) == 0) goto L16a1;             /* 01672/01677 */

    /* --- erase branch A: shadow/ball table at 0x966 --- */
    bx = UW(0xa3c);                            /* 01679 */
    bx <<= 1; bx <<= 1; bx <<= 1;              /* 0167d/0167f/01681 : bx *= 8 */
    ax = UW(0x9c6) & 3;                        /* 01683/01686 */
    ax <<= 1;                                  /* 01689 */
    bx = (unsigned short)(bx + ax);            /* 0168b */
    draw_sprite2(W(0x9c6), W(0x98a),           /* 01695/01691 args (right-to-left pushed) */
                 UW(0x966 + (unsigned short)bx)); /* 0168d push [bx+0x966] */
    goto L16d0;                                /* 0169f */

L16a1:
    /* --- erase branch B: table at 0xa18 with offset-adjusted coords --- */
    bx = UW(0xa3c);                            /* 016a1 */
    bx <<= 1; bx <<= 1; bx <<= 1;              /* 016a5/016a7/016a9 : bx *= 8 */
    ax = (unsigned short)(UW(0x9c6) + 0xfffc); /* 016ab/016ae : ax -= 4 */
    ax &= 3;                                   /* 016b1 */
    ax <<= 1;                                  /* 016b4 */
    bx = (unsigned short)(bx + ax);            /* 016b6 */
    draw_sprite2((int)(short)(UW(0x9c6) + 0xfffc),   /* 016c3/016c6 : x = [0x9c6] - 4 */
                 (int)(short)(UW(0x98a) + 0xfffb),   /* 016bc/016bf : y = [0x98a] - 5 */
                 UW(0xa18 + (unsigned short)bx));     /* 016b8 push [bx+0xa18] */

L16d0:
    /* --- draw player 1 sprite --- */
    bx = p1_frame;                             /* 016d0 [0x9be] */
    bx <<= 1; bx <<= 1; bx <<= 1;              /* 016d4/016d6/016d8 : bx *= 8 */
    ax = ball_x & 3;                           /* 016da/016dd [0x98c] */
    ax <<= 1;                                  /* 016e0 */
    bx = (unsigned short)(bx + ax);            /* 016e2 */
    draw_sprite(ball_x, p1_y,                  /* 016ec/016e8 args */
                UW(0x994 + (unsigned short)bx)); /* 016e4 push [bx+0x994] */

    /* --- draw player 2 sprite --- */
    bx = p2_frame;                             /* 016f6 [0x9c0] */
    bx <<= 1; bx <<= 1; bx <<= 1;              /* 016fa/016fc/016fe : bx *= 8 */
    ax = ball_y & 3;                           /* 01700/01703 [0x98e] */
    ax <<= 1;                                  /* 01706 */
    bx = (unsigned short)(bx + ax);            /* 01708 */
    draw_sprite(ball_y, p2_y,                  /* 01712/0170e args */
                UW(0x9ee + (unsigned short)bx)); /* 0170a push [bx+0x9ee] */

    /* --- draw the net/court box --- */
    bgi_rectangle(3, 0xb, 0x138, 0xc7);        /* 0171c..0172c (pushed 0xc7,0x138,0xb,3) */

    /* --- draw center sprite IMG(0x9b4) at (0x9e, 0x67) --- */
    draw_sprite(0x9e, 0x67, IMG(0x9b4));       /* 01734..01740 */

    return ax;                                 /* 01746: ret (AX undefined here) */
}
