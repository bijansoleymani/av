/* 0x00724 joy_calibrate - measure joystick game-port axis pulse timing and set joy_val/joy_timeout */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* Reads the game port (0x201, di) bit 0 as the axis-line one-shot state.
 * The original does raw out/in on port 0x201; we read bit 0 of the axis
 * line via joy_read_button(0) (returns the current bit-0 level 0/1). */
int joy_calibrate(void)
{
    int si;           /* si: timing counter (signed 16-bit)      */
    unsigned al;      /* al: game-port byte read                 */
    int ax;           /* ax: return / scratch                    */

    /* 00726: mov di,0x201  (game port; strobe below starts a one-shot) */
    joy_timeout = 0x7d00;                 /* 00729: [0xa42]=0x7d00 */
    si = 0;                               /* 0072f: xor si,si      */

    al = 0x7f;                            /* 00731: mov al,0x7f    */
    /* 00733/00735: out 0x201,al  -- strobe game port to start conversion */
    (void)al;

L736:
    al = joy_read_button(0) & 1;          /* 00736/00738: in al,dx (bit0 of port 0x201) */
    if ((al & 1) != 0) goto L747;         /* 00739/0073c: test al,1 ; jne 0x747 */
    if (si >= joy_timeout) goto L747;     /* 0073e/00742: cmp si,[0xa42] ; jge 0x747 (signed) */
    si++;                                 /* 00744: inc si */
    goto L736;                            /* 00745: jmp 0x736 */

L747:
    if (si < 0) goto L762;                /* 00747/00749: or si,si ; jl 0x762 (signed) */

    si = 0;                               /* 0074b: xor si,si */
L74d:
    al = joy_read_button(0) & 1;          /* 0074d/0074f: in al,dx (bit0 of port 0x201) */
    if ((al & 1) == 0) goto L75e;         /* 00750/00753: test al,1 ; je 0x75e */
    if (si >= joy_timeout) goto L75e;     /* 00755/00759: cmp si,[0xa42] ; jge 0x75e (signed) */
    si++;                                 /* 0075b: inc si */
    goto L74d;                            /* 0075c: jmp 0x74d */

L75e:
    if (si >= 0) goto L766;               /* 0075e/00760: or si,si ; jge 0x766 (signed) */

L762:
    ax = 0;                               /* 00762: xor ax,ax */
    goto L776;                            /* 00764: jmp 0x776 */

L766:
    joy_val = si;                         /* 00766: [0xa44]=si */
    ax = si;                              /* 0076a: mov ax,si */
    ax = ax >> 1;                         /* 0076c: sar ax,1 (signed) */
    ax = (short)(ax + si);                /* 0076e: add ax,si (16-bit wrap) */
    joy_timeout = ax;                     /* 00770: [0xa42]=ax */
    ax = 1;                               /* 00773: mov ax,1 */

L776:
    return ax;                            /* 00776..00778: pop di/si ; ret (AX) */
}
