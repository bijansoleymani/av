/* 0x0063f sub_0063f - idle/serve animation loop; run frames until a key is hit, return that key char */
#include "dos.h"
#include "game_protos.h"

/* Turbo C console RTL pair used by this loop (not game/BGI functions):
 *   sub_0432b == kbhit()  -> AX nonzero when a key is waiting
 *   sub_04104 == getch()  -> AL = the key character                    */
extern int sub_0432b(void);
extern int sub_04104(void);

int sub_0063f(void)
{
    /* Run the demo/serve animation one frame at a time until a key is pressed. */
    while ((sub_0432b() & 0xffff) == 0) {
        hit_count = hit_count + 1;

        /* If the serving side is grounded (state -1), flip the serve: the old
         * server stops pressing jump, the new server starts. */
        if (player_state(server) == -1) {
            ctrl_jump(server) = 0;
            server = (short)(1 - server);
            ctrl_jump(server) = 1;
        }

        sub_00e7a();                 /* advance ball physics */

        /* snapshot each player's X (0x98c/0x98e); these serve as the sprite X
         * coords, the sub-pixel shift (& 3), and the near-edge post tests below.
         * (The original decompiler mislabeled these as ball_x/ball_y.) */
        int px0 = player_x(0);
        int px1 = player_x(1);

        wait_vsync();

        draw_sprite(px0, player_y(0),
                    player_sprite(0, player_frame(0), px0 & 3));
        draw_sprite(px1, player_y(1),
                    player_sprite(1, player_frame(1), px1 & 3));

        /* left post top when player 0 near the left edge */
        if (px0 < 6)
            draw_sprite(0, (short)(player_y(0) - 2), post_left_sprite);

        /* right post top when player 1 near the right edge */
        if (px1 > 0x113)
            draw_sprite(0x138, (short)(player_y(1) - 2), post_right_sprite);

        draw_sprite(0x9e, 0x67, net_sprite);
    }

    return sub_04104();              /* getch -> key char */
}
