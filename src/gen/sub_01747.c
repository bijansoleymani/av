/* 0x01747 sub_01747 - end-of-round: settle the ball with a damped physics loop,
 *                     award/redraw the score for the winning side, then reset
 *                     all round state for the next serve. */
#include "dos.h"
#include "game_protos.h"

/* RTL / library helpers reached by this function but living outside the
 * decompiled game range.  By usage:
 *   sub_04b57(void)              : sound/delay teardown (paired with sub_04b2b)
 *   sub_04b2b(int ms)            : delay(ms) busy-wait
 *   sub_04439(val,buf,radix)->buf: itoa-style number->string, returns buf ptr
 *   sub_03f7a(int ms)            : delay(ms) busy-wait (second timer)
 * They are called by their sub name / mapped shim to stay faithful to the bytes.*/
extern void  sub_04b57(void);
extern void  sub_04b2b(int ms);
extern dsptr sub_04439(int value, dsptr buf, int radix);
extern void  sub_03f7a(int ms);

/* The original keeps a small itoa scratch buffer on the stack at SS:[bp-4].
 * Our model only exposes DS[], and sub_04439/bgi_outtextxy take a dsptr, so we
 * reserve a private scratch at the very top of DS (above DGROUP/heap). */
#define S1747_BUF 0xfff8u

int sub_01747(void)
{
    int winner;         /* winning side (0 = left, 1 = right)            */
    int vx_cap;         /* |ball_vx| >> 3  (x-velocity magnitude cap)    */
    int vy_cap;         /* |ball_vy| >> 3  (y-velocity magnitude cap)    */
    int settle_left;    /* settle-loop countdown (starts at 0x14)        */
    int score_x;        /* winner*0xe6 + 0x28 : score strip x-coordinate */
    int mag;            /* scratch for magnitude/clamp arithmetic        */
    dsptr buf;          /* itoa result pointer                           */

    /* ---- decide the winning side ---- */
    if (touches <= 2) {
        /* few touches: whoever the ball ended up nearest wins */
        winner = (ball_x >= 0x96) ? 0 : 1;
    } else {
        winner = 1 - server;
    }

    sub_01452();

    /* clamp targets: shrink the ball's velocity magnitude by a factor of 8 */
    vy_cap = ball_vy;
    if (vy_cap < 0) vy_cap = -vy_cap;
    vy_cap >>= 3;

    vx_cap = ball_vx;
    if (vx_cap < 0) vx_cap = -vx_cap;
    vx_cap >>= 3;

    settle_left = 0x14;

    /* ---- damped settle loop: run at least 0x14 frames, then keep going
     *      until both players are grounded (player_state == -1).
     *      The disasm's back-edges (jmp 0x1799) re-enter ABOVE these two
     *      writes, so ctrl_jump for both players is re-zeroed at the top of
     *      EVERY iteration (0x1799: xor ax,ax / mov [0x9d2],ax / mov
     *      [0x9cc],ax), clearing any jump the input ISR posted mid-frame. ---- */
    do {
        ctrl_jump(1) = 0;   /* 0179b: W(0x9d2) */
        ctrl_jump(0) = 0;   /* 0179e: W(0x9cc) */

        sub_04b57();
        sub_00e7a();

        /* clamp ball_vx magnitude to vx_cap, preserving sign */
        mag = ball_vx;
        if (mag < 0) mag = -mag;
        if (mag > vx_cap)
            ball_vx = (ball_vx >= 0) ? vx_cap : -vx_cap;

        /* clamp ball_vy magnitude to vy_cap, preserving sign */
        mag = ball_vy;
        if (mag < 0) mag = -mag;
        if (mag > vy_cap)
            ball_vy = (ball_vy >= 0) ? vy_cap : -vy_cap;

        sub_01121();

        if (sound_on != 0)
            sub_04b2b(winner == lead_side ? 0x1388 : 0x64);

        sub_01006();
        sub_01452();
    } while (settle_left-- > 0 ||
             player_state(0) != -1 || player_state(1) != -1);

    /* ================= award point / update display ================= */
    sub_04b57();

    /* score strip x-coordinate for the winning side */
    score_x = (int)(unsigned short)((unsigned)winner * 0xe6u) + 0x28;

    if (winner == lead_side) {
        /* same side still leads: clear the score strip and redraw the
         * incremented score as text */
        bgi_bar(score_x, 0, score_x + 0xf, 7);

        score(winner) = score(winner) + 1;
        buf = sub_04439(score(winner), S1747_BUF, 0xa);
        bgi_outtextxy(score_x, 0, buf);

        /* game over at 15+ points with a 2-point lead */
        if (score(winner) > 0xe &&
            score(winner) - score(1 - winner) > 1)
            game_over = 1;
    } else {
        /* winning side changed: move the small "serving side" dot.
         * bgi_setcolor(0) erases, bgi_setcolor(3) draws white;
         * bgi_fillellipse draws a 1x1 filled dot. */
        bgi_setcolor(0);
        bgi_fillellipse((int)(unsigned short)((unsigned)(1 - winner) * 0xe6u) + 0x23,
                        3, 1, 1);            /* erase old dot */
        bgi_setcolor(3);
        bgi_fillellipse((int)(short)(score_x + 0xfffb), 3, 1, 1); /* new dot */
        lead_side = winner;
    }

    sub_03f7a(0x64);
    sub_0166f();

    /* ================= reset all round state for next serve ================= */
    mag = (int)(unsigned short)((unsigned)winner * 0xa5u) + 0x40;
    ball_prev_x = mag;
    ball_x = mag;
    ball_xf = (int)(unsigned short)((unsigned)mag << 6);

    ball_prev_y = 0x87;
    ball_y = 0x87;
    ball_yf = (int)(unsigned short)((unsigned)0x87 << 6);

    W(0x9e0) = 0;               /* (no game.h name) round/physics state */
    W(0x9de) = 0;               /* (no game.h name) round/physics state */
    touches = 0;
    ball_vy = 0;
    ball_vx = 0;
    ball_high = 0;
    ball_frame = 0;
    W(0x986) = 6;               /* (no game.h name) */

    bounce_shift = 1;
    W(0xa4b) = 1;               /* (no game.h name) */
    server = 2;

    /* serve_state = |hit_count| % 5  (signed remainder) */
    mag = hit_count;
    if (mag < 0) mag = -mag;
    serve_state = mag % 5;

    if (score(lead_side) == 0xe)
        serve_state = 5;
    W(0x9b8) = 0;               /* (no game.h name) */

    return 0;
}
