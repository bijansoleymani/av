/* 0x01006 sub_01006 - advance ball one step: clamp velocity, integrate position, bounce off walls/net, return whether ball hit the floor region */
#include "dos.h"
#include "game_protos.h"

int sub_01006(void)
{
    int vx = ball_vx;
    int vy = ball_vy;
    int on_court;               /* result: 1 while ball still in play, 0 when it hit the floor */

    /* clamp both velocity components into [-0x13f, 0x13f] */
    if (vx > 0x13f)
        vx = 0x13f;
    if (vx < -0x13f)
        vx = -0x13f;
    if (vy > 0x13f)
        vy = 0x13f;
    if (vy < -0x13f)
        vy = -0x13f;

    /* remember where the ball was, then integrate the sub-pixel position */
    ball_prev_x = ball_x;
    ball_prev_y = ball_y;
    ball_xf += vx;
    ball_yf += vy;

    /* left wall: clamp and reflect X, shedding 1/16 of each component (0.9375 damping) */
    if (ball_xf < 0x140) {
        ball_xf = 0x140;
        vx = -vx;
        vx -= vx >> 4;
        vy -= vy >> 4;
        if (server == 1) {      /* ball crossed to the left while right was serving: fault */
            server = 2;
            touches = 0;
        }
    }

    /* right wall: clamp and reflect X, same damping */
    if (ball_xf > 0x46c0) {
        ball_xf = 0x46c0;
        vx = -vx;
        vx -= vx >> 4;
        vy -= vy >> 4;
        if (server == 0) {      /* ball crossed to the right while left was serving: fault */
            server = 2;
            touches = 0;
        }
    }

    /* ceiling: clamp and reflect Y, same damping */
    if (ball_yf < 0x340) {
        ball_yf = 0x340;
        vy = -vy;
        vx -= vx >> 4;
        vy -= vy >> 4;
    }

    /* floor: clamp and reflect Y; ball has landed (result 0), otherwise still in play (result 1) */
    if (ball_yf > 0x2c80) {
        ball_yf = 0x2c80;
        vy = -vy;
        on_court = 0;
    } else {
        on_court = 1;
    }

    vy++;                       /* gravity: pull velocity downward one step */

    /* project the sub-pixel accumulators back to integer screen coordinates */
    ball_x = ball_xf >> 6;
    ball_y = ball_yf >> 6;

    ball_vx = vx;
    ball_vy = vy;

    return on_court;
}
