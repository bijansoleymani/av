/* 0x01cea sub_01cea - AI: steer paddle toward predicted ball landing / block spot */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_01cea(void)
{
    int ax, bx, dx;      /* general regs (16-bit signed) */
    int si = 0, di;      /* saved regs used as locals */
    long ldiv;           /* for idiv (32-bit dividend) */
    int loc2, loc4, loc6;/* [bp-2], [bp-4], [bp-6] */

    W(0x9d2) = 0;                                /* 01cf2 */

    ax = W(0x98a);                               /* 01cf8 */
    if (ax >= W(0xa4d)) goto L1d04;              /* 01cfb/01cff jge */
    W(0xa4d) = ax;                               /* 01d01 */
L1d04:
    /* [bp-2] = 5 - (hit_count % 10)  (signed idiv) */
    ax = hit_count;                              /* 01d04 [0x9e2] */
    bx = 0xa;                                    /* 01d07 */
    ldiv = (long)ax;                             /* cdq (sign-extend into dx:ax) */
    dx = (int)(ldiv % bx);                       /* idiv bx -> remainder in dx */
    ax = 5;                                      /* 01d0d */
    ax = ax - dx;                                /* 01d10 sub ax,dx */
    loc2 = ax;                                   /* 01d12 [bp-2] */

    if (W(0xa4b) != 0) goto L1d1f;               /* 01d15/01d1a jne */
    goto L1e56;                                  /* 01d1c */
L1d1f:
    ax = W(0xa14);                               /* 01d1f */
    ax = ax & 1;                                 /* 01d22 */
    if (ax == 1) goto L1d2d;                     /* 01d25/01d28 je */
    goto L1e56;                                  /* 01d2a */
L1d2d:
    ax = W(0xa3e);                               /* 01d2d */
    if ((unsigned)ax <= 5u) goto L1d38;          /* 01d30/01d33 jbe */
    goto L1e4f;                                  /* 01d35 */
L1d38:
    /* switch jump table, cases 0..5 (bx = ax*2 index) */
    switch (ax) {                                /* 01d3a shl / 01d3c jmp cs:[bx+..] */
        case 0: goto L1d4d;
        case 1: goto L1d64;
        case 2: goto L1d7b;
        case 3: goto L1d92;
        case 4: goto L1dcd;
        case 5: goto L1e0a;
    }

L1d4d:
    si = collide_check(1, 0xe8, 2);              /* 01d4d..01d5f */
    goto L1e4f;

L1d64:
    si = collide_check(1, 0xca, 2);              /* 01d64..01d78 */
    goto L1e4f;

L1d7b:
    si = collide_check(1, 0xd0, 2);              /* 01d7b..01d8f */
    goto L1e4f;

L1d92:
    if (W(0x9b8) != 0) goto L1db8;               /* 01d92/01d97 jne */
    si = collide_check(1, 0xfa, 2);              /* 01d99..01dab */
    ax = si;                                     /* 01dad */
    if (ax != 0) goto L1db6;                      /* 01daf or / 01db1 jne */
    goto L1e4f;                                  /* 01db3 */
L1db6:
    goto L1dee;                                  /* 01db6 */
L1db8:
    collide_check(1, 0xdc, 2);                   /* 01db8..01dc7 */
    goto L1e4c;                                  /* 01dca */

L1dcd:
    if (W(0x9b8) != 0) goto L1df6;               /* 01dcd/01dd2 jne */
    si = collide_check(1, 0xbe, 2);              /* 01dd4..01de6 */
    ax = si;                                     /* 01de8 */
    if (ax == 0) goto L1e4f;                      /* 01dea or / 01dec je */
L1dee:
    W(0x9b8) = 1;                                /* 01dee */
    goto L1e4f;                                  /* 01df4 */
L1df6:
    collide_check(1, 0xe6, 2);                   /* 01df6..01e05 */
    goto L1e4c;                                  /* 01e08 */

L1e0a:                                           /* switch case 5 */
    if (W(0x9b8) != 0) goto L1e31;               /* 01e0a/01e0f jne */
    ax = collide_check(1, 0x115, 2);             /* 01e11..01e20 */
    if (ax == 0) goto L1e2d;                      /* 01e23 or / 01e25 je */
    W(0x9b8) = 1;                                /* 01e27 */
L1e2d:
    si = 0;                                      /* 01e2d xor si,si */
    goto L1e4f;                                  /* 01e2f */
L1e31:
    ax = W(0x9b8);                               /* 01e35 (value before inc) */
    W(0x9b8) = W(0x9b8) + 1;                     /* 01e38 inc [0x9b8] */
    dx = 0x110 - ax;                             /* 01e3c/01e3f */
    collide_check(1, dx, 1);                     /* 01e42..01e49 (arg0=1,arg1=dx,arg2=1) */
L1e4c:
    si = 1;                                      /* 01e4c */
L1e4f:
    W(0x9d2) = si;                               /* 01e4f */
    goto L2002;                                  /* 01e53 */

L1e56:
    if (W(0x9b6) > 0) goto L1e60;                /* 01e56/01e5b jg */
    goto L1ff0;                                  /* 01e5d */
L1e60:
    if (W(0x9c6) > 0x7d) goto L1e6a;             /* 01e60/01e65 jg */
    goto L1ff0;                                  /* 01e67 */
L1e6a:
    ax = W(0x9b6) >> 6;                          /* 01e6a..01e70 sar ax,6 (arith) */
    if (ax != 0) goto L1e7d;                     /* 01e72 or / 01e74 jne */
    loc6 = 0;                                    /* 01e76 [bp-6]=0 */
    goto L1e93;                                  /* 01e7b */
L1e7d:
    /* [bp-6] = (0x8c - [0x98a]) / ([0x9b6] >> 6)  (signed idiv) */
    ax = 0x8c - W(0x98a);                        /* 01e7d/01e80 */
    bx = W(0x9b6) >> 6;                          /* 01e84..01e8b sar bx,6 */
    ldiv = (long)ax;                             /* cdq */
    loc6 = (int)(ldiv / bx);                     /* 01e8e idiv bx -> quotient */
L1e93:
    if (loc6 < 1) goto L1ea5;                    /* 01e93/01e97 jl */
    ax = W(0x988) >> 6;                          /* 01e99..01e9f sar ax,6 */
    if (ax != 0) goto L1eab;                     /* 01ea1 or / 01ea3 jne */
L1ea5:
    di = W(0x9c6);                               /* 01ea5 */
    goto L1ebf;                                  /* 01ea9 */
L1eab:
    ax = W(0x988) >> 6;                          /* 01eab..01eb1 sar ax,6 */
    /* mul [bp-6] : unsigned 16x16, low 16 bits */
    ax = (int)((unsigned)((unsigned short)ax * (unsigned short)loc6)); /* 01eb3 */
    di = ax;                                     /* 01eb6 */
    di = di + W(0x9c6);                          /* 01eb8 */
    di = di + (-4);                              /* 01ebc add di,-4 */
L1ebf:
    ax = ball_y - W(0x9c6);                      /* 01ebf/01ec2 [0x98e]-[0x9c6] */
    loc4 = ax;                                   /* 01ec6 [bp-4] */

    ax = W(0x988);                               /* 01ec9 */
    if (ax >= 0) goto L1ed2;                      /* 01ecc or / 01ece jge */
    ax = -ax;                                    /* 01ed0 neg */
L1ed2:
    if (ax >= 0x80) goto L1f31;                  /* 01ed2/01ed5 jge */
    if (W(0xa4d) >= 0x4b) goto L1f31;            /* 01ed7/01edc jge */

    /* dl = (0x98a >= 0x9e) ? 0 : 1 */
    if (W(0x98a) >= 0x9e) goto L1eeb;            /* 01ede/01ee4 jge */
    ax = 1;                                      /* 01ee6 */
    goto L1eed;                                  /* 01ee9 */
L1eeb:
    ax = 0;                                      /* 01eeb xor ax,ax */
L1eed:
    dx = ax;                                     /* 01eed push ax (saved in dx below) */
    /* ah/al = (0x988 >= 0) ? 0 : 1 */
    if (W(0x988) >= 0) goto L1efa;               /* 01eee/01ef3 jge */
    ax = 1;                                      /* 01ef5 */
    goto L1efc;                                  /* 01ef8 */
L1efa:
    ax = 0;                                      /* 01efa xor ax,ax */
L1efc:
    /* pop dx (the earlier value) then xor dx,ax */
    dx = dx ^ ax;                                /* 01efc/01efd */
    if (dx == 0) goto L1f19;                     /* 01eff je */
    collide_check(1, W(0x9c6) + 0xf, 3);         /* 01f01..01f13 (0xf) */
    goto L2002;                                  /* 01f16 */
L1f19:
    collide_check(1, W(0x9c6) + (short)0xfff1, 3);/* 01f19..01f2b (0xfff1 == -15) */
    goto L2002;                                  /* 01f2e */

L1f31:
    if (W(0x98a) > 0x82) goto L1f3c;             /* 01f31/01f37 jg */
    goto L1fc0;                                  /* 01f39 */
L1f3c:
    ax = loc4;                                   /* 01f3c [bp-4] */
    if (ax >= 0) goto L1f45;                      /* 01f3f or / 01f41 jge */
    ax = -ax;                                    /* 01f43 neg */
L1f45:
    if (ax <= 6) goto L1f7a;                      /* 01f45/01f48 jle */
    ax = W(0x988);                               /* 01f4a */
    if (ax >= 0) goto L1f53;                      /* 01f4d or / 01f4f jge */
    ax = -ax;                                    /* 01f51 neg */
L1f53:
    if (ax >= 0x400) goto L1f7a;                 /* 01f53/01f56 jge */
    /* collide_check(1, ((ball_y + 0xff26) >> 3) + [0x9c6], 3) */
    ax = ball_y + (short)0xff26;                 /* 01f5c/01f5f (0xff26 == -218) */
    ax = ax >> 1; ax = ax >> 1; ax = ax >> 1;    /* 01f62/01f64/01f66 sar (arith) */
    ax = ax + W(0x9c6);                          /* 01f68 */
    collide_check(1, ax, 3);                     /* 01f58..01f74 */
    goto L2002;                                  /* 01f77 */
L1f7a:
    /* collide_check(1, [0x9c6] - [bp-2] - ((ball_y+0xff26)>>3), 0xa) */
    ax = ball_y + (short)0xff26;                 /* 01f7e/01f81 */
    ax = ax >> 1; ax = ax >> 1; ax = ax >> 1;    /* 01f84/01f86/01f88 sar */
    dx = W(0x9c6);                               /* 01f8a */
    dx = dx - loc2;                              /* 01f8e sub dx,[bp-2] */
    dx = dx - ax;                                /* 01f91 sub dx,ax */
    collide_check(1, dx, 0xa);                   /* 01f7a..01f9b */

    /* ax = (ball_y > 0xaf && !(server==1 && [0x9dc]>=2)) ? 1 : 0 */
    if (ball_y <= 0xaf) goto L1fb9;              /* 01f9e/01fa4 jle */
    if (server != 1) goto L1fb4;                 /* 01fa6/01fab jne */
    if (W(0x9dc) >= 2) goto L1fb9;               /* 01fad/01fb2 jge */
L1fb4:
    ax = 1;                                      /* 01fb4 */
    goto L1fbb;                                  /* 01fb7 */
L1fb9:
    ax = 0;                                      /* 01fb9 xor ax,ax */
L1fbb:
    W(0x9d2) = ax;                               /* 01fbb */
    goto L2002;                                  /* 01fbe */

L1fc0:
    if (di >= 0x9e) goto L1fcd;                  /* 01fc0/01fc4 jge */
    di = 0x13c - di;                             /* 01fc6/01fc9/01fcb */
L1fcd:
    if (di <= 0x115) goto L1fda;                 /* 01fcd/01fd1 jle */
    di = 0x22a - di;                             /* 01fd3/01fd6/01fd8 */
L1fda:
    collide_check(1, di - loc2, 3);              /* 01fda..01feb */
    goto L2002;                                  /* 01fee */

L1ff0:
    collide_check(1, 0xd3, 8);                   /* 01ff0..01fff */
    /* fall through to L2002 */
L2002:
    return 0;                                    /* 02002..02007 epilogue; AX incidental */
}
