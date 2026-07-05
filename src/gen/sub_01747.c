/* 0x01747 sub_01747 - end-of-round: settle the ball with a damped physics loop,
 *                     award/redraw the score for the winning side, then reset
 *                     all round state for the next serve. */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* RTL / library helpers reached by this function but living outside the
 * decompiled game range.  By usage:
 *   sub_04b57(void)              : sound/delay teardown (paired with sub_04b2b)
 *   sub_04b2b(int ms)            : delay(ms) busy-wait
 *   sub_04439(val,buf,radix)->buf: itoa-style number->string, returns buf ptr
 *   sub_03f7a(int ms)            : delay(ms) busy-wait (second timer)
 * They are called by their sub name / mapped shim to stay faithful to the bytes.*/
extern void  sub_04b57(void);
extern void  sub_04b2b(int ms);
extern dsptr sub_04439(int value, dsptr buf, int radix);
extern void  sub_03f7a(int ms);

/* far-call RTL used here:
 *   0x6a19 bar        -> bgi_bar(x1,y1,x2,y2)
 *   0x6c42 putimage   -> bgi_outtextxy(x,y,str)  (far string ptr)
 *   0x6b04 lib_6b04   -> bgi_settextstyle_dir(dir)
 *   0x6129 lib_6129   -> bgi_outtextxy(x,y,str)  (near string ptr; text draw) */

/* The original keeps a small itoa scratch buffer on the stack at SS:[bp-4].
 * Our model only exposes DS[], and sub_04439/bgi_outtextxy take a dsptr, so we
 * reserve a private scratch at the very top of DS (above DGROUP/heap). */
#define S1747_BUF 0xfff8u

int sub_01747(void)
{
    int si;                 /* winning side index (0 or 1)              */
    int di;                 /* |[0x988]| >> 3   (ball x-velocity cap)   */
    int ax;
    int loc6;               /* [bp-6] = |[0x9b6]| >> 3  (y-velocity cap)*/
    int loc8;               /* [bp-8] = settle-loop countdown (0x14)    */
    int loca;               /* [bp-0xa] = si*0xe6 + 0x28                 */
    int bx;
    dsptr buf = S1747_BUF;  /* [bp-4] itoa buffer                       */

    /* ---- decide winning side (si) ---- */
    if (W(0x9dc) <= 2) {                    /* 01747: cmp [0x9dc],2 ; jle 0x175f */
        if (W(0x9c6) >= 0x96) {             /* 0175f: cmp [0x9c6],0x96 ; jge 0x176c */
            si = 0;                         /* 0176c: xor si,si */
        } else {
            si = 1;                         /* 01767: mov si,1 */
        }
    } else {
        si = 1 - server;                    /* 01756: mov si,1 ; sub si,[0x9d6] */
    }

    sub_01452();                            /* 0176e: call 0x1452 */

    /* ---- loc6 = |[0x9b6]| >> 3 (signed shift) ---- */
    ax = W(0x9b6);                          /* 01771 */
    if (ax < 0) ax = -ax;                   /* 01774: or/jge/neg */
    ax = ax >> 1; ax = ax >> 1; ax = ax >> 1; /* 0177a-0177e: sar x3 */
    loc6 = ax;                              /* 01780: [bp-6] = ax */

    /* ---- di = |[0x988]| >> 3 ---- */
    ax = W(0x988);                          /* 01783 */
    if (ax < 0) ax = -ax;                   /* 01786: or/jge/neg */
    di = ax;                                /* 0178c: mov di,ax */
    di = di >> 1; di = di >> 1; di = di >> 1; /* 0178e-01792: sar x3 */

    loc8 = 0x14;                            /* 01794: [bp-8] = 0x14 */

    ax = 0;                                 /* 01799: xor ax,ax */
    W(0x9d2) = ax;                          /* 0179b */
    W(0x9cc) = ax;                          /* 0179e */

L1799:
    sub_04b57();                            /* 017a1: call 0x4b57 */
    sub_00e7a();                            /* 017a4: call 0xe7a */

    /* ---- clamp [0x988] magnitude to di ---- */
    ax = W(0x988);                          /* 017a7 */
    if (ax < 0) ax = -ax;                   /* 017aa: or/jge/neg */
    if (ax > di) {                          /* 017b0: cmp ax,di ; jle 0x17c8 */
        if (W(0x988) >= 0) {                /* 017b4: cmp [0x988],0 ; jge 0x17c4 */
            W(0x988) = di;                  /* 017c4 */
        } else {
            ax = di;                        /* 017bb: mov ax,di */
            ax = -ax;                       /* 017bd: neg ax */
            W(0x988) = ax;                  /* 017bf */
        }
    }

    /* ---- clamp [0x9b6] magnitude to loc6 ---- */
    ax = W(0x9b6);                          /* 017c8 */
    if (ax < 0) ax = -ax;                   /* 017cb: or/jge/neg */
    if (ax > loc6) {                        /* 017d1: cmp ax,[bp-6] ; jle 0x17ed */
        if (W(0x9b6) >= 0) {                /* 017d6: cmp [0x9b6],0 ; jge 0x17e7 */
            W(0x9b6) = loc6;                /* 017e7-017ea */
        } else {
            ax = loc6;                      /* 017dd: mov ax,[bp-6] */
            ax = -ax;                       /* 017e0: neg ax */
            W(0x9b6) = ax;                  /* 017e2 */
        }
    }

    sub_01121();                            /* 017ed: call 0x1121 */

    if (quit_flag != 0) {                   /* 017f0: cmp [0x24e],0 ; je 0x1811 */
        if (si == W(0xa14)) {               /* 017f7: cmp si,[0xa14] ; jne 0x1808 */
            sub_04b2b(0x1388);              /* 017fd-01801 */
        } else {
            sub_04b2b(0x64);                /* 01808-0180c */
        }
    }
    /* 01811: */
    sub_01006();                            /* 01811: call 0x1006 */
    sub_01452();                            /* 01814: call 0x1452 */

    ax = loc8;                              /* 01817: mov ax,[bp-8] */
    loc8 = loc8 - 1;                        /* 0181a: dec [bp-8] */
    if (ax > 0) goto L1799;                 /* 0181d: or ax,ax ; jle 0x1824 ; jmp 0x1799 */

    /* 01824: countdown spent - keep looping until the ball is at rest */
    if (W(0x9e8) != -1) goto L1799;         /* 01824: cmp [0x9e8],-1 ; je 0x182e ; jmp 0x1799 */
    if (W(0x9ea) != -1) goto L1799;         /* 0182e: cmp [0x9ea],-1 ; je 0x1838 ; jmp 0x1799 */

    /* ================= 0x1838: award point / update display ================= */
    sub_04b57();                            /* 01838 */

    /* loca = si*0xe6 + 0x28 (unsigned multiply, low 16 bits) */
    loca = (int)(unsigned short)((unsigned)si * 0xe6u) + 0x28; /* 0183b-01845 */

    if (si == W(0xa14)) {                    /* 01848: cmp si,[0xa14] ; jne 0x18bf */
        /* clear the score strip then draw the incremented score as text */
        bgi_bar(loca, 0, loca + 0xf, 7);     /* 0184e-0185f: bar(loca,0,loca+0xf,7) */

        bx = si << 1;                        /* 0186f-01871 */
        W(bx + 0xa10) = W(bx + 0xa10) + 1;   /* 01873: inc [bx+0xa10]  (++score[si]) */
        /* buf = itoa(score[si], buf, 10) */
        buf = sub_04439(W(bx + 0xa10), S1747_BUF, 0xa); /* 01877-0187c */
        /* putimage(x=loca, y=0, str=buf) -> outtextxy */
        bgi_outtextxy(loca, 0, buf);         /* 0188c */

        bx = si << 1;                        /* 01894-01896 */
        if (W(bx + 0xa10) > 0xe) {           /* 01898: cmp [bx+0xa10],0xe ; jle 0x190d */
            ax = W((si << 1) + 0xa10);       /* 0189f-018a3 */
            bx = (1 - si) << 1;              /* 018a7-018ac */
            ax = ax - W(bx + 0xa10);         /* 018ae */
            if (ax > 1) {                    /* 018b2: cmp ax,1 ; jle 0x190d */
                W(0x9d4) = 1;                /* 018b7: mov [0x9d4],1 (game over) */
            }
        }
        /* jmp 0x190d */
    } else {
        /* 018bf: winning side changed - relabel the two score headers */
        bgi_settextstyle_dir(0);             /* 018bf-018c2: lib_6b04(0) */
        /* lib_6129( (1-si)*0xe6 + 0x23, 3, 1 ) -> outtextxy(x, y=3, str=1) */
        bgi_outtextxy((int)(unsigned short)((unsigned)(1 - si) * 0xe6u) + 0x23,
                      3, 1);                  /* 018c9-018df */
        bgi_settextstyle_dir(0xf);            /* 018e7-018eb: lib_6b04(0xf) */
        /* lib_6129( loca + 0xfffb, 3, 1 ) ; 0xfffb == -5 */
        bgi_outtextxy((int)(short)(loca + 0xfffb), 3, 1); /* 018f2-01901 */
        W(0xa14) = si;                        /* 01909 */
        /* fall through to 0x190d */
    }

    /* 0190d: */
    sub_03f7a(0x64);                          /* 0190d-01911: call 0x3f7a */
    sub_0166f();                              /* 01916: call 0x166f */

    /* ================= reset all round state for next serve ================= */
    ax = (int)(unsigned short)((unsigned)si * 0xa5u) + 0x40; /* 01919-01920 */
    W(0xa3a) = ax;                            /* 01923 */
    W(0x9c6) = ax;                            /* 01926 */
    W(0xa40) = (int)(unsigned short)((unsigned)ax << 6); /* 01929-0192e: shl ax,cl(=6) */

    ax = 0x87;                                /* 01931 */
    W(0x9c4) = ax;                            /* 01934 */
    W(0x98a) = ax;                            /* 01937 */
    W(0x9ec) = (int)(unsigned short)((unsigned)ax << 6); /* 0193a: shl ax,cl(=6) */

    ax = 0;                                   /* 0193f */
    W(0x9e0) = ax;                            /* 01941 */
    W(0x9de) = ax;                            /* 01944 */
    W(0x9dc) = ax;                            /* 01947 */
    W(0x9b6) = ax;                            /* 0194a */
    W(0x988) = ax;                            /* 0194d */
    W(0xa0e) = ax;                            /* 01950 */
    W(0xa3c) = ax;                            /* 01953 */
    W(0x986) = 6;                             /* 01956: mov [0x986],cx (cx==6) */

    ax = 1;                                   /* 0195a */
    W(0x9bc) = ax;                            /* 0195d */
    W(0xa4b) = ax;                            /* 01960 */
    W(0x9d6) = 2;                             /* 01963: server = 2 */

    /* [0xa3e] = |hit_count| % 5  (signed idiv remainder) */
    ax = hit_count;                           /* 01969: mov ax,[0x9e2] */
    if (ax < 0) ax = -ax;                     /* 0196c: or/jge/neg */
    W(0xa3e) = ax % 5;                        /* 01972-01978: cdq; idiv 5; store dx */

    bx = W(0xa14) << 1;                       /* 0197c-01980 */
    if (W(bx + 0xa10) == 0xe) {               /* 01982: cmp [bx+0xa10],0xe ; jne 0x198f */
        W(0xa3e) = 5;                         /* 01989 */
    }
    W(0x9b8) = 0;                             /* 0198f */

    return 0;                                 /* 0199a: ret (no meaningful AX) */
}
