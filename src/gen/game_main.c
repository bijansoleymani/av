/* 0x001a5 game_main - top-level: init graphics, load data, then run match rounds */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* RTL-region routine at 0x3c21: gates joystick calibration (bit0 of AX). */
extern int sub_03c21(void);

int game_main(void)
{
	int gd;         /* [bp-4] graphics driver */
	int gm;         /* [bp-2] graphics mode   */
	int di, si;     /* loop indices           */
	unsigned bx;    /* struct index           */
	int al;         /* byte scratch           */

	int rc;         /* AX return scratch      */

	gd = 1;                                  /* 001ad */
	gm = 3;                                  /* 001b2 */

	/* registerbgidriver(0x2470): shim always succeeds (returns >=0). */
	registerbgidriver();                     /* 001bb (far ptr arg 0x2470 ignored) */
	rc = 0;                                  /* AX from the call */
	if (rc < 0)                              /* 001c0 or ax,ax ; 001c2 jge 0x1cd */
		dos_exit(1);                     /* 001c8 exit(1) */

	/* initgraph(&gd, &gm, "") -> cga_init() */
	(void)gd; (void)gm;                      /* driver/mode consumed by initgraph */
	cga_init();                              /* 001dc */

	if (graphresult() < 0) {                /* 001e4/001e9 or ax,ax ; 001eb jge */
		dos_printf(0x2b3);              /* 001f1 printf("CGA mode not available.") */
		goto L2fa;                      /* 001f6 jmp 0x2fa (no closegraph) */
	}

	if (load_data() == 0)                   /* 001f9/001fc or ax,ax ; 001fe jne */
		goto L2f5;                      /* 00200 jmp 0x2f5 */

	sound_on = 0;                          /* 00205 [0x24e]=0 */
	side_swap = 0;                          /* 00208 [0x9d8]=0 */

	if (sub_300() != 0)                     /* 0020b/0020e or ax,ax ; 00210 jne */
		goto L254;                      /* skip loop 1 */

	/* ---- loop 1: di=1..2, si=0..13 ---- */
	di = 1;                                 /* 00212 */
	goto L24f;                              /* 00215 */
L217:
	si = 0;                                 /* 00217 xor si,si */
	goto L23c;                              /* 00219 */
L21b:
	bx = (unsigned)(di * 0x3c) + (unsigned)si;      /* 0021b-00224 */
	al = B(bx + 0xd8) & 0xff;                        /* 00226 mov al,[bx+0xd8] */
	bx = (unsigned)(di * 0x3c) + (unsigned)si;      /* 0022b-00234 */
	B(bx + 0xca) = (unsigned char)al;               /* 00237 mov [bx+0xca],al */
	si++;                                            /* 0023b */
L23c:
	if (si < 0xe)                                    /* 0023c cmp si,0xe ; 0023f jl */
		goto L21b;
	bx = (unsigned)(di * 0x3c);                     /* 00241-00248 */
	W(bx + 0xac)--;                                 /* 0024a dec word [bx+0xac] */
	di++;                                            /* 0024e */
L24f:
	if (di < 3)                                     /* 0024f cmp di,3 ; 00252 jl */
		goto L217;

L254:
	if (sub_03c21() & 1) {                          /* 00254/00257 test ax,1 ; 0025a je */
		if (joy_calibrate() != 0)               /* 0025c/0025f or ax,ax ; 00261 jne */
			goto L2c5;                      /* skip loop 2 */
	}

	/* ---- loop 2: di=1..2, si=0..13 ---- */
	di = 1;                                         /* 00263 */
	goto L2c0;                                      /* 00266 */
L268:
	si = 0;                                         /* 00268 xor si,si */
	goto L2ad;                                      /* 0026a */
L26c:
	bx = (unsigned)(di * 0x3c) + (unsigned)si;      /* 0026c-00273 */
	al = B(bx + 0xca) & 0xff;                        /* 00277 mov al,[bx+0xca] */
	bx = (unsigned)(di * 0x3c) + (unsigned)si;      /* 0027c-00283 */
	B(bx + 0xbc) = (unsigned char)al;               /* 00288 mov [bx+0xbc],al */
	bx = (unsigned)(di * 0x3c) + (unsigned)si;      /* 0028c-00295 */
	al = B(bx + 0xd8) & 0xff;                        /* 00297 mov al,[bx+0xd8] */
	bx = (unsigned)(di * 0x3c) + (unsigned)si;      /* 0029c-002a5 */
	B(bx + 0xca) = (unsigned char)al;               /* 002a8 mov [bx+0xca],al */
	si++;                                            /* 002ac */
L2ad:
	if (si < 0xe)                                    /* 002ad cmp si,0xe ; 002b0 jl */
		goto L26c;
	bx = (unsigned)(di * 0x3c);                     /* 002b2-002b9 */
	W(bx + 0xac)--;                                 /* 002bb dec word [bx+0xac] */
	di++;                                            /* 002bf */
L2c0:
	if (di < 3)                                     /* 002c0 cmp di,3 ; 002c3 jl */
		goto L268;

L2c5:
	setup_round();                                  /* 002c5 */
	ball_x = 0x40;                                  /* 002c8 [0x98c]=0x40 */
	ball_y = 0xe2;                                  /* 002ce [0x98e]=0xe2 */
	p1_y = 0xad;                                    /* 002d4 [0x9e4]=0xad */
	p2_y = 0xad;                                    /* 002da [0x9e6]=0xad */

L2e0:
	if (play_round() == 0)                          /* 002e0/002e3 or ax,ax ; 002e5 je */
		goto L2f5;
	sub_02008();                                    /* 002e7 */
	side_swap = side_swap ^ 1;                      /* 002ea-002f0 [0x9d8]^=1 */
	goto L2e0;                                      /* 002f3 jmp 0x2e0 */

L2f5:
	cga_close();                                    /* 002f5 lcall 0x5ce3 -> closegraph */
L2fa:
	return 0;                                       /* 002ff ret (AX not meaningful) */
}
