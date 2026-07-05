/* 0x01121 sub_01121 - ball/player collision + wall/net bounce resolution */
#include "dos.h"
#include "game_protos.h"

int sub_01121(void)
{
    int player;
    int dx_dist;            /* horizontal ball-to-player distance metric */
    int dy_half;            /* (ball_y - player_y) >> 1 */
    int dist_sq;            /* squared-distance collision metric */
    int spin;              /* 8 - (hit_count & 0xf), added to velocities */
    int v;                  /* scratch velocity value */

    /* ---- collision test against each player ---- */
    for (player = 0; player < 2; player++) {
        dx_dist = ball_x - player_x(player) - (short)(player * 7);
        dy_half = (ball_y - player_y(player)) >> 1;

        /* dist_sq = |dx|*dx (via dx>>2) + dy_half^2, computed in 16 bits */
        dist_sq = (short)((unsigned short)((unsigned short)(dx_dist >> 2)
                                            * (unsigned short)dx_dist)
                          + (unsigned short)((unsigned short)dy_half
                                              * (unsigned short)dy_half));

        if (dist_sq >= 0x6e) {
            /* no hit: clear this side's "in contact" flag if it was set */
            if (W(0x9de + player * 2) != 0) {   /* per-side contact flag */
                bounce_shift = 0;
                W(0x9de + player * 2) = 0;
            }
            continue;
        }

        /* ---- ball is touching player: bounce it away ---- */
        spin = 8 - (hit_count & 0xf);

        if (player_state(player) > -1) {
            /* airborne player: add a jump-driven kick to the vertical bounce */
            int kick = W(0x264 + player_state(player) * 2)   /* jump-kick table */
                       << ((3 << (bounce_shift & 0x1f)) & 0x1f);
            ball_vy = -(ball_vy < 0 ? -ball_vy : ball_vy) + kick;
        } else {
            /* grounded player */
            ball_vy = -(ball_vy < 0 ? -ball_vy : ball_vy);
        }
        ball_vy = (short)(ball_vy + spin);

        /* horizontal impulse from the hit offset plus the player's input */
        {
            int abs_dx = (dx_dist < 0 ? -dx_dist : dx_dist);
            int impulse = (short)((unsigned short)((unsigned short)abs_dx
                                                    * (unsigned short)dx_dist));
            int steer = (short)((ctrl_right(player) + ctrl_left(player))
                                << ((bounce_shift + 4) & 0x1f));
            ball_vx = (short)(ball_vx + impulse + steer + spin);
        }

        /* mark contact; on a fresh contact, hand the serve to this side */
        if (W(0x9de + player * 2) == 0) {       /* per-side contact flag */
            W(0xa4d) = 0xc8;                     /* serve-dot / effect state */
            W(0xa4b) = 0;
            W(0x9de + player * 2) = 1;
            if (server == player) {
                touches = (short)(touches + 1);
            } else {
                server = player;
                touches = 0;
            }
        }
    }

    /* ---- net / post collision resolution ---- */
    player = 1;   /* reused as a "hit the net band" flag below */
    if (ball_y > 0x5b) {
        if (ball_prev_x < 0x80 && ball_x > 0x7f) {
            /* crossed into the net from the left: reflect and clamp X */
            ball_vx = -(ball_vx < 0 ? -ball_vx : ball_vx) >> 1;
            ball_xf = 0x1fc0;
            player = 0;
        } else if (ball_prev_x > 0x9f && ball_x < 0xa0) {
            /* crossed into the net from the right */
            ball_vx = (ball_vx < 0 ? -ball_vx : ball_vx) >> 1;
            ball_xf = 0x2800;
            player = 0;
        }
    }

    /* ---- ball inside the net's vertical band: post-top bounce ---- */
    if (player == 0)
        return 0;
    if (ball_y <= 0x51 || ball_x <= 0x7f || ball_x >= 0xa0)
        return 0;

    if (ball_y > 0x5b) {
        /* just under the net rope: reflect vx toward the near side, done */
        if (ball_x < 0x94)
            ball_vx = -(ball_vx < 0 ? -ball_vx : ball_vx);   /* push left */
        else
            ball_vx = (ball_vx < 0 ? -ball_vx : ball_vx);    /* push right */
        return 0;
    }

    /* at the posts: compare the ball's overlap against the post's profile
     * column W(0x250 + (0x5b - ball_y)*2); no overlap -> no bounce */
    if (ball_x > 0x93) {
        /* right post */
        if (W(0x250 + (0x5b - ball_y) * 2) > (0xa1 - ball_x))   /* post profile */
            return 0;
    } else {
        /* left post (ball_x <= 0x93) */
        if (W(0x250 + (0x5b - ball_y) * 2) > (short)(ball_x + 0xff7b))  /* ball_x-133 */
            return 0;
    }

    /* ---- post bounce ---- */
    if (ball_vy > 0) {
        int edge = (short)(ball_x + 0xff6f);   /* ball_x - 145 */
        if (edge < -5)
            ball_vx = -(ball_vx < 0 ? -ball_vx : ball_vx);
        if (edge > 5)
            ball_vx = (ball_vx < 0 ? -ball_vx : ball_vx);
        ball_vy = -(ball_vy < 0 ? -ball_vy : ball_vy);
    }

    /* damp overly fast velocities after a post bounce */
    v = (ball_vx < 0 ? -ball_vx : ball_vx);
    if (v > 0x20)
        ball_vx = ball_vx >> 1;
    v = (ball_vy < 0 ? -ball_vy : ball_vy);
    if (v > 0x20)
        ball_vy = ball_vy >> 1;

    return 0;
}
