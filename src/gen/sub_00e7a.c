/* 0x00e7a sub_00e7a - per-player: clamp ball-x toward target, run spike/jump animation state machine */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_00e7a(void)
{
    int si;            /* loop index 0..1 (player)          */
    int di;            /* [bp-4] target, then working value  */
    int bp_m4;         /* local [bp-4]                       */
    int bp_m2;         /* local [bp-2]                       */
    int ax;
    int bx;

    si = 0;                                     /* 00e82: xor si,si */
    goto Lff8;                                  /* 00e84: jmp 0xff8 */

Le87:
    bx = si * 6;                                /* 00e87..00e8e: ax=si*6 -> bx */
    if (W(0x9cc + bx) == 0)                     /* 00e90: cmp [bx+0x9cc],0 */
        goto Leac;                              /* 00e95: je 0xeac */
    bx = si << 1;                               /* 00e97: shl bx,1 */
    if (W(0x9e8 + bx) != -1)                    /* 00e9b: cmp [bx+0x9e8],-1 ; 00ea0: jne 0xeac */
        goto Leac;
    bx = si << 1;                               /* 00ea2: shl bx,1 */
    W(0x9e8 + bx) = 0;                          /* 00ea6: mov [bx+0x9e8],0 */

Leac:
    bx = si * 6;                                /* 00eac..00eb3: ax=si*6 -> bx */
    di = W(0x9c8 + bx);                         /* 00eb5: di = [bx+0x9c8] */
    bx = si * 6;                                /* 00eb9..00ec0: ax=si*6 -> bx */
    di += W(0x9ca + bx);                        /* 00ec2: di += [bx+0x9ca] */
    bp_m4 = di;                                 /* 00ec6: [bp-4] = di */
    bx = si << 1;                               /* 00ec9: shl bx,1 */
    di += W(0x98c + bx);                        /* 00ecd: di += [bx+0x98c] (ball_x[si]) */

    ax = si * 0x9b;                             /* 00ed1..00ed6: ax = si*0x9b */
    ax += 3;                                    /* 00ed8: add ax,3 */
    bp_m2 = ax;                                 /* 00edb: [bp-2] = ax */

    if (bp_m4 <= 0)                             /* 00ede: cmp [bp-4],0 ; 00ee2: jle 0xefb */
        goto Lefb;
    ax += 0x77;                                 /* 00ee4: add ax,0x77 */
    if (ax > di)                                /* 00ee7: cmp ax,di ; 00ee9: jg 0xf00 (signed) */
        goto Lf00;
    ax = bp_m2;                                 /* 00eeb: ax = [bp-2] */
    ax += 0x77;                                 /* 00eee: add ax,0x77 */
    bx = si << 1;                               /* 00ef1: shl bx,1 */
    W(0x98c + bx) = ax;                         /* 00ef5: [bx+0x98c] = ax */
    goto Lf15;                                  /* 00ef9: jmp 0xf15 */

Lefb:
    if (di <= bp_m2)                            /* 00efb: cmp di,[bp-2] ; 00efe: jle 0xf0a (signed) */
        goto Lf0a;
Lf00:
    bx = si << 1;                               /* 00f00: shl bx,1 */
    W(0x98c + bx) = di;                         /* 00f04: [bx+0x98c] = di */
    goto Lf15;                                  /* 00f08: jmp 0xf15 */

Lf0a:
    ax = bp_m2;                                 /* 00f0a: ax = [bp-2] */
    bx = si << 1;                               /* 00f0d: shl bx,1 */
    W(0x98c + bx) = ax;                         /* 00f11: [bx+0x98c] = ax */

Lf15:
    bx = si << 1;                               /* 00f15: shl bx,1 */
    if (W(0x9e8 + bx) != -2)                    /* 00f19: cmp [bx+0x9e8],-2 ; 00f1e: jne 0xf3e */
        goto Lf3e;
    bx = si << 1;                               /* 00f20: shl bx,1 */
    W(0x9e4 + bx) = 0xad;                       /* 00f24: [bx+0x9e4] = 0xad (p_y[si]) */
    bx = si << 1;                               /* 00f2a: shl bx,1 */
    W(0x9be + bx) = 0;                          /* 00f2e: [bx+0x9be] = 0 (p_frame[si]) */
    bx = si << 1;                               /* 00f34: shl bx,1 */
    W(0x9e8 + bx) = (short)0xffff;              /* 00f38: [bx+0x9e8] = 0xffff (-1) */

Lf3e:
    bx = si << 1;                               /* 00f3e: shl bx,1 */
    if (W(0x9e8 + bx) != -1)                    /* 00f42: cmp [bx+0x9e8],-1 ; 00f47: jne 0xf95 */
        goto Lf95;
    if (bp_m4 == 0)                             /* 00f49: cmp [bp-4],0 ; 00f4d: je 0xf89 */
        goto Lf89;
    bx = si << 1;                               /* 00f4f: shl bx,1 */
    ax = W(0x990 + bx);                         /* 00f53: ax = [bx+0x990] */
    bx = si << 1;                               /* 00f57: shl bx,1 */
    ax -= W(0x98c + bx);                        /* 00f5b: ax -= [bx+0x98c] (ball_x[si]) */
    if (ax >= 0)                                /* 00f5f: or ax,ax ; 00f61: jge 0xf65 */
        goto Lf65;
    ax = -ax;                                   /* 00f63: neg ax */
Lf65:
    if (ax > 4)                                 /* 00f65: cmp ax,4 ; 00f68: jg 0xf6d (signed) */
        goto Lf6d;
    goto Lff7;                                  /* 00f6a: jmp 0xff7 */
Lf6d:
    bx = si << 1;                               /* 00f6d: shl bx,1 */
    W(0x9be + bx) ^= 1;                         /* 00f71: xor [bx+0x9be],1 (p_frame[si]) */
    bx = si << 1;                               /* 00f77: shl bx,1 */
    ax = W(0x98c + bx);                         /* 00f7b: ax = [bx+0x98c] (ball_x[si]) */
    bx = si << 1;                               /* 00f7f: shl bx,1 */
    W(0x990 + bx) = ax;                         /* 00f83: [bx+0x990] = ax */
    goto Lff7;                                  /* 00f87: jmp 0xff7 */

Lf89:
    bx = si << 1;                               /* 00f89: shl bx,1 */
    W(0x9be + bx) = 0;                          /* 00f8d: [bx+0x9be] = 0 (p_frame[si]) */
    goto Lff7;                                  /* 00f93: jmp 0xff7 */

Lf95:
    bx = si << 1;                               /* 00f95: shl bx,1 */
    if (W(0x9e8 + bx) <= 0x12)                  /* 00f99: cmp [bx+0x9e8],0x12 ; 00f9e: jle 0xfa5 (signed) */
        goto Lfa5;
    ax = 1;                                     /* 00fa0: mov ax,1 */
    goto Lfa7;                                  /* 00fa3: jmp 0xfa7 */
Lfa5:
    ax = 0;                                     /* 00fa5: xor ax,ax */
Lfa7:
    ax += 2;                                    /* 00fa7: add ax,2 */
    bx = si << 1;                               /* 00faa: shl bx,1 */
    W(0x9be + bx) = ax;                         /* 00fae: [bx+0x9be] = ax (p_frame[si]) */

    bx = si << 1;                               /* 00fb2: shl bx,1 */
    if (W(0x9e8 + bx) != 0x13)                  /* 00fb6: cmp [bx+0x9e8],0x13 ; 00fbb: jne 0xfc6 */
        goto Lfc6;
    bx = si << 1;                               /* 00fbd: shl bx,1 */
    W(0x9e4 + bx) -= 4;                         /* 00fc1: sub [bx+0x9e4],4 (p_y[si]) */

Lfc6:
    bx = si << 1;                               /* 00fc6: shl bx,1 */
    ax = W(0x9e8 + bx);                         /* 00fca: ax = [bx+0x9e8] */
    W(0x9e8 + bx) += 1;                         /* 00fce: inc [bx+0x9e8] */
    bx = ax << 1;                               /* 00fd2: bx = ax<<1 */
    ax = W(0x264 + bx);                         /* 00fd6: ax = [bx+0x264] (jump-arc table) */
    bx = si << 1;                               /* 00fda: shl bx,1 */
    W(0x9e4 + bx) += ax;                        /* 00fde: [bx+0x9e4] += ax (p_y[si]) */

    bx = si << 1;                               /* 00fe2: shl bx,1 */
    if (W(0x9e8 + bx) <= 0x25)                  /* 00fe6: cmp [bx+0x9e8],0x25 ; 00feb: jle 0xff7 (signed) */
        goto Lff7;
    bx = si << 1;                               /* 00fed: shl bx,1 */
    W(0x9e8 + bx) = (short)0xfffe;              /* 00ff1: [bx+0x9e8] = 0xfffe (-2) */

Lff7:
    si++;                                       /* 00ff7: inc si */
Lff8:
    if (si >= 2)                                /* 00ff8: cmp si,2 ; 00ffb: jge 0x1000 (signed) */
        goto L1000;
    goto Le87;                                  /* 00ffd: jmp 0xe87 */

L1000:
    return 0;                                   /* 01000..01005: epilogue + ret (no meaningful AX) */
}
