/* 0x009af play_round - run one match: reset per-player state, then loop the
 *                      menu/round dispatcher until it returns a terminal code. */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* RTL helper at 0x4104 == getch() (blocking key read); result ignored here.
 * Declared extern; provided elsewhere (matches sub_0063f.c). */
extern int sub_04104(void);

int play_round(void)
{
    int r;          /* [bp-2]: return code from menu_draw()                    */
    int si;         /* loop counter                                            */
    int bx;         /* 8-bit/16-bit index scratch                              */
    int ax;

    /* 0x9b6..0x9d9: erase player-1 sprite.
     * bx = p1_frame*8 + (ball_x & 3)*2 ; img = W(bx + 0x994) */
    bx = (p1_frame << 3);
    ax = (ball_x & 3) << 1;
    bx += ax;
    draw_sprite2(ball_x, p1_y, IMG(bx + 0x994));   /* 0x23e2 */

    /* 0x9dc..0x9ff: erase player-2 sprite.
     * bx = p2_frame*8 + (ball_y & 3)*2 ; img = W(bx + 0x9ee) */
    bx = (p2_frame << 3);
    ax = (ball_y & 3) << 1;
    bx += ax;
    draw_sprite2(ball_y, p2_y, IMG(bx + 0x9ee));   /* 0x23e2 */

    /* 0xa02..0xa17: rectangle(3, 0xb, 0x138, 0xc7) (pushed 0xc7,0x138,0xb,3) */
    bgi_rectangle(3, 0xb, 0x138, 0xc7);            /* lib_5fca */

    /* 0xa1a..0xa29: draw_sprite(0x9e, 0x67, IMG(0x9b4)) (pushed img,0x67,0x9e) */
    draw_sprite(0x9e, 0x67, IMG(0x9b4));           /* 0x22d0 */

    /* 0xa2c..0xa7e: reset per-player state for si = 0,1 */
    for (si = 0; si < 2; si++) {                    /* cmp si,2 ; jl */
        W(0x9be + si * 2) = 0;                      /* 0xa36: frame index = 0     */
        W(0x9c8 + si * 6) = 0;                      /* 0xa45 */
        W(0x9ca + si * 6) = 0;                      /* 0xa54 */
        W(0x9cc + si * 6) = si;                     /* 0xa61 */
        W(0x9e8 + si * 2) = si - 1;                 /* 0xa6c: ax=si; dec ax       */
        W(0x9e4 + si * 2) = 0xad;                   /* 0xa74 */
    }

    /* 0xa80: server = 1 */
    server = 1;

L_a86:
    /* 0xa86..0xa8f: r = menu_draw(); if (r == 4) run the score-display block */
    r = menu_draw();                               /* 0x779 */
    if (r == 4)                                     /* je 0xa94 */
        goto L_a94;
    goto L_b82;                                     /* jmp 0xb82 */

L_a94:
    /* 0xa94..0xb5b: r == 4 -> draw six score-digit strings, each captured via
     * sub_00965() into a small results table (W(0x9e..0xa2), W(0xa4..0xa8)). */
    /* di = 0x9e */
    bgi_outtextxy(0x68, 0x28, 0x2ed);              /* 0xa94..0xaa9 */
    W(0x9e + 0) = sub_00965(0x28);                 /* 0xaac..0xab5 */
    bgi_outtextxy(0x68, 0x30, 0x2fa);              /* 0xab7..0xac9 */
    W(0x9e + 2) = sub_00965(0x30);                 /* 0xacc..0xad5 */
    bgi_outtextxy(0x68, 0x38, 0x307);              /* 0xad8..0xaea */
    W(0x9e + 4) = sub_00965(0x38);                 /* 0xaed..0xaf6 */
    /* di = 0xa4 */
    bgi_outtextxy(0x68, 0x40, 0x314);              /* 0xaf9..0xb0e */
    W(0xa4 + 0) = sub_00965(0x40);                 /* 0xb11..0xb1a */
    bgi_outtextxy(0x68, 0x48, 0x321);              /* 0xb1c..0xb2e */
    W(0xa4 + 2) = sub_00965(0x48);                 /* 0xb31..0xb3a */
    bgi_outtextxy(0x68, 0x50, 0x32e);              /* 0xb3d..0xb4f */
    W(0xa4 + 4) = sub_00965(0x50);                 /* 0xb52..0xb5b */

    /* 0xb5e..0xb80: erase 7 font strips: draw_sprite2(0x68, si*8 + 0x28, font_img) */
    for (si = 0; si < 7; si++) {                    /* cmp si,7 ; jl */
        draw_sprite2(0x68, (si << 3) + 0x28, font_img);   /* 0xb62..0xb79 */
    }

L_b82:
    /* 0xb82..0xbc1: r == 5 -> "calibrate joystick" screen */
    if (r != 5)                                     /* jne 0xbc3 */
        goto L_bc3;
    bgi_outtextxy(0x28, 0x28, 0x33b);              /* 0xb88..0xb97 (pushed ds,0x33b,0x28,0x28) */
    sub_04104();                                    /* 0xb9a: getch (wait for key) */
    joy_calibrate();                                /* 0xb9d: 0x724 */
    /* 0xba0..0xbc1: erase 4 font strips: draw_sprite2(si*0x2a + 0x28, 0x28, font_img) */
    for (si = 0; si < 4; si++) {                    /* cmp si,4 ; jl */
        draw_sprite2(si * 0x2a + 0x28, 0x28, font_img);   /* 0xba4..0xbba */
    }

L_bc3:
    /* 0xbc3..0xbcf: if 0 < r < 6 loop back to menu_draw */
    if (r > 0 && r < 6)                             /* jle 0xbd2 ; jge 0xbd2 else */
        goto L_a86;                                 /* 0xbcf: jmp 0xa86 */

    /* 0xbd2..0xbe5: terminal codes */
    if (r == 6) {                                   /* jne 0xbdc */
        ax = 0;                                     /* 0xbd8: xor ax,ax */
        goto L_be8;                                 /* jmp 0xbe8 */
    }
    /* 0xbdc..0xbe5: quit_flag = W(0x15e) ^ 1 ; return 1 */
    quit_flag = W(0x15e) ^ 1;                       /* 0xbdc..0xbe2 */
    ax = 1;                                         /* 0xbe5 */

L_be8:
    return ax;                                       /* 0xbed: ret (AX) */
}
