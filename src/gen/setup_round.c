/* 0x00bee setup_round - clear screen, draw court rectangle, net sprite, and score/label text for a new round */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int setup_round(void)
{
    wait_vsync();                       /* 00bee: call 0x22bf */
    bgi_cleardevice();                  /* 00bf1: lcall 0x5d80 */

    /* 00bf6..00c0b: push 0xc7,0x138,0xb,3 -> bgi_rectangle(x1,y1,x2,y2) (right-to-left) */
    bgi_rectangle(3, 0xb, 0x138, 0xc7);

    /* 00c0e..00c1d: push IMG(0x9b4),0x67,0x9e -> draw_sprite(x,y,img) (right-to-left) */
    draw_sprite(0x9e, 0x67, IMG(0x9b4));

    /* 00c20..00c31: push ds,0x35a,0,0x5a -> putimage==bgi_outtextxy(x,y,str) */
    bgi_outtextxy(0x5a, 0, 0x35a);
    /* 00c34..00c45: push ds,0x36c,0,0x28 -> bgi_outtextxy(0x28,0,0x36c) */
    bgi_outtextxy(0x28, 0, 0x36c);
    /* 00c48..00c59: push ds,0x36e,0,0x10e -> bgi_outtextxy(0x10e,0,0x36e) */
    bgi_outtextxy(0x10e, 0, 0x36e);

    return 0;                           /* 00c5c: ret (no meaningful AX) */
}

/* NOTE: 0x00c5d..0x00d4f is a SEPARATE routine that follows setup_round's ret:
 * it pushes all registers, loads DS=0x86a, reads scancode via `in al,0x60`,
 * ACKs via port 0x61 / `out 0x20`, updates key-repeat tables at 0x9c8/0x9e (stride 6)
 * against key definitions, and ends in `iret`. This is the INT 9 keyboard ISR.
 * Per the contract it is NOT emulated here (SDL feeds key state); the vector is
 * installed elsewhere via kb_install(). It is intentionally omitted from this
 * translation unit because it is not part of setup_round. */
