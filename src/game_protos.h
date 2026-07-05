/* game_protos.h — prototypes for the decompiled game functions.
 *
 * Functions still being decompiled are declared with unspecified args
 * (`int name();`) so any call site links; each definition uses its real
 * parameters.  The already-verified pipeline/primitive functions keep exact
 * signatures. */
#ifndef GAME_PROTOS_H
#define GAME_PROTOS_H
#include "dos.h"
#include "game.h"      /* named game-state accessors (player_x, ball_x, …) */

/* verified data / render pipeline (game.c) */
dsptr read_chunk(int n);
void  preshift_sprite(dsptr group);
int   load_data(void);
void  dos_data_init(void);

/* decompiled game logic (return AX; void-like ones return 0) */
int game_main(void);
int detect_mouse(void);              /* mouse-driver detect (INT 33h)               */
int idle_wait_key(void);            /* idle/serve animation; returns key hit        */
int joy_calibrate(void);        /* 0x724 read/calibrate game-port joystick      */
int menu_draw(void);            /* draw + run the options menu; returns row      */
int capture_key(int a0);          /* format/draw a value's low byte at row a0      */
int play_round(void);           /* menu dispatcher (Play/Define/Joystick/Exit)   */
int setup_round(void);          /* draw court/title/net; install keyboard ISR    */
int read_joystick(int player);  /* 0xd50 sample joystick -> control words        */
int read_mouse(int player);     /* 0xde9 sample mouse (INT 33h) -> control words */
int update_players(void);            /* per-player spike/jump animation state machine */
int advance_ball(void);
int collide_players(void);            /* ball/player physics step                      */
int draw_edge_sprites(int a0, int a1);
int render_frame(void);            /* per-frame render (erase+draw players/ball/net)*/
int redraw_frame(void);            /* redraw frame                                  */
int end_round(void);            /* scoring / point-won handling                  */
int collide_check(int a0, int a1, int a2); /* 0x199b ball-vs-player collision    */
int ai_choose_move(void);            /* AI (computer player) / ball state machine     */
int ai_track_ball(void);
int play_match(void);            /* the rally: run one point until it ends        */
int draw_sprite_xor(int a0, int a1, int a2);     /* highlight/invert a menu text strip */

/* keyboard ISR replacement + input glue (platform/game) */
void kb_isr(void);            /* original INT 9 handler (unused; SDL feeds state) */

#endif
