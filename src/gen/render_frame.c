/* 0x01452 render_frame - per-frame render: erase/redraw ball spike trail, animate players and ball, draw net/scenery */
#include "dos.h"
#include "game_protos.h"

/* magnitude of a 16-bit difference, matching the binary's neg-on-a-16-bit-reg */
static int abs16(short v) { return v < 0 ? -v : v; }

int render_frame(void)
{
    short px0 = player_x(0);
    short px1 = player_x(1);
    int hit;

    /* The ball counts as a spiking "hit" when it is close to either player
     * (|dx| < 0x29 and |dy| < 0x21), or once it has fallen below y 0xab. */
    if ((abs16(ball_x - px0) < 0x29 && abs16(ball_y - player_y(0)) < 0x21) ||
        (abs16(ball_x - px1) < 0x29 && abs16(ball_y - player_y(1)) < 0x21) ||
        ball_prev_y > 0xab)
        hit = 1;
    else
        hit = 0;

    wait_vsync();

    /* Erase last frame's ball at its previous position: the small "high" ball if
     * it was flagged high, otherwise the big ball (drawn at offset -4,-5) plus a
     * redraw of whatever it overlapped. */
    if (ball_high) {
        draw_sprite2(ball_prev_x, ball_prev_y,
                     smallball_sprite(ball_frame, ball_prev_x & 3));
    } else if (hit) {
        draw_sprite2((short)(ball_prev_x - 4), (short)(ball_prev_y - 5),
                     ball_sprite(ball_frame, (short)(ball_prev_x - 4) & 3));
        draw_edge_sprites(ball_prev_x, ball_prev_y);
    }
    ball_high = hit;

    /* Animation timer at 0x986: decrement every frame; when the pre-decrement
     * value was zero, recompute the period from the ball speed and advance the
     * ball's rotation frame. */
    {
        short timer = W(0x986);
        W(0x986) = timer - 1;
        if (timer == 0) {
            short speed = abs16(ball_vx) + abs16(ball_vy);
            W(0x986) = abs16(0xc - (short)(speed >> 6));
            ball_frame = (ball_frame + 1) & 3;
        }
    }

    /* Draw both players at their current position/frame. */
    draw_sprite(px0, player_y(0), player_sprite(0, player_frame(0), px0 & 3));
    draw_sprite(px1, player_y(1), player_sprite(1, player_frame(1), px1 & 3));

    /* Draw the ball at its current position: the small "high" ball if flagged
     * high, else the big ball at the -4,-5 offset. */
    if (ball_high) {
        draw_sprite_xor(ball_x, ball_y, smallball_sprite(ball_frame, ball_x & 3));
    } else {
        draw_sprite((short)(ball_x - 4), (short)(ball_y - 5),
                    ball_sprite(ball_frame, (short)(ball_x - 4) & 3));
    }
    draw_edge_sprites(ball_x, ball_y);

    /* Left/right court-edge markers when the ball is off the near edge. */
    if (px0 < 6)
        draw_sprite(0, (short)(player_y(0) - 2), post_left_sprite);
    if (px1 > 0x113)
        draw_sprite(0x138, (short)(player_y(1) - 2), post_right_sprite);

    /* Net / center post. */
    draw_sprite(0x9e, 0x67, net_sprite);

    return 0;
}
