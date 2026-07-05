/* 0x001a5 game_main - top-level: init graphics, load data, then run match rounds */
#include "dos.h"
#include "game_protos.h"

/* RTL-region routine at 0x3c21: gates joystick calibration (bit0 of AX). */
extern int dos_equipment(void);

int game_main(void)
{
	int gd = 1;                 /* graphics driver (consumed by initgraph) */
	int gm = 3;                 /* graphics mode   (consumed by initgraph) */
	int side, col;             /* per-side / per-column loop indices */
	unsigned idx;              /* byte offset into a per-side control block */

	/* registerbgidriver(0x2470): shim always succeeds (returns >= 0). */
	registerbgidriver();       /* far-ptr arg 0x2470 ignored; result treated as 0 */
	if (0 < 0)                 /* original: or ax,ax / jge — never taken here */
		dos_exit(1);

	/* initgraph(&gd, &gm, "") -> cga_init() */
	(void)gd; (void)gm;
	cga_init();

	if (graphresult() < 0) {
		dos_printf(0x2b3);        /* "CGA mode not available." — no closegraph */
		return 0;
	}

	if (load_data() == 0) {
		cga_close();
		return 0;
	}

	sound_on = 0;
	side_swap = 0;

	/* If no mouse driver, seed each side's control block from its defaults.
	 * Per side (1..2): copy 14 bytes [0xd8..] -> [0xca..], then dec [0xac]. */
	if (detect_mouse() == 0) {
		for (side = 1; side < 3; side++) {
			idx = (unsigned short)(side * 0x3c);
			for (col = 0; col < 0xe; col++)
				B(idx + col + 0xca) = B(idx + col + 0xd8);
			W(idx + 0xac)--;
		}
	}

	/* If the joystick path is enabled and calibration succeeds (nonzero),
	 * skip the keyboard-default seeding below. */
	if (!((dos_equipment() & 1) && joy_calibrate() != 0)) {
		/* Per side (1..2): promote current -> active [0xca]->[0xbc], then
		 * reload current from defaults [0xd8]->[0xca]; then dec [0xac]. */
		for (side = 1; side < 3; side++) {
			idx = (unsigned short)(side * 0x3c);
			for (col = 0; col < 0xe; col++) {
				B(idx + col + 0xbc) = B(idx + col + 0xca);
				B(idx + col + 0xca) = B(idx + col + 0xd8);
			}
			W(idx + 0xac)--;
		}
	}

	setup_round();
	player_x(0) = 0x40;        /* [0x98c] */
	player_x(1) = 0xe2;        /* [0x98e] */
	player_y(0) = 0xad;        /* [0x9e4] standing on the floor */
	player_y(1) = 0xad;        /* [0x9e6] */

	/* Run rounds until play_round() reports the match should stop; each round
	 * runs the rally, then swaps ends. */
	while (play_round() != 0) {
		play_match();
		side_swap = side_swap ^ 1;
	}

	cga_close();
	return 0;
}
