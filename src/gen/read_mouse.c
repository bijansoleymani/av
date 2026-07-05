/* 0x00de9 read_mouse - poll INT 33h mouse: store button + horizontal-motion direction flags for player a0 */
#include "dos.h"
#include "game_protos.h"

/* Original uses a 16-byte union REGS block as a stack local at [bp-0x10].
 * In small model SS==DS so that local is itself a DS offset; we model it with
 * the same fixed DS scratch region as the other int86 callers.
 * Word layout (Turbo C union REGS.x): AX=+0, BX=+2, CX=+4, DX=+6.  */
#define REG_BLK 0xffe0

int read_mouse(int player)
{
    dsptr regs = REG_BLK;
    int motion_x;      /* CX from INT 33h AX=0xB: relative horizontal motion */
    int flag;          /* per-direction control word (-2 / 0 / 2) */

    /* --- INT 33h AX=3: get button state (BX) --- */
    W(regs + 0) = 3;
    dos_int86(0x33, regs, regs);

    /* jump/press: left mouse button = bit 0 of BX */
    ctrl_jump(player) = UW(regs + 2) & 1;

    /* --- INT 33h AX=0xB: read relative motion counters (CX = dx) --- */
    W(regs + 0) = 0xb;
    dos_int86(0x33, regs, regs);

    motion_x = W(regs + 4);

    /* rightward flag: (dx > 0 ? 1 : 0) << 1  ->  2 or 0 */
    flag = (short)((unsigned)(motion_x > 0 ? 1 : 0) << 1);
    ctrl_right(player) = flag;

    /* leftward flag: -(dx < 0 ? 1 : 0) << 1  ->  -2 (0xFFFE) or 0 */
    flag = (short)((unsigned)(short)(-(motion_x < 0 ? 1 : 0)) << 1);
    ctrl_left(player) = flag;

    return flag;   /* AX at ret = last computed (leftward) flag */
}
