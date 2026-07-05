/* 0x0166f sub_0166f - redraw frame: erase old ball, draw both players, net box, and center sprite */
#include "dos.h"
#include "game_protos.h"

int sub_0166f(void)
{
    wait_vsync();

    if (ball_high) {
        /* far/high ball: erase the small "far" ball at its current position */
        draw_sprite2(ball_x, ball_y,
                     smallball_sprite(ball_frame, ball_x & 3));
    } else {
        /* near ball: erase the full ball, offset by (-4, -5) to its top-left */
        draw_sprite2(ball_x - 4, ball_y - 5,
                     ball_sprite(ball_frame, (ball_x - 4) & 3));
    }

    /* draw both players */
    draw_sprite(player_x(0), player_y(0),
                player_sprite(0, player_frame(0), player_x(0) & 3));
    draw_sprite(player_x(1), player_y(1),
                player_sprite(1, player_frame(1), player_x(1) & 3));

    /* court box and the net sprite */
    bgi_rectangle(3, 0xb, 0x138, 0xc7);
    draw_sprite(0x9e, 0x67, net_sprite);

    return 0;   /* original leaves AX undefined; callers ignore it */
}
