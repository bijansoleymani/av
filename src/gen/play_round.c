/* 0x009af play_round - run one match: reset per-player state, then loop the
 *                      menu/round dispatcher until it returns a terminal code. */
#include "dos.h"
#include "game_protos.h"

/* RTL helper at 0x4104 == getch() (blocking key read); result ignored here.
 * Declared extern; provided elsewhere (matches idle_wait_key.c). */
extern int dos_getch(void);

int play_round(void)
{
    int r;      /* return code from menu_draw() */
    int i;      /* loop counter */

    /* Erase both players' current sprites from the court. */
    draw_sprite2(player_x(0), player_y(0),
                 player_sprite(0, player_frame(0), player_x(0) & 3));
    draw_sprite2(player_x(1), player_y(1),
                 player_sprite(1, player_frame(1), player_x(1) & 3));

    /* Redraw the court border and the net. */
    bgi_rectangle(3, 0xb, 0x138, 0xc7);
    draw_sprite(0x9e, 0x67, net_sprite);

    /* Reset per-player state for the new match (side 0 = left, 1 = right). */
    for (i = 0; i < 2; i++) {
        player_frame(i) = 0;
        ctrl_left(i)    = 0;
        ctrl_right(i)   = 0;
        ctrl_jump(i)    = i;        /* left starts un-jumped, right pre-armed */
        player_state(i) = i - 1;    /* -1 grounded (left), 0 (right)          */
        player_y(i)     = 0xad;     /* stand on the floor                     */
    }
    server = 1;

    for (;;) {
        r = menu_draw();

        if (r == 4) {
            /* Show the six score-digit strings, capturing each drawn strip
             * into the results table at W(0x9e..0xa2) / W(0xa4..0xa8). */
            bgi_outtextxy(0x68, 0x28, 0x2ed);
            W(0x9e + 0) = capture_key(0x28);
            bgi_outtextxy(0x68, 0x30, 0x2fa);
            W(0x9e + 2) = capture_key(0x30);
            bgi_outtextxy(0x68, 0x38, 0x307);
            W(0x9e + 4) = capture_key(0x38);
            bgi_outtextxy(0x68, 0x40, 0x314);
            W(0xa4 + 0) = capture_key(0x40);
            bgi_outtextxy(0x68, 0x48, 0x321);
            W(0xa4 + 2) = capture_key(0x48);
            bgi_outtextxy(0x68, 0x50, 0x32e);
            W(0xa4 + 4) = capture_key(0x50);

            /* Erase the seven font strips just drawn. */
            for (i = 0; i < 7; i++)
                draw_sprite2(0x68, (i << 3) + 0x28, font_img);
        }

        if (r == 5) {
            /* "Calibrate joystick" screen: prompt, wait for a key, calibrate. */
            bgi_outtextxy(0x28, 0x28, 0x33b);
            dos_getch();                /* getch: wait for key */
            joy_calibrate();

            /* Erase the four font strips just drawn. */
            for (i = 0; i < 4; i++)
                draw_sprite2(i * 0x2a + 0x28, 0x28, font_img);
        }

        /* Codes 1..5 are menu selections -> redraw the menu. */
        if (r > 0 && r < 6)
            continue;

        /* Terminal codes: 6 -> quit (return 0); otherwise toggle sound and
         * return 1 to start the match. */
        if (r == 6)
            return 0;
        sound_on = W(0x15e) ^ 1;        /* 0x15e: Sound menu-item flag */
        return 1;
    }
}
