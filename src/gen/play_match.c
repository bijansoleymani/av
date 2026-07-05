/* 0x02008 play_match - play one match: reset per-side state, install the
 *                     keyboard ISR, then run the input/physics/render loop until
 *                     the round terminates, restoring the vector on the way out. */
#include "dos.h"
#include "game_protos.h"

/* Interrupt-vector helpers in the Turbo C RTL region (not part of the dumped
 * game code):
 *   getvect == getvect(int)      -> old handler far ptr in DX:AX
 *   setvect == setvect(int,ptr)  -> install handler
 * The getvect(9)/setvect(9, our_isr) pair at 0x21b9/0x21d0 installs the game's
 * INT 9 keyboard ISR (located at cs:0xc5d); the setvect(9, old) at 0x22a7
 * restores it.  Per the porting contract these are replaced with kb_install()
 * (SDL feeds key state) and the restore becomes a no-op. */
extern void kb_install(void);

/* RTL polling/blocking key helpers (same as elsewhere in the game). */
extern int dos_kbhit(void);   /* kbhit-style poll: AX nonzero when key waiting */
extern int dos_getch(void);   /* getch-style blocking read (result discarded)  */

/* Per-player control kind decoded from the options menu (menu key char):
 * 0 = none, 1 = joystick, 2 = mouse, 3 = keyboard. */
static int decode_control(int menu_char)
{
    switch (menu_char) {
    case 0x4a: return 1;   /* 'J' joystick */
    case 0x20: return 2;   /* ' ' mouse    */
    case 0x43: return 3;   /* 'C' keyboard */
    default:   return 0;
    }
}

int play_match(void)
{
    int side;
    int frame_parity;   /* alternates 0/1 each frame (SI): gates mouse reads */
    int alive;          /* rally still running (DI): render-step result      */
    int ctl1, ctl2;     /* player-1/2 control kind (see decode_control)      */

    /* Reset ball state and a couple of per-match constants. */
    ball_vy = 0;
    ball_vx = 0;
    ball_high = 0;
    ball_frame = 0;
    touches = 0;
    W(0x986) = 6;        /* (unnamed) */
    W(0xa4d) = 0xc8;     /* (unnamed) */

    /* Per-side reset: home position, standing state, cleared inputs/scores.
     * Also pick which side started the previous game at match point so ends
     * can be swapped. */
    for (side = 0; side < 2; side++) {
        W(0x990 + side * 2) = player_x(side);  /* (unnamed) saved spawn X */
        player_y(side) = 0xad;                 /* on the floor */
        player_state(side) = 0xffff;           /* -1: grounded */
        player_frame(side) = 0;
        ctrl_jump(side) = 0;
        ctrl_left(side) = 0;
        ctrl_right(side) = 0;
        score(side) = 0;
        W(0x9de + side * 2) = 0;               /* (unnamed) per-side counter */

        /* If this side is at (max-1) points and the other side is exactly at
         * (max-1), record it as the side to swap ends.  Tables at (n+1)*0x3c:
         *   +0xaa = current score, +0xac = target score. */
        if (W((side + 1) * 0x3c + 0xaa) >= W((side + 1) * 0x3c + 0xac) - 1)
            continue;
        if (W((2 - side) * 0x3c + 0xaa) != W((2 - side) * 0x3c + 0xac) - 1)
            continue;
        side_swap = side;
    }

    /* joystick sample window = joy_val * 1.5  (arithmetic shift, signed). */
    joy_timeout = joy_val + (joy_val >> 1);

    /* Side-dependent court layout: serve position depends on which end. */
    ball_prev_x = side_swap * 0xa5 + 0x40;
    ball_x = ball_prev_x;
    ball_xf = (unsigned short)((unsigned)ball_x << 6);

    ball_prev_y = 0x87;
    ball_y = 0x87;
    ball_yf = (unsigned short)((unsigned)ball_y << 6);

    lead_side = side_swap + 2;

    server = 2;
    bounce_shift = 1;
    W(0xa4b) = 1;        /* (unnamed) "serve pending" flag: force one physics tick */
    game_over = 0;

    /* Decode each player's control device from the options menu structures. */
    ctl1 = decode_control(B(W(0xe6) * 0xe + 0xee) & 0xff);
    ctl2 = decode_control(B(W(0x122) * 0xe + 0x12a) & 0xff);

    /* getvect(9)/setvect(9, our_isr) -> install the keyboard ISR (SDL feeds
     * key state, so this becomes kb_install()). */
    kb_install();

    setup_round();

    frame_parity = 0;

    while (game_over == 0) {
        hit_count = hit_count * 5 + 1;

        /* Player-1 input, by control kind. */
        switch (ctl1) {
        case 1:
            read_joystick(0);
            break;
        case 2:
            if (frame_parity != 0)   /* mouse only read on odd frames */
                read_mouse(0);
            break;
        case 3:
            ai_choose_move();
            break;
        }

        /* Player-2 input, by control kind. */
        switch (ctl2) {
        case 1:
            read_joystick(1);
            break;
        case 2:
            if (frame_parity != 0)
                read_mouse(1);
            break;
        case 3:
            ai_track_ball();
            break;
        }

        frame_parity = 1 - frame_parity;

        update_players();

        /* Physics + render.  While a serve is pending (W(0xa4b)) run the step
         * unconditionally; otherwise only while the rally is still alive, and
         * let advance_ball() report whether it continues. */
        if (W(0xa4b) != 0) {
            collide_players();
            render_frame();
            alive = 1;
        } else if (alive != 0) {
            collide_players();
            alive = advance_ball();
            render_frame();
        }

        /* Loop back unless the rally ended or the volley touch count spilled
         * past 2, in which case run the point-won handler first. */
        if (alive == 0 || touches > 2)
            end_round();
    }

    /* Post-round cleanup: redraw, restore INT 9 (no-op under SDL), then drain
     * the keyboard buffer. */
    redraw_frame();
    /* setvect(9, old_vector) restore -> no-op */
    while (dos_kbhit() != 0)
        dos_getch();   /* discard */

    return 0;
}
