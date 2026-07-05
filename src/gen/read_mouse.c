/* 0x00de9 read_mouse - poll INT 33h mouse: store button + horizontal-motion direction flags for player a0 */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* Original uses a 16-byte union REGS block as a stack local at [bp-0x10].
 * In small model SS==DS so that local is itself a DS offset; we model it with
 * the same fixed DS scratch region as the other int86 callers.
 * Word layout (Turbo C union REGS.x): AX=+0, BX=+2, CX=+4, DX=+6.  */
#define REG_BLK 0xffe0

int read_mouse(int a0)
{
    dsptr r = REG_BLK;                 /* di=[bp+8]=a0; regs at [bp-0x10] */
    int di = a0;
    int si;
    unsigned bx;                       /* mul dx result -> bx = di*6 (struct index) */
    int ax;

    /* --- INT 33h AX=3: get button state (BX) and cursor position (CX,DX) --- */
    W(r + 0) = 3;                      /* mov word ptr [bp-0x10], 3 */
    dos_int86(0x33, r, r);             /* push &out; push &in; push 0x33; call int86 */

    ax = UW(r + 2) & 1;                /* mov ax,[bp-0xe](BX); and ax,1  (left button) */
    bx = (unsigned)di * 6;             /* mov ax,di; mov dx,6; mul dx; mov bx,ax */
    W(bx + 0x9cc) = ax;                /* mov word ptr [bx+0x9cc], ax */

    /* --- INT 33h AX=0xB: read relative motion counters (CX=dx, DX=dy) --- */
    W(r + 0) = 0xb;                    /* mov word ptr [bp-0x10], 0xb */
    dos_int86(0x33, r, r);             /* call int86 */

    si = W(r + 4);                     /* mov si,[bp-0xc](CX)  ; horizontal motion */

    /* rightward flag: (si>0 ? 1 : 0) << 1  -> 2 or 0, stored at [di*6+0x9ca] */
    if (si > 0)                        /* or si,si; jle 0xe43 */
        ax = 1;                        /* mov ax,1 */
    else
        ax = 0;                        /* xor ax,ax */
    ax = (short)((unsigned)ax << 1);   /* shl ax,1 */
    bx = (unsigned)di * 6;             /* mov ax,di; mov dx,6; mul dx; mov bx,ax */
    W(bx + 0x9ca) = ax;                /* mov word ptr [bx+0x9ca], ax */

    /* leftward flag: -(si<0 ? 1 : 0) << 1 -> -2 (0xFFFE) or 0, stored at [di*6+0x9c8] */
    if (si >= 0)                       /* or si,si; jge 0xe5f */
        ax = 0;                        /* xor ax,ax */
    else
        ax = 1;                        /* mov ax,1 */
    ax = (short)(-ax);                 /* neg ax */
    ax = (short)((unsigned)ax << 1);   /* shl ax,1 */
    bx = (unsigned)di * 6;             /* mov ax,di; mov dx,6; mul dx; mov bx,ax */
    W(bx + 0x9c8) = ax;                /* mov word ptr [bx+0x9c8], ax */

    return ax;                         /* AX at ret (last computed value) */
}
