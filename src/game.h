/* game.h — readable names for the game's state, all backed by the DS[] model.
 *
 * These are thin macro/accessor overlays on the exact same memory the original
 * used (see dos.h): renaming/structuring them changes how the code READS, never
 * what it does.  Player/side indices are 0 = left, 1 = right.  Names marked
 * "(inferred)" are best-guess semantics; the DS offset is kept so they stay
 * traceable to the disassembly.
 */
#ifndef GAME_H
#define GAME_H
#include "dos.h"

#define IMG(o) UW(o)                 /* a sprite-pointer global (dsptr) */

/* ============================ players ============================ */
#define player_x(p)      W(0x98c + (p) * 2)   /* on-court X (low 2 bits = sub-pixel) */
#define player_y(p)      W(0x9e4 + (p) * 2)   /* Y; 0xad (173) = standing on the floor */
#define player_frame(p)  W(0x9be + (p) * 2)   /* animation frame index               */
#define player_state(p)  W(0x9e8 + (p) * 2)   /* jump/serve state: -1 grounded, -2 landing */

/* per-player control input, written by the keyboard ISR / joystick / mouse:
 *   left = -2 or 0,  right = +2 or 0,  jump = 1 or 0   (stride 6 bytes) */
#define ctrl_left(p)     W(0x9c8 + (p) * 6)
#define ctrl_right(p)    W(0x9ca + (p) * 6)
#define ctrl_jump(p)     W(0x9cc + (p) * 6)

#define score(s)         W(0xa10 + (s) * 2)   /* match score per side */

/* ============================= ball ============================= */
#define ball_x           W(0x9c6)             /* integer screen position */
#define ball_y           W(0x98a)
#define ball_vx          W(0x988)             /* velocity                */
#define ball_vy          W(0x9b6)
#define ball_xf          W(0xa40)             /* sub-pixel accumulator (pos << 6) */
#define ball_yf          W(0x9ec)
#define ball_prev_x      W(0xa3a)             /* previous position (for erase/reset) */
#define ball_prev_y      W(0x9c4)
#define ball_frame       W(0xa3c)             /* rotation animation index */
#define ball_high        W(0xa0e)             /* (inferred) nonzero => draw the small "far" ball */

/* ========================== match state ========================= */
#define server           W(0x9d6)             /* side currently serving (0/1) */
#define lead_side        W(0xa14)             /* side whose score header is highlighted */
#define hit_count        W(0x9e2)             /* running touch counter */
#define game_over        W(0x9d4)             /* set to end the match (Esc / 15 pts) */
#define sound_on         W(0x24e)             /* sound-enabled flag (= Sound menu ^ 1) */
#define side_swap        W(0x9d8)             /* teams swap ends each game */

/* physics / serve state whose meaning is only partially recovered — named by
 * best guess, offset preserved for reference. */
#define touches          W(0x9dc)             /* (inferred) touches this volley */
#define serve_state      W(0xa3e)             /* (inferred) 0..5 serve/AI state index */
#define bounce_shift     W(0x9bc)             /* (inferred) ball-speed shift for bounces */

/* ===================== sprite tables (dsptr) ====================
 * Each animated sprite is 4 pre-shifted copies (shift = x & 3); groups are
 * 8 bytes apart (frame index).  See load_data()/preshift_sprite(). */
#define player_sprite(p, frame, shift)  IMG(((p) ? 0x9ee : 0x994) + (frame) * 8 + (shift) * 2)
#define ball_sprite(frame, shift)       IMG(0xa18 + (frame) * 8 + (shift) * 2)
#define smallball_sprite(frame, shift)  IMG(0x966 + (frame) * 8 + (shift) * 2)
#define net_sprite                      IMG(0x9b4)
#define post_left_sprite                IMG(0x9da)
#define post_right_sprite               IMG(0xa38)

/* ============================ misc RTL ========================== */
#define dat_fd           W(0xa50)             /* AV.DAT file handle           */
#define font_img         W(0x9c2)             /* captured "clear strip" image */
#define joy_timeout      W(0xa42)
#define joy_val          W(0xa44)

#endif /* GAME_H */
