/* dos.h — small-model DOS runtime model for the Arcade Volleyball reconstruction.
 *
 * The original AV.EXE is a Turbo C 1987 small-model program: one 64 KB data
 * segment (DGROUP) holding string constants, globals (BSS) and the malloc heap,
 * plus the CGA video segment at 0xB800.  We reproduce that faithfully:
 *
 *   DS[]      the 64 KB data segment.  A "near pointer" in the original is just
 *             a 16-bit offset into this array, so we keep pointers AS offsets
 *             (type dsptr) and dereference with the B()/W() accessors below.
 *             DS[0..0x966) is initialised verbatim from the EXE (DS_INIT);
 *             the rest is BSS (zero) and heap.
 *   VIDEO[]   the 16 KB CGA framebuffer (segment 0xB800), 320x200 4-colour,
 *             two interleaved 8 KB banks (even scanlines / odd scanlines).
 *
 * Turbo C `int` is 16-bit and signed; `char` is signed.  We mirror that with
 * short/signed char so overflow and sign behaviour match the binary.
 */
#ifndef DOS_H
#define DOS_H
#include <stdint.h>
#include <stddef.h>

typedef uint16_t dsptr;              /* a near pointer == offset into DS[] */

extern unsigned char DS[0x10000];    /* data segment (DGROUP + heap)       */
extern unsigned char VIDEO[0x4000];  /* CGA 0xB800 segment, 16 KB          */

/* ---- data-segment accessors (o is a DS offset) ---- */
#define B(o)   (DS[(uint16_t)(o)])                          /* unsigned char lvalue */
#define SB(o)  (*(signed char *)(DS + (uint16_t)(o)))       /* signed char lvalue   */
#define W(o)   (*(short *)(DS + (uint16_t)(o)))             /* 16-bit signed int    */
#define UW(o)  (*(unsigned short *)(DS + (uint16_t)(o)))    /* 16-bit unsigned      */
#define FARPTR(o) (DS + (uint16_t)(o))                      /* host ptr for RTL use */

/* ---- named scalar globals (see build/symbols.txt) ---- */
#define ball_x     W(0x98c)     /* ball position (fixed-point, see physics)  */
#define ball_y     W(0x98e)
#define p1_y       W(0x9e4)     /* player baseline Y positions               */
#define p2_y       W(0x9e6)
#define p1_frame   W(0x9be)     /* current animation frame index, player 1   */
#define p2_frame   W(0x9c0)
#define server     W(0x9d6)     /* which side serves (0/1)                   */
#define side_swap  W(0x9d8)
#define hit_count  W(0x9e2)
#define font_img   W(0x9c2)     /* captured "clear strip" image (dsptr)      */
#define dat_fd     W(0xa50)     /* AV.DAT file handle                        */
#define joy_timeout W(0xa42)
#define joy_val    W(0xa44)
#define sound_on   W(0x24e)

/* sprite-pointer globals just hold dsptr values, addressed like any other
 * word global (e.g. W(0x994 + frame*8 + shift*2)); nothing special needed. */

/* =========================== shim / RTL API ============================ */
/* memory */
dsptr   dos_malloc(uint16_t n);                 /* -> DS heap offset (0 = fail) */
void    movedata_ds(dsptr src, dsptr dst, uint16_t n);  /* memcpy within DS     */

/* AV.DAT file I/O (operates on the real file) */
int     sys_open(dsptr name, int mode);
int     sys_read(int fd, dsptr buf, uint16_t len);
int     sys_close(int fd);

/* console / misc RTL */
int     dos_printf(dsptr fmt);                  /* used only for fatal messages */
void    dos_exit(int code);

/* ---- CGA / BGI graphics (implemented over VIDEO[]) ---- */
void    cga_init(void);                          /* enter 320x200x4 (SDL window) */
void    cga_close(void);
int     graphresult(void);
uint16_t bgi_imagesize(int x1, int y1, int x2, int y2);
void    bgi_getimage(int x1, int y1, int x2, int y2, dsptr buf);
void    bgi_setfillstyle(int pattern, int color);
void    bgi_bar(int x1, int y1, int x2, int y2);
void    bgi_outtext(dsptr s);                    /* 0x63f4: text at current pos  */
void    bgi_outtextxy(int x, int y, dsptr s);    /* 0x6c42: text at (x,y)        */
void    bgi_settextstyle_dir(int dir);           /* lib_6b04: text direction     */
void    bgi_setcolor(int c);                     /* lib_6129 helper              */
void    bgi_cleardevice(void);                   /* lib_5d80                     */
void    bgi_rectangle(int x1, int y1, int x2, int y2); /* lib_5fca               */
void    bgi_fillellipse(int x, int y, int xr, int yr); /* lib_6129 (serve dot)    */
void    registerbgidriver(void);                 /* no-op                        */

/* ---- the game's own primitives (decompiled in game.c) ---- */
void    draw_sprite(int x, int y, dsptr img);    /* 0x22d0: blit sprite to VIDEO */
void    draw_sprite2(int x, int y, dsptr img);   /* 0x23e2: erase sprite region  */
void    wait_vsync(void);                        /* 0x22bf: wait CGA vretrace     */

/* ---- input (mapped by platform.c from SDL) ---- */
void    kb_install(void);                         /* no-op: SDL delivers key events */
void    av_kbd_scancode(unsigned char sc);        /* replicates the INT 9 ISR body  */
int     joy_read_button(int player);             /* game-port button (bit test)   */
int     joy_read_axis(int player, int *timeout);
int     mouse_read(int *buttons, int *dx, int *dy);
int     dos_int86(int intno, dsptr in_regs, dsptr out_regs); /* mouse INT 33h etc. */

/* PC-speaker sound */
void    speaker_tone(int divisor);               /* out 0x42/0x61 */
void    speaker_off(void);

/* platform frame pump / timing */
void    platform_pump(void);                     /* poll SDL, present VIDEO       */
int     platform_should_quit(void);
void    rtl_push_key(int biosval);               /* enqueue (scancode<<8)|ascii   */

#endif /* DOS_H */
