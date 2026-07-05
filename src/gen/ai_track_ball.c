/* 0x01cea ai_track_ball - AI: steer paddle toward predicted ball landing / block spot */
#include "dos.h"
#include "game_protos.h"

/* Unnamed DS globals used only by the AI here (no game.h name yet):
 *   W(0x9d2)  AI decision output (1 = commit to a block/spike, 0 = just track)
 *   W(0xa4d)  running minimum of ball_y seen this volley (peak height)
 *   W(0xa4b)  flag: is this side the AI/computer player?
 *   W(0x9b8)  block/spike state progress counter
 *   W(0x98e)  ball vertical target/anchor (distinct from ball_y at 0x98a) */

/* 16-bit absolute value, matching the DOS `neg` (0x8000 stays 0x8000). */
static int abs16(int v)
{
    return (v < 0) ? (short)-v : v;
}

int ai_track_ball(void)
{
    int serve_offset;   /* 5 - (hit_count % 10)      */
    int frames_to_net;  /* signed idiv result [bp-6] */
    int target_x;       /* predicted ball x    [di]  */
    int ball_dy;        /* W(0x98e) - ball_x   [bp-4]*/
    int decision;       /* AI decision word    [si]  */

    W(0x9d2) = 0;

    /* track the peak (minimum y) the ball reaches this volley */
    if (ball_y < W(0xa4d))
        W(0xa4d) = ball_y;

    serve_offset = 5 - (hit_count % 10);   /* signed idiv remainder */

    /* Block/spike logic only when this side is the AI and it's this side's turn. */
    if (W(0xa4b) != 0 && (lead_side & 1) == 1) {
        decision = 0;
        if ((unsigned)serve_state <= 5u) {
            switch (serve_state) {
            case 0:
                decision = collide_check(1, 0xe8, 2);
                break;
            case 1:
                decision = collide_check(1, 0xca, 2);
                break;
            case 2:
                decision = collide_check(1, 0xd0, 2);
                break;
            case 3:
                if (W(0x9b8) == 0) {
                    decision = collide_check(1, 0xfa, 2);
                    if (decision != 0)
                        W(0x9b8) = 1;      /* advance once we've engaged */
                } else {
                    collide_check(1, 0xdc, 2);
                    decision = 1;
                }
                break;
            case 4:
                if (W(0x9b8) == 0) {
                    decision = collide_check(1, 0xbe, 2);
                    if (decision != 0)
                        W(0x9b8) = 1;
                } else {
                    collide_check(1, 0xe6, 2);
                    decision = 1;
                }
                break;
            case 5:
                if (W(0x9b8) == 0) {
                    if (collide_check(1, 0x115, 2) != 0)
                        W(0x9b8) = 1;
                    decision = 0;
                } else {
                    /* walk back along a fixed arc as the counter climbs */
                    int step = W(0x9b8);
                    W(0x9b8) = step + 1;
                    collide_check(1, 0x110 - step, 1);
                    decision = 1;
                }
                break;
            }
        }
        W(0x9d2) = decision;
        return 0;
    }

    /* --- ball-tracking branch --- */

    /* only predict once the ball is falling and past the net */
    if (ball_vy <= 0 || ball_x <= 0x7d) {
        collide_check(1, 0xd3, 8);   /* idle: hold near mid-court */
        return 0;
    }

    /* frames until the ball crosses y=0x8c, using the coarse (>>6) velocity */
    if ((ball_vy >> 6) == 0)
        frames_to_net = 0;
    else
        frames_to_net = (0x8c - ball_y) / (ball_vy >> 6);

    /* extrapolate the landing x if the ball is actually moving horizontally */
    if (frames_to_net >= 1 && (ball_vx >> 6) != 0) {
        target_x = (int)(unsigned short)((unsigned short)(ball_vx >> 6)
                                         * (unsigned short)frames_to_net);
        target_x += ball_x - 4;
    } else {
        target_x = ball_x;
    }

    ball_dy = W(0x98e) - ball_x;

    /* Slow horizontal ball that hasn't peaked high: play the near reaction. */
    if (abs16(ball_vx) < 0x80 && W(0xa4d) < 0x4b) {
        int low   = (ball_y < 0x9e);        /* ball low on our side  */
        int leftv = (ball_vx < 0);          /* moving left           */
        if (low != leftv)
            collide_check(1, ball_x + 0xf, 3);
        else
            collide_check(1, ball_x - 15, 3);   /* 0xfff1 == -15 */
        return 0;
    }

    if (ball_y > 0x82) {
        /* ball above/behind: aim just under it, or camp at the landing spot */
        if (abs16(ball_dy) > 6 && abs16(ball_vx) < 0x400) {
            collide_check(1, ((W(0x98e) + (short)0xff26) >> 3) + ball_x, 3);
            return 0;
        }
        collide_check(1, ball_x - serve_offset - ((W(0x98e) + (short)0xff26) >> 3), 0xa);
        /* commit to a hit unless the server has already touched twice
         * (0x1f9e reads W(0x98e), the vertical anchor, NOT ball_y at 0x98a) */
        W(0x9d2) = (W(0x98e) > 0xaf && !(server == 1 && touches >= 2)) ? 1 : 0;
        return 0;
    }

    /* fold the predicted x back inside the court bounds, then chase it */
    if (target_x < 0x9e)
        target_x = 0x13c - target_x;
    if (target_x > 0x115)
        target_x = 0x22a - target_x;
    collide_check(1, target_x - serve_offset, 3);
    return 0;
}
