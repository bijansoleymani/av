/* 0x0d50 read_joystick - poll one player's game-port joystick: store button + X-axis direction bits */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* Reads the game port (0x201, di) for player a0.
 * The original does raw in/out on port 0x201; bit 4 (0x10) is the button
 * (active-low) and bit 0 is the X-axis one-shot line.  We read those bits via
 * joy_read_button(a0):  &0x10 -> button, &1 -> axis line level (0/1).
 * The two busy-loops measure the axis one-shot pulse width exactly as the
 * binary does; the first loop's count is discarded (xor si,si at 0xd8f) and
 * only the second loop (high-pulse width) drives the result. */
int read_joystick(int a0)   /* a0 = player index, [bp+8] */
{
    int si;          /* si: axis timing counter (signed 16-bit) */
    int ax;          /* ax register (16-bit signed model)       */
    unsigned al;     /* al: game-port byte read                 */
    unsigned bx;     /* bx: player*6 index base                 */

    si = 0;                                  /* 00d58: xor si,si */

    /* ---- button: in al,0x201; test al,0x10 -> pressed?0:1 (active-low) ---- */
    al = (unsigned)joy_read_button(a0);      /* 00d5c: in al,dx (port 0x201) */
    if ((al & 0x10) != 0)                    /* 00d5d/00d60: test al,0x10 ; jne 0xd67 */
        ax = 0;                              /* 00d67: xor ax,ax */
    else
        ax = 1;                              /* 00d62: mov ax,1  */
    /* store button state at [player*6 + 0x9cc] (00d6a..00d75) */
    bx = (unsigned)(a0 * 6);                 /* 00d6d/00d70: mov dx,6 ; mul dx */
    W(0x9cc + bx) = (short)ax;               /* 00d75: mov [bx+0x9cc],ax */

    /* ---- strobe the game port to start the axis one-shot (00d79..00d7d) ---- */
    al = 0xff;                               /* 00d79: mov al,0xff */
    /* 00d7b/00d7d: out 0x201,al -- strobe (no observable effect in model) */
    (void)al;

    /* ---- loop 1: wait while axis bit 0 is LOW, up to joy_timeout (00d7e..00d8d) */
L7e:
    al = (unsigned)joy_read_button(a0);      /* 00d80: in al,dx */
    if ((al & 1) != 0) goto L8f;             /* 00d81/00d84: test al,1 ; jne 0xd8f */
    if (si >= joy_timeout) goto L8f;         /* 00d86/00d8a: cmp si,[0xa42] ; jge 0xd8f (signed) */
    si++;                                    /* 00d8c: inc si */
    goto L7e;                                /* 00d8d: jmp 0xd7e */

L8f:
    si = 0;                                  /* 00d8f: xor si,si (discard loop-1 count) */

    /* ---- loop 2: measure while axis bit 0 is HIGH, up to joy_timeout (00d91..00da0) */
L91:
    al = (unsigned)joy_read_button(a0);      /* 00d93: in al,dx */
    if ((al & 1) == 0) goto La2;             /* 00d94/00d97: test al,1 ; je 0xda2 */
    if (si >= joy_timeout) goto La2;         /* 00d99/00d9d: cmp si,[0xa42] ; jge 0xda2 (signed) */
    si++;                                    /* 00d9f: inc si */
    goto L91;                                /* 00da0: jmp 0xd91 */

La2:
    /* ---- axis threshold: (joy_val>>1) >= si ? 0 : 1, then <<1 -> {0,2} (00da2..00dc0) ---- */
    ax = (short)joy_val;                     /* 00da2: mov ax,[0xa44] */
    ax = ax >> 1;                            /* 00da5: sar ax,1 (signed) */
    if (ax >= si)                            /* 00da7/00da9: cmp ax,si ; jge 0xdb0 */
        ax = 0;                              /* 00db0: xor ax,ax */
    else
        ax = 1;                              /* 00dab: mov ax,1 */
    ax = (short)((unsigned)ax << 1);         /* 00db2: shl ax,1 -> 0 or 2 */
    bx = (unsigned)(a0 * 6);                 /* 00db8/00dbb: mov dx,6 ; mul dx */
    W(0x9ca + bx) = (short)ax;               /* 00dc0: mov [bx+0x9ca],ax */

    /* ---- timeout flag: si >= joy_timeout ? 0 : 1, neg, <<1 -> {0,-2} (00dc4..00de1) ---- */
    if (si >= joy_timeout)                   /* 00dc4/00dc8: cmp si,[0xa42] ; jge 0xdcf (signed) */
        ax = 0;                              /* 00dcf: xor ax,ax */
    else
        ax = 1;                              /* 00dca: mov ax,1 */
    ax = (short)(-ax);                       /* 00dd1: neg ax */
    ax = (short)((unsigned)ax << 1);         /* 00dd3: shl ax,1 -> 0 or -2 */
    bx = (unsigned)(a0 * 6);                 /* 00dd9/00ddc: mov dx,6 ; mul dx */
    W(0x9c8 + bx) = (short)ax;               /* 00de1: mov [bx+0x9c8],ax */

    return ax;                               /* AX at ret = last stored value {0,-2} */
}
