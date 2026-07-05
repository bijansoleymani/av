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
int sub_300(void);              /* mouse-driver detect (INT 33h)               */
int sub_0063f(void);            /* idle/serve animation; returns key hit        */
int joy_calibrate(void);        /* 0x724 read/calibrate game-port joystick      */
int menu_draw(void);            /* draw + run the options menu; returns row      */
int sub_00965(int a0);          /* format/draw a value's low byte at row a0      */
int play_round(void);           /* menu dispatcher (Play/Define/Joystick/Exit)   */
int setup_round(void);          /* draw court/title/net; install keyboard ISR    */
int read_joystick(int player);  /* 0xd50 sample joystick -> control words        */
int read_mouse(int player);     /* 0xde9 sample mouse (INT 33h) -> control words */
int sub_00e7a(void);            /* per-player spike/jump animation state machine */
int sub_01006(void);
int sub_01121(void);            /* ball/player physics step                      */
int sub_013ca(int a0, int a1);
int sub_01452(void);            /* per-frame render (erase+draw players/ball/net)*/
int sub_0166f(void);            /* redraw frame                                  */
int sub_01747(void);            /* scoring / point-won handling                  */
int collide_check(int a0, int a1, int a2); /* 0x199b ball-vs-player collision    */
int sub_019f4(void);            /* AI (computer player) / ball state machine     */
int sub_01cea(void);
int sub_02008(void);            /* the rally: run one point until it ends        */
int sub_0234e(int a0, int a1, int a2);     /* highlight/invert a menu text strip */

/* keyboard ISR replacement + input glue (platform/game) */
void kb_isr(void);            /* original INT 9 handler (unused; SDL feeds state) */

#endif
