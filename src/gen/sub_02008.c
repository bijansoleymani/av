/* 0x02008 sub_02008 - play one match: reset per-side state, install the
 *                     keyboard ISR, then run the input/physics/render loop until
 *                     the round terminates, restoring the vector on the way out. */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* Interrupt-vector helpers in the Turbo C RTL region (not part of the dumped
 * game code):
 *   sub_0413c == getvect(int)      -> old handler far ptr in DX:AX
 *   sub_0414c == setvect(int,ptr)  -> install handler
 * The getvect(9)/setvect(9, our_isr) pair at 0x21b9/0x21d0 installs the game's
 * INT 9 keyboard ISR (located at cs:0xc5d); the setvect(9, old) at 0x22a7
 * restores it.  Per the porting contract these are replaced with kb_install()
 * (SDL feeds key state) and the restore becomes a no-op. */
extern void kb_install(void);

/* RTL polling/blocking key helpers (same as elsewhere in the game). */
extern int sub_0432b(void);   /* kbhit-style poll: AX nonzero when key waiting */
extern int sub_04104(void);   /* getch-style blocking read (result discarded)  */

int sub_02008(void)
{
    int si;                 /* register SI */
    int di;                 /* register DI */
    int ax, dx, bx;         /* scratch */
    int ctl1;               /* [bp-4]: player-1 control kind (0x2010 result)   */
    int ctl2;               /* [bp-2]: player-2 control kind                   */

    /* 0x2010..0x202b: zero a batch of globals, seed two constants. */
    W(0x9b6) = 0;                    /* 02012 */
    W(0x988) = 0;                    /* 02015 */
    W(0xa0e) = 0;                    /* 02018 */
    W(0xa3c) = 0;                    /* 0201b */
    W(0x9dc) = 0;                    /* 0201e */
    W(0x986) = 6;                    /* 02021 */
    W(0xa4d) = 0xc8;                 /* 02027 */

    /* 0x202d..0x20f1: per-side reset loop, si = 0,1 */
    si = 0;                          /* 0202d: mov si,ax(=0) */
    goto L20ec;

L2032:
    /* 02032..0203e: W(0x990 + si*2) = W(0x98c + si*2) */
    ax = W(0x98c + si * 2);          /* 02036 */
    W(0x990 + si * 2) = ax;          /* 0203e */
    /* 02042..02054 */
    W(0x9e4 + si * 2) = 0xad;        /* 02046 */
    W(0x9e8 + si * 2) = 0xffff;      /* 02050: -1 */
    /* 02056..0205c: frame index = 0 */
    ax = 0;                          /* 02056: xor ax,ax */
    W(0x9be + si * 2) = ax;          /* 0205c */
    /* 02060..0206b: W(0x9cc + si*6) = ax(=0) */
    W(0x9cc + si * 6) = ax;          /* 0206b */
    /* 0206f..0207a: W(0x9c8 + si*6) = ax(=0) */
    W(0x9c8 + si * 6) = ax;          /* 0207a */
    /* 0207e..02099: W(0x9ca + si*6), W(0xa10 + si*2), W(0x9de + si*2) = ax(=0) */
    W(0x9ca + si * 6) = ax;          /* 02089 */
    W(0xa10 + si * 2) = ax;          /* 02091 */
    W(0x9de + si * 2) = ax;          /* 02099 */

    /* 0209d..020be:
     *   dx = W((si+1)*0x3c + 0xaa)
     *   ax = W((si+1)*0x3c + 0xac) - 1
     *   if (dx >= ax) goto L20eb                                   (signed jge) */
    dx = W((si + 1) * 0x3c + 0xaa);  /* 020a7 */
    ax = W((si + 1) * 0x3c + 0xac);  /* 020b6 */
    ax = ax - 1;                     /* 020ba */
    if (dx >= ax)                    /* 020be: jge */
        goto L20eb;

    /* 020c0..020e5:
     *   dx = W((2-si)*0x3c + 0xaa)
     *   ax = W((2-si)*0x3c + 0xac) - 1
     *   if (dx != ax) goto L20eb                                   (jne) */
    dx = W((2 - si) * 0x3c + 0xaa);  /* 020cc */
    ax = W((2 - si) * 0x3c + 0xac);  /* 020dd */
    ax = ax - 1;                     /* 020e1 */
    if (dx != ax)                    /* 020e5: jne */
        goto L20eb;

    side_swap = si;                  /* 020e7: [0x9d8] = si */

L20eb:
    si++;                            /* 020eb */
L20ec:
    if (si >= 2)                     /* 020ec/020ef: cmp si,2 ; jge */
        goto L20f4;
    goto L2032;                      /* 020f1 */

L20f4:
    /* 020f4..020fd: joy_timeout = joy_val + (joy_val >> 1)   (sar = signed) */
    ax = joy_val >> 1;               /* 020f7: sar ax,1 */
    ax = ax + joy_val;               /* 020f9 */
    joy_timeout = ax;                /* 020fd: [0xa42] */

    /* 02100..0212d: side-dependent layout constants.
     *   ax = side_swap*0xa5 + 0x40 */
    ax = side_swap * 0xa5 + 0x40;    /* 02100..02108 */
    W(0xa3a) = ax;                   /* 0210b */
    W(0x9c6) = ax;                   /* 0210e */
    W(0xa40) = (unsigned short)((unsigned)ax << 6);  /* 02111..02116: shl ax,6 */

    ax = 0x87;                       /* 02119 */
    W(0x9c4) = ax;                   /* 0211c */
    W(0x98a) = ax;                   /* 0211f */
    W(0x9ec) = (unsigned short)((unsigned)ax << 6);  /* 02122..02124: shl ax,6 */

    W(0xa14) = side_swap + 2;        /* 02127..0212d */

    /* 02130..0213f */
    server = 2;                      /* 02130: [0x9d6] = 2 */
    W(0x9bc) = 1;                    /* 02136..02139 */
    W(0xa4b) = 1;                    /* 0213c */
    W(0x9d4) = 0;                    /* 0213f */

    /* 02145..0214a: ctl1 = ctl2 = 0 */
    ctl2 = 0;                        /* 02147: [bp-2] = 0 */
    ctl1 = 0;                        /* 0214a: [bp-4] = 0 */

    /* 0214d..0217a: player-1 control kind from menu struct.
     *   al = B(W(0xe6)*0xe + 0xee) ; switch(al) */
    ax = B(W(0xe6) * 0xe + 0xee) & 0xff;   /* 0214d..0215b (ah=0) */
    if (ax == 0x20)                  /* 0215d: je 0x2175 */
        goto L2175;
    if (ax == 0x43)                  /* 02162: je 0x217c */
        goto L217c;
    if (ax == 0x4a)                  /* 02167: je 0x216e */
        goto L216e;
    goto L2181;                      /* 0216c */
L216e:
    ctl1 = 1;                        /* 0216e */
    goto L2181;
L2175:
    ctl1 = 2;                        /* 02175 */
    goto L2181;
L217c:
    ctl1 = 3;                        /* 0217c */
L2181:
    /* 02181..021ae: player-2 control kind from menu struct.
     *   al = B(W(0x122)*0xe + 0x12a) ; switch(al) */
    ax = B(W(0x122) * 0xe + 0x12a) & 0xff; /* 02181..0218f (ah=0) */
    if (ax == 0x20)                  /* 02191: je 0x21a9 */
        goto L21a9;
    if (ax == 0x43)                  /* 02196: je 0x21b0 */
        goto L21b0;
    if (ax == 0x4a)                  /* 0219b: je 0x21a2 */
        goto L21a2;
    goto L21b5;                      /* 021a0 */
L21a2:
    ctl2 = 1;                        /* 021a2 */
    goto L21b5;
L21a9:
    ctl2 = 2;                        /* 021a9 */
    goto L21b5;
L21b0:
    ctl2 = 3;                        /* 021b0 */
L21b5:
    /* 021b5..021c2: getvect(9) -> save old INT 9 vector in [0xa49]:[0xa47].
     * 021c5..021d3: setvect(9, cs:0xc5d) -> install the game's keyboard ISR.
     * Replaced with kb_install() (SDL delivers key state). */
    kb_install();

    /* 021d6: setup_round() */
    setup_round();                   /* 021d6 */

    si = 0;                          /* 021d9: xor si,si */

L21db:
    /* 021db..021e2: if (W(0x9d4) != 0) goto L2298 (post-round cleanup) */
    if (W(0x9d4) != 0)               /* 021db: cmp [0x9d4],0 ; jne */
        goto L2298;

    /* 021e5..021ee: hit_count = hit_count*5 + 1 */
    hit_count = hit_count * 5 + 1;   /* 021e5..021ee */

    /* 021f1..02224: player-1 input dispatch on ctl1.
     *   ax = ctl1 - 1 ; if ((unsigned)ax > 2) skip ; switch(ax) */
    ax = ctl1 - 1;                   /* 021f4 */
    if ((unsigned)ax > 2u)           /* 021f8: ja 0x2224 */
        goto L2224;
    switch (ax) {                    /* 021fe: jmp cs:[bx*2 + 0x2203] */
    case 0: goto L2209;              /* joystick */
    case 1: goto L2213;              /* mouse    */
    case 2: goto L2221;              /* keyboard */
    }
L2209:
    read_joystick(0);                /* 0220b: read_joystick(0) */
    goto L2224;                      /* 02211 */
L2213:
    if (si == 0)                     /* 02213: or si,si ; je */
        goto L2224;
    read_mouse(0);                   /* 0221a: read_mouse(0) */
    goto L2224;                      /* 0221f */
L2221:
    sub_019f4();                     /* 02221 */
L2224:
    /* 02224..02259: player-2 input dispatch on ctl2. */
    ax = ctl2 - 1;                   /* 02227 */
    if ((unsigned)ax > 2u)           /* 0222b: ja 0x2259 */
        goto L2259;
    switch (ax) {                    /* 02231: jmp cs:[bx*2 + 0x2236] */
    case 0: goto L223c;              /* joystick */
    case 1: goto L2247;              /* mouse    */
    case 2: goto L2256;              /* keyboard */
    }
L223c:
    read_joystick(1);                /* 02240: read_joystick(1) */
    goto L2259;                      /* 02245 */
L2247:
    if (si == 0)                     /* 02247: or si,si ; je */
        goto L2259;
    read_mouse(1);                   /* 0224f: read_mouse(1) */
    goto L2259;                      /* 02254 */
L2256:
    sub_01cea();                     /* 02256 */
L2259:
    /* 02259..0225e: si = 1 - si */
    si = 1 - si;                     /* 02259..0225e */

    sub_00e7a();                     /* 02260 */

    /* 02263..02284: physics/render branch depending on W(0xa4b). */
    if (W(0xa4b) != 0) {             /* 02263: cmp [0xa4b],0 ; je 0x2275 */
        sub_01121();                 /* 0226a */
        sub_01452();                 /* 0226d */
        di = 1;                      /* 02270 */
        goto L2284;                  /* 02273 */
    }
    /* 02275..02281 */
    if (di == 0)                     /* 02275: or di,di ; je 0x2284 */
        goto L2284;
    sub_01121();                     /* 02279 */
    di = sub_01006();                /* 0227c..0227f: di = AX */
    sub_01452();                     /* 02281 */

L2284:
    /* 02284..02295: loop-back / round-end test.
     *   if (di != 0 && W(0x9dc) <= 2) loop; else sub_01747() and loop. */
    if (di == 0)                     /* 02284: or di,di ; je 0x2292 */
        goto L2292;
    if (W(0x9dc) > 2)                /* 02288: cmp [0x9dc],2 ; jg 0x2292 */
        goto L2292;
    goto L21db;                      /* 0228f */
L2292:
    sub_01747();                     /* 02292 */
    goto L21db;                      /* 02295 */

L2298:
    /* 02298..022b7: post-round cleanup.
     *   sub_0166f();
     *   setvect(9, old_vector)  -> restore INT 9 (no-op under SDL);
     *   while (kbhit()) getch();  (drain the keyboard buffer) */
    sub_0166f();                     /* 02298 */
    /* 0229b..022aa: setvect(9, [0xa49]:[0xa47]) restore -> no-op */
L22ad:
    if (sub_0432b() == 0)            /* 022ad..022b2: kbhit ; je 0x22b9 */
        goto L22b9;
    sub_04104();                     /* 022b4: getch (discard) */
    goto L22ad;                      /* 022b7 */

L22b9:
    /* 022b9..022be: epilogue; nothing meaningful in AX. */
    return 0;
}
