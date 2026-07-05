/* 0x00e7a sub_00e7a - per-player: move/clamp player-x toward target, run spike/jump animation state machine */
#include "dos.h"
#include "game_protos.h"

int sub_00e7a(void)
{
    int player;
    int move;          /* control delta: left(-2/0) + right(+2/0)     */
    int target;        /* desired player-x = current x + move          */
    int left_edge;     /* left court boundary for this player          */
    int last_x;        /* 0x990 + p*2: last x when the walk frame flipped */
    int arc;           /* jump-arc table entry                         */

    for (player = 0; player < 2; player++) {
        /* jump pressed while grounded -> begin the jump (state 0) */
        if (ctrl_jump(player) != 0 && player_state(player) == -1)
            player_state(player) = 0;

        /* move toward target x, clamped to the court boundaries */
        move = ctrl_left(player) + ctrl_right(player);
        target = move + player_x(player);
        left_edge = player * 0x9b + 3;
        if (move > 0) {
            /* moving right: clamp to the right edge */
            if (left_edge + 0x77 > target)
                player_x(player) = target;
            else
                player_x(player) = left_edge + 0x77;
        } else {
            /* moving left or standing: clamp to the left edge */
            if (target <= left_edge)
                player_x(player) = left_edge;
            else
                player_x(player) = target;
        }

        /* finished landing: return the player to standing */
        if (player_state(player) == -2) {
            player_y(player) = 0xad;
            player_frame(player) = 0;
            player_state(player) = -1;
        }

        if (player_state(player) == -1) {
            /* grounded */
            if (move == 0) {
                /* standing still: idle frame */
                player_frame(player) = 0;
            } else {
                /* walking: flip stride frame once we've moved > 4px */
                last_x = W(0x990 + player * 2);   /* last x at frame flip */
                if (last_x - player_x(player) < 0)
                    last_x = -(last_x - player_x(player));
                else
                    last_x = last_x - player_x(player);
                if (last_x > 4) {
                    player_frame(player) ^= 1;
                    W(0x990 + player * 2) = player_x(player);
                }
            }
        } else {
            /* airborne (jump/spike): pick the spike frame from the state */
            player_frame(player) = (player_state(player) > 0x12 ? 1 : 0) + 2;

            /* at the apex-transition state, nudge up a bit */
            if (player_state(player) == 0x13)
                player_y(player) -= 4;

            /* advance jump state and apply this step of the jump arc */
            arc = W(0x264 + player_state(player) * 2);   /* jump-arc table */
            player_state(player) += 1;
            player_y(player) += arc;

            /* past the end of the arc: enter the landing state */
            if (player_state(player) > 0x25)
                player_state(player) = -2;
        }
    }

    return 0;
}
