/* 0x00bee setup_round - clear screen, draw court rectangle, net sprite, and score/label text for a new round */
#include "dos.h"
#include "game_protos.h"

int setup_round(void)
{
    wait_vsync();
    bgi_cleardevice();

    /* court border rectangle (x1,y1,x2,y2) */
    bgi_rectangle(3, 0xb, 0x138, 0xc7);

    /* net sprite at the center of the court */
    draw_sprite(0x9e, 0x67, net_sprite);

    /* header strings: title 0x35a and the two score labels 0x36c / 0x36e */
    bgi_outtextxy(0x5a, 0, 0x35a);
    bgi_outtextxy(0x28, 0, 0x36c);
    bgi_outtextxy(0x10e, 0, 0x36e);

    return 0;
}

/* NOTE: 0x00c5d..0x00d4f is a SEPARATE routine that follows setup_round's ret:
 * it pushes all registers, loads DS=0x86a, reads scancode via `in al,0x60`,
 * ACKs via port 0x61 / `out 0x20`, updates key-repeat tables at 0x9c8/0x9e (stride 6)
 * against key definitions, and ends in `iret`. This is the INT 9 keyboard ISR.
 * Per the contract it is NOT emulated here (SDL feeds key state); the vector is
 * installed elsewhere via kb_install(). It is intentionally omitted from this
 * translation unit because it is not part of setup_round. */
