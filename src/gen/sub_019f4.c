/* 0x019f4 sub_019f4 - AI/serve target selection: choose paddle move and call collide_check */
#include "dos.h"
#include "game_protos.h"

/* Left-side AI / serve driver.  Either runs a scripted serve sequence (a jump
 * table on serve_state) or tracks the ball and picks a paddle target, then asks
 * collide_check() to move the left player toward that target.  Returns whatever
 * collide_check() last returned (its result is left in AX on every path).
 *
 * Offsets with no game.h name are kept raw with a role comment:
 *   W(0xa4d) - running min of ball_y (peak height reached this volley; Y grows down)
 *   W(0xa4b) - "serve in progress" flag
 *   W(0x9b8) - serve sub-step / one-shot latch used across serve states
 *   ctrl_jump(0) == W(0x9cc) - jump request for the left player
 */
int sub_019f4(void)
{
    int result;                 /* mirrors AX at the ret: collide_check()'s value */
    int slack;                  /* 5 - (hit_count % 10): aim tolerance            */
    int steps;                  /* estimated frames until the ball arrives         */
    int target;                 /* paddle target X passed to collide_check         */
    int player_off;             /* player_x(0) - ball_x                            */
    int vx_abs;                 /* |ball_vx| for range tests                       */

    ctrl_jump(0) = 0;

    /* Track the peak height (smallest ball_y, since Y grows downward). */
    if (ball_y < W(0xa4d))              /* W(0xa4d): running min of ball_y */
        W(0xa4d) = ball_y;

    slack = 5 - (hit_count % 10);

    /* Scripted serve sequence: only while a serve is in progress and this side's
     * score header is not highlighted; otherwise fall through to ball tracking. */
    if (W(0xa4b) != 0                  /* W(0xa4b): serve-in-progress flag */
        && (lead_side & 1) == 0) {

        int jump = 0;                  /* mirror of `si`: value written to ctrl_jump(0) */

        /* Out-of-range serve_state: leave ctrl_jump(0) at 0 and return the raw
         * state value (the binary returns AX = serve_state here). */
        if ((unsigned)serve_state > 5) {
            ctrl_jump(0) = 0;
            return serve_state;
        }

        /* On every path `result` holds collide_check()'s value (the AX returned
         * by the binary); `jump` is the original `si` written to ctrl_jump(0). */
        switch (serve_state) {
        case 0:
            result = collide_check(0, 0x37, 2);
            jump = result;
            break;
        case 1:
            result = collide_check(0, 0x54, 2);
            jump = result;
            break;
        case 2:
            result = collide_check(0, 0x50, 2);
            jump = result;
            break;
        case 3:
            if (W(0x9b8) != 0) {       /* W(0x9b8): serve sub-step latch */
                result = collide_check(0, 0x3a, 2);
                jump = 1;
            } else {
                result = collide_check(0, 0x2c, 2);
                jump = result;
                if (result != 0)
                    W(0x9b8) = 1;
            }
            break;
        case 4:
            if (W(0x9b8) != 0) {
                result = collide_check(0, 0x3a, 2);
                jump = 1;
            } else {
                result = collide_check(0, 0x5a, 2);
                jump = result;
                if (result != 0)
                    W(0x9b8) = 1;
            }
            break;
        case 5:
            if (W(0x9b8) != 0) {
                /* arg = W(0x9b8) + 8, then post-increment W(0x9b8) */
                int arg = W(0x9b8) + 8;
                W(0x9b8) = W(0x9b8) + 1;
                result = collide_check(0, arg, 1);
                jump = 1;
            } else {
                result = collide_check(0, 3, 2);
                if (result != 0)
                    W(0x9b8) = 1;
                jump = 0;
            }
            break;
        }

        ctrl_jump(0) = jump;
        return result;
    }

    /* Not serving (or ineligible): if the ball isn't descending toward this side,
     * just move the paddle to a neutral spot. */
    if (ball_vy <= 0 || ball_x >= 0x8c)
        return collide_check(0, 0x38, 8);

    /* Estimate how many frames until the ball reaches this side. */
    if ((ball_vy >> 6) == 0) {
        steps = 0;
    } else {
        steps = (0x8c - ball_y) / (ball_vy >> 6);
    }

    /* Predicted landing X = ball_x + horizontal drift over `steps` frames. */
    if (steps < 1 || (ball_vx >> 6) == 0) {
        target = ball_x;
    } else {
        /* low 16 bits of (ball_vx >> 6) * steps, then + ball_x - 4 */
        target = (int)(unsigned short)((unsigned)(unsigned short)(ball_vx >> 6)
                                       * (unsigned)(unsigned short)steps);
        target = target + ball_x - 4;
    }

    player_off = player_x(0) - ball_x;

    /* --- slow ball, ball not yet high: nudge to one side --- */
    vx_abs = ball_vx < 0 ? -ball_vx : ball_vx;
    if (vx_abs < 0x80 && W(0xa4d) < 0x4b) {
        /* XOR of (ball_y < 0x9e) and (ball_vx < 0): pick which side to lean. */
        int lean = (ball_y < 0x9e) ^ (ball_vx < 0);
        if (lean)
            /* 0xfff1 = -15; sum wraps to 16 bits, compared signed downstream. */
            return collide_check(0, (short)(ball_x + 0xfff1), 3);
        else
            return collide_check(0, (short)(ball_x + 0xf), 3);
    }

    /* --- ball high enough (else: predicted-landing handling below) --- */
    if (ball_y > 0x82) {
        int off_abs = player_off < 0 ? -player_off : player_off;
        vx_abs = ball_vx < 0 ? -ball_vx : ball_vx;

        /* Ball moving fast and roughly overhead: aim just under it. */
        if (off_abs > 6 && vx_abs < 0x400) {
            /* target = ball_x - ((player_x(0) + (-60)) >> 3) */
            int drift = ((int)(short)(player_x(0) + 0xffc4)) >> 3;
            return collide_check(0, ball_x - drift, 3);
        }

        /* Otherwise aim at the predicted landing with the aim slack. */
        target = (((int)(short)(player_x(0) + 0xffc4)) >> 3) + (ball_x + slack);
        result = collide_check(0, target, 0xa);

        /* Request a jump when the paddle is near the net and it's our chance:
         * player_x(0) < 0x69 and (server != 0 or touches < 2). */
        ctrl_jump(0) = (player_x(0) < 0x69
                        && (server != 0 || touches < 2)) ? 1 : 0;
        return result;
    }

    /* Ball not high: reflect the predicted target into the playable range. */
    if (target < 3)
        target = 6 - target;
    if (target > 0x7b)
        target = 0xf6 - target;
    return collide_check(0, target + slack, 3);
}
