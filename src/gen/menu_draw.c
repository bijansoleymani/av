/* 0x00779 menu_draw - draw the options menu, run its input loop, apply +/-/enter, and clear on exit */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* RTL / library helpers reached by this function.  Their addresses (0x432b,
 * 0x4104, 0x4b2b, 0x4b57) lie in the Turbo C runtime region above the game
 * code, so they are not decompiled game functions.  By usage:
 *   sub_0432b() -> AX flag : input poll (kbhit-style; nonzero when a key is up)
 *   sub_04104() -> AX      : blocking key read (getch-style), returns scancode/char
 *   sub_04b2b(int ms)      : delay(ms) busy-wait
 *   sub_04b57(void)        : delay/sound teardown paired with sub_04b2b
 * They are called here by their sub name to stay faithful to the bytes. */
extern int  sub_0432b(void);
extern int  sub_04104(void);
extern void sub_04b2b(int ms);
extern void sub_04b57(void);

int menu_draw(void)
{
    /* locals mirroring the [bp-N] frame slots */
    int  bpm6 = 0x68;         /* [bp-6]  first text/X coord (104)            */
    int  bpm4 = 0x70;         /* [bp-4]  second text/X coord (112)           */
    int  di   = 0x28;         /* row base offset (40); row Y = si*8 + di     */
    int  si;                  /* current menu row index                      */
    int  bpm8;                /* [bp-8]  clear-loop counter                  */
    unsigned char key;        /* [bp-1]  last key read (byte)                */
    unsigned bx;              /* struct base offset (si * 0x3c)              */
    int  ax, dx;

    /* 0x0781..0x078e prologue slot inits done above; di=0x28, si=0 */

    /* --- initial pass: erase/prime the 7 menu rows (draw_sprite2) --- */
    si = 0;                                       /* 0x078e xor si,si */
    goto L7ab;                                    /* 0x0790 jmp 0x7ab */
L792:
    /* 0x0792 draw_sprite2([bp-6]=0x68, si*8 + di, font_img) */
    draw_sprite2(bpm6, (si << 3) + di, font_img);
    /* 0x07aa inc si */
    si++;
L7ab:
    if (si < 7) goto L792;                        /* 0x07ab cmp si,7 ; jl */

    /* 0x07b0 wait_vsync */
    wait_vsync();

    /* --- draw each row's current-value label via putimage (text) --- */
    si = 0;                                       /* 0x07b3 xor si,si */
    goto L7f4;                                    /* 0x07b5 jmp 0x7f4 */
L7b7:
    bx = (unsigned)si * 0x3c;                     /* 0x07b7 si*0x3c */
    /* label addr = struct[si].value*0xe + si*0x3c + 0xae */
    ax = (int)((unsigned)W(bx + 0xaa) * 0xe);     /* 0x07c0 value * 0x0e */
    dx = (int)((unsigned)si * 0x3c);              /* 0x07ca si*0x3c */
    ax = ax + dx;                                 /* 0x07d4 add ax,dx */
    ax = ax + 0xae;                               /* 0x07d6 add ax,0xae */
    /* 0x07db push ds ; 0x07dc push ax (far ptr => dsptr string) */
    /* 0x07dd..7e5 ax = si*8 + di ; 0x07e8 push [bp-4]=0x70 */
    /* lcall 0x6c42 -> bgi_outtextxy(x, y, str) */
    bgi_outtextxy(bpm4, (si << 3) + di, (dsptr)ax);
    /* 0x07f3 inc si */
    si++;
L7f4:
    if (si < 7) goto L7b7;                         /* 0x07f4 cmp si,7 ; jl */

    /* 0x07f9 sub_0234e(0x68, di, font_img) : highlight row 0's strip */
    sub_0234e(bpm6, di, font_img);

    si  = 0;                                       /* 0x0807 xor si,si */
    key = 0;                                       /* 0x0809 mov [bp-1],0 */

    /* ================= main menu input loop ================= */
L80d:
    /* 0x080d ax = sub_0063f() (returns key in AL) */
    key = (unsigned char)(sub_0063f() & 0xff);     /* 0x0810 mov [bp-1],al */
    if (key == 0xd) goto L882;                      /* 0x0813 cmp al,0xd ; je */

    /* 0x0818 if (sub_0432b()) key = sub_04104() */
    if (sub_0432b() != 0) {                         /* 0x081b or ax,ax ; je 0x825 */
        key = (unsigned char)(sub_04104() & 0xff);  /* 0x0822 mov [bp-1],al */
    }
    /* 0x0825 (je target): fall through to delay/redraw */
    /* 0x0825 sub_04b2b(0xfa0) ; 0x082e sub_04b57() ; 0x0831 wait_vsync */
    sub_04b2b(0xfa0);
    sub_04b57();
    wait_vsync();

    /* 0x0834 sub_0234e(0x68, si*8+di, font_img) : redraw highlight strip */
    sub_0234e(bpm6, (si << 3) + di, font_img);

    /* 0x084c handle Down keys (0x32 '2' / 0x50 keypad-down) : next row */
    if (key == 0x32 || key == 0x50) {              /* 0x084c/0x0852 */
        si = si + 1;                               /* 0x0858 inc si */
        /* 0x0859 si = (si) % 7  (signed idiv by 7, remainder) */
        si = si % 7;                               /* 0x085f idiv bx ; si=dx */
    }
    /* 0x0863 handle Up keys (0x38 '8' / 0x48 keypad-up) : prev row */
    if (key == 0x38 || key == 0x48) {              /* 0x0863/0x0869 */
        /* 0x0872 si = (si + 6) % 7 */
        si = (si + 6) % 7;                          /* 0x087b idiv bx ; si=dx */
        goto L919;                                  /* 0x087f jmp 0x919 */
    }
    goto L919;                                      /* 0x086f jmp 0x919 (default) */

    /* ---- Enter pressed : advance the selected row's value ---- */
L882:
    bx = (unsigned)si * 0x3c;                       /* 0x0882 si*0x3c */
    if (W(bx + 0xac) > 0) goto L895;                /* 0x088b cmp [bx+0xac],0 ; jg */
    goto L934;                                      /* 0x0892 jmp 0x934 (no choices -> exit) */
L895:
    bx = (unsigned)si * 0x3c;                       /* 0x0895 si*0x3c */
    W(bx + 0xaa) = W(bx + 0xaa) + 1;                /* 0x089e inc [bx+0xaa] */
    ax = W(bx + 0xaa);                              /* 0x08a2 ax = [bx+0xaa] */
    /* 0x08b1 ax = ax % [bx+0xac]  (signed idiv, remainder in dx) */
    {
        unsigned bx2 = (unsigned)si * 0x3c;         /* 0x08a7 recompute base */
        int m = W(bx2 + 0xac);                      /* 0x08b2 divisor */
        int rem = ax % m;                           /* remainder */
        /* 0x08b8 recompute base ; store remainder into [bx+0xaa] */
        unsigned bx3 = (unsigned)si * 0x3c;
        W(bx3 + 0xaa) = rem;                        /* 0x08c1 mov [bx+0xaa],ax */
    }

    /* 0x08c5 draw_sprite2(0x68, si*8+di, font_img) : erase old label */
    draw_sprite2(bpm6, (si << 3) + di, font_img);

    /* 0x08dd putimage: draw new label for the selected row */
    bx = (unsigned)si * 0x3c;                       /* 0x08dd si*0x3c */
    ax = (int)((unsigned)W(bx + 0xaa) * 0xe);       /* 0x08e6 value * 0x0e */
    dx = (int)((unsigned)si * 0x3c);                /* 0x08f0 si*0x3c */
    ax = ax + dx;                                   /* 0x08fa add ax,dx */
    ax = ax + 0xae;                                 /* 0x08fc add ax,0xae */
    /* 0x0911 lcall 0x6c42 -> bgi_outtextxy([bp-4]=0x70, si*8+di, str) */
    bgi_outtextxy(bpm4, (si << 3) + di, (dsptr)ax);

    /* ---- common tail: redraw highlight strip, loop ---- */
L919:
    /* 0x0919 sub_0234e(0x68, si*8+di, font_img) */
    sub_0234e(bpm6, (si << 3) + di, font_img);
    goto L80d;                                      /* 0x0931 jmp 0x80d */

    /* ---- exit path : clear all 7 rows and return ---- */
L934:
    bpm8 = 0;                                        /* 0x0934 [bp-8]=0 */
    goto L957;                                       /* 0x0939 jmp 0x957 */
L93b:
    /* 0x093b draw_sprite2(0x68, bpm8*8 + di, font_img) */
    draw_sprite2(bpm6, (bpm8 << 3) + di, font_img);
    bpm8++;                                          /* 0x0954 inc [bp-8] */
L957:
    if (bpm8 < 7) goto L93b;                         /* 0x0957 cmp [bp-8],7 ; jl */

    /* 0x095d mov ax,si  (return current row index in AX) */
    return si;                                       /* 0x0964 ret */
}
