/* 0x019f4 sub_019f4 - AI/serve target selection: choose paddle move and call collide_check */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* AX at the final `ret` is reproduced via the `ax` mirror below: every
 * instruction that writes AX updates `ax`, so `return ax` matches the binary
 * on every control-flow path.  collide_check() leaves its result in AX and the
 * following `add sp,6` does not disturb it, so `ax = collide_check(...)`. */
int sub_019f4(void)
{
    int ax, bx, dx;          /* register mirrors */
    int si = 0, di;          /* saved registers used as locals */
    int loc2;                /* [bp-2] */
    int loc4;                /* [bp-4] */
    int loc6;                /* [bp-6] */

    W(0x9cc) = 0;                                        /* 019fc */

    ax = W(0x98a);                                       /* 01a02 */
    if (ax >= W(0xa4d)) goto L1a0e;                      /* 01a05 jge */
    W(0xa4d) = ax;                                       /* 01a0b */
L1a0e:
    /* dx = hit_count % 10 (signed);  loc2 = 5 - dx */
    ax = hit_count;                                      /* 01a0e [0x9e2] */
    bx = 0xa;                                            /* 01a11 */
    dx = ax % bx;                                        /* 01a14 cdq / 01a15 idiv bx */
    ax = 5;                                              /* 01a17 */
    ax = ax - dx;                                        /* 01a1a sub ax,dx */
    loc2 = ax;                                           /* 01a1c [bp-2] */

    if (W(0xa4b) != 0) goto L1a29;                       /* 01a1f/01a24 jne */
    goto L1b3e;                                          /* 01a26 */
L1a29:
    if ((UW(0xa14) & 1) == 0) goto L1a34;                /* 01a29/01a2f je */
    goto L1b3e;                                          /* 01a31 */
L1a34:
    ax = W(0xa3e);                                       /* 01a34 */
    if ((unsigned)ax <= 5) goto L1a3f;                   /* 01a37/01a3a jbe */
    goto L1b37;                                          /* 01a3c */
L1a3f:
    /* switch(ax) 6 cases via jump table at 0x1a48 */
    switch (ax) {                                        /* 01a3f-01a43 */
        case 0: goto L1a54;
        case 1: goto L1a6a;
        case 2: goto L1a80;
        case 3: goto L1a96;
        case 4: goto L1abb;
        case 5: goto L1af6;
    }

L1a54:
    ax = collide_check(0, 0x37, 2);                     /* 01a54-01a62 */
    si = ax;                                            /* 01a65 */
    goto L1b37;
L1a6a:
    ax = collide_check(0, 0x54, 2);                     /* 01a6a-01a78 */
    si = ax;                                            /* 01a7b */
    goto L1b37;
L1a80:
    ax = collide_check(0, 0x50, 2);                     /* 01a80-01a8e */
    si = ax;                                            /* 01a91 */
    goto L1b37;
L1a96:
    if (W(0x9b8) != 0) goto L1ae3;                       /* 01a96/01a9b jne */
    ax = collide_check(0, 0x2c, 2);                     /* 01a9d-01aab */
    si = ax;                                            /* 01aae */
    ax = si;                                            /* 01ab0 */
    if (ax != 0) goto L1ab9;                             /* 01ab2/01ab4 jne */
    goto L1b37;                                          /* 01ab6 */
L1ab9:
    goto L1adb;                                          /* 01ab9 */
L1abb:
    if (W(0x9b8) != 0) goto L1ae3;                       /* 01abb/01ac0 jne */
    ax = collide_check(0, 0x5a, 2);                     /* 01ac2-01ad0 */
    si = ax;                                            /* 01ad3 */
    ax = si;                                            /* 01ad5 */
    if (ax == 0) goto L1b37;                             /* 01ad7/01ad9 je */
L1adb:
    W(0x9b8) = 1;                                        /* 01adb */
    goto L1b37;                                          /* 01ae1 */
L1ae3:
    ax = collide_check(0, 0x3a, 2);                      /* 01ae3-01af1 */
    goto L1b34;                                          /* 01af4 */

L1af6:
    if (W(0x9b8) != 0) goto L1b1c;                       /* 01af6/01afb jne */
    ax = collide_check(0, 3, 2);                        /* 01afd-01b0b */
    if (ax == 0) goto L1b18;                             /* 01b0e/01b10 je */
    W(0x9b8) = 1;                                        /* 01b12 */
L1b18:
    si = 0;                                             /* 01b18 xor si,si */
    goto L1b37;                                         /* 01b1a */
L1b1c:
    /* arg = pre-increment W(0x9b8) + 8, then W(0x9b8)++ */
    ax = W(0x9b8);                                      /* 01b20 */
    W(0x9b8) = W(0x9b8) + 1;                            /* 01b23 inc [0x9b8] */
    ax = ax + 8;                                        /* 01b27 */
    ax = collide_check(0, ax, 1);                       /* 01b1c-01b31 */
L1b34:
    si = 1;                                             /* 01b34 */
L1b37:
    W(0x9cc) = si;                                      /* 01b37 (AX unchanged) */
    goto L1ce4;                                         /* 01b3b */

L1b3e:
    if (W(0x9b6) > 0) goto L1b48;                       /* 01b3e/01b43 jg */
    goto L1cd3;                                         /* 01b45 */
L1b48:
    if (W(0x9c6) < 0x8c) goto L1b53;                    /* 01b48/01b4e jl */
    goto L1cd3;                                         /* 01b50 */
L1b53:
    ax = W(0x9b6) >> 6;                                 /* 01b53-01b59 sar ax,6 (signed) */
    if (ax != 0) goto L1b66;                            /* 01b5b/01b5d jne */
    loc6 = 0;                                           /* 01b5f [bp-6] */
    goto L1b7c;                                         /* 01b64 */
L1b66:
    ax = 0x8c - W(0x98a);                               /* 01b66-01b69 */
    bx = W(0x9b6) >> 6;                                 /* 01b6d-01b74 sar bx,6 */
    loc6 = ax / bx;                                     /* 01b76-01b77 idiv bx (signed) */
L1b7c:
    if (loc6 < 1) goto L1b8e;                           /* 01b7c/01b80 jl */
    ax = W(0x988) >> 6;                                 /* 01b82-01b88 sar ax,6 */
    if (ax != 0) goto L1b94;                            /* 01b8a/01b8c jne */
L1b8e:
    di = W(0x9c6);                                      /* 01b8e */
    goto L1ba8;                                         /* 01b92 */
L1b94:
    /* di = (unsigned16)((W(0x988)>>6) * loc6) + W(0x9c6) - 4  (mul low 16 bits) */
    ax = (int)(unsigned short)((unsigned)(unsigned short)(W(0x988) >> 6)
                               * (unsigned)(unsigned short)loc6);  /* 01b94-01b9c mul [bp-6] */
    di = ax;                                            /* 01b9f */
    di = di + W(0x9c6);                                 /* 01ba1 */
    di = di + (-4);                                     /* 01ba5 add di,-4 */
L1ba8:
    loc4 = ball_x - W(0x9c6);                           /* 01ba8-01baf [0x98c]-[0x9c6] */

    /* --- abs(W(0x988)) >= 0x80 ? --- */
    ax = W(0x988);                                      /* 01bb2 */
    if (ax >= 0) goto L1bbb;                            /* 01bb5/01bb7 jge */
    ax = -ax;                                           /* 01bb9 neg */
L1bbb:
    if (ax >= 0x80) goto L1c18;                         /* 01bbb/01bbe jge */
    if (W(0xa4d) >= 0x4b) goto L1c18;                   /* 01bc0/01bc5 jge */

    /* dx = (W(0x98a) < 0x9e) ^ (W(0x988) < 0) */
    if (W(0x98a) >= 0x9e) goto L1bd4;                   /* 01bc7/01bcd jge */
    ax = 1;                                             /* 01bcf */
    goto L1bd6;                                         /* 01bd2 */
L1bd4:
    ax = 0;                                             /* 01bd4 xor ax,ax */
L1bd6:
    dx = ax;                                            /* 01bd6 push ax */
    if (W(0x988) >= 0) goto L1be3;                      /* 01bd7/01bdc jge */
    ax = 1;                                             /* 01bde */
    goto L1be5;                                         /* 01be1 */
L1be3:
    ax = 0;                                             /* 01be3 xor ax,ax */
L1be5:
    dx = dx ^ ax;                                       /* 01be5-01be6 pop dx / xor dx,ax */
    if (dx == 0) goto L1c01;                            /* 01be8 je */
    /* dx != 0: 0xfff1 = -15. The 16-bit sum is pushed and collide_check
       compares it signed (cmp di,a1 / jge), so model as (short). */
    ax = collide_check(0, (short)(W(0x9c6) + 0xfff1), 3);          /* 01bea-01bfb */
    goto L1ce4;                                         /* 01bfe */
L1c01:
    ax = collide_check(0, (short)(W(0x9c6) + 0xf), 3);             /* 01c01-01c12 */
    goto L1ce4;                                         /* 01c15 */

L1c18:
    if (W(0x98a) > 0x82) goto L1c23;                    /* 01c18/01c1e jg */
    goto L1ca6;                                         /* 01c20 */
L1c23:
    ax = loc4;                                          /* 01c23 */
    if (ax >= 0) goto L1c2c;                            /* 01c26/01c28 jge */
    ax = -ax;                                           /* 01c2a neg */
L1c2c:
    if (ax <= 6) goto L1c62;                            /* 01c2c/01c2f jle */
    ax = W(0x988);                                      /* 01c31 */
    if (ax >= 0) goto L1c3a;                            /* 01c34/01c36 jge */
    ax = -ax;                                           /* 01c38 neg */
L1c3a:
    if (ax >= 0x400) goto L1c62;                        /* 01c3a/01c3d jge */
    /* dx = W(0x9c6) - ((ball_x + 0xffc4) >> 3);  0xffc4 = -60, sar x3 */
    ax = (int)(short)(ball_x + 0xffc4);                 /* 01c43-01c46 */
    ax = ax >> 1;                                       /* 01c49 sar */
    ax = ax >> 1;                                       /* 01c4b sar */
    ax = ax >> 1;                                       /* 01c4d sar */
    dx = W(0x9c6);                                      /* 01c4f */
    dx = dx - ax;                                       /* 01c53 */
    ax = collide_check(0, dx, 3);                       /* 01c3f-01c5c */
    goto L1ce4;                                         /* 01c5f */
L1c62:
    /* ax = ((ball_x + 0xffc4) >> 3) + (W(0x9c6) + loc2) */
    ax = (int)(short)(ball_x + 0xffc4);                 /* 01c66-01c69 */
    ax = ax >> 1;                                       /* 01c6c sar */
    ax = ax >> 1;                                       /* 01c6e sar */
    ax = ax >> 1;                                       /* 01c70 sar */
    dx = W(0x9c6);                                      /* 01c72 */
    dx = dx + loc2;                                     /* 01c76 add dx,[bp-2] */
    ax = ax + dx;                                       /* 01c79 */
    ax = collide_check(0, ax, 0xa);                     /* 01c62-01c82 */

    /* W(0x9cc) = (ball_x < 0x69 && !(server!=0 || W(0x9dc)>=2)) ? 1 : 0 */
    if (ball_x >= 0x69) goto L1c9f;                     /* 01c85/01c8a jge */
    if (server != 0) goto L1c9a;                        /* 01c8c/01c91 jne */
    if (W(0x9dc) >= 2) goto L1c9f;                      /* 01c93/01c98 jge */
L1c9a:
    ax = 1;                                             /* 01c9a */
    goto L1ca1;                                         /* 01c9d */
L1c9f:
    ax = 0;                                             /* 01c9f xor ax,ax */
L1ca1:
    W(0x9cc) = ax;                                      /* 01ca1 */
    goto L1ce4;                                         /* 01ca4 */

L1ca6:
    if (di >= 3) goto L1cb2;                            /* 01ca6/01ca9 jge */
    ax = 6 - di;                                        /* 01cab-01cae */
    di = ax;                                            /* 01cb0 */
L1cb2:
    if (di <= 0x7b) goto L1cbe;                         /* 01cb2/01cb5 jle */
    ax = 0xf6 - di;                                     /* 01cb7-01cba */
    di = ax;                                            /* 01cbc */
L1cbe:
    ax = di + loc2;                                     /* 01cc2-01cc4 */
    ax = collide_check(0, ax, 3);                       /* 01cbe-01cce */
    goto L1ce4;                                         /* 01cd1 */

L1cd3:
    ax = collide_check(0, 0x38, 8);                     /* 01cd3-01ce1 */
L1ce4:
    return ax;                                          /* 01ce4-01ce9 ret (AX) */
}
