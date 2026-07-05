/* 0x0234e sub_0234e - XOR-blit a sprite into CGA video memory (draw_sprite via XOR) */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* cdecl args after "push si; push di; push bp; mov bp,sp":
 *   a0 = [bp+8]  = x
 *   a1 = [bp+0xa] = y
 *   a2 = [bp+0xc] = sprite dsptr (header: W(a2)=width, W(a2+2)=height, then bytes) */
int sub_0234e(int a0, int a1, int a2)
{
    unsigned ax, bx, dx;         /* register mirrors */
    unsigned cl;                 /* low byte of the y arg (odd/even bank select) */
    unsigned di, si;             /* di = VIDEO offset, si = sprite data pointer */
    unsigned char bl;            /* byte width (bytes per row) */
    int ah;                      /* row-pair loop counter (signed dec/jg) */

    /* --- compute starting VIDEO offset di --- */
    ax = (unsigned short)a1;                             /* 0235b mov ax,[bp+0xa] */
    cl = ax & 0xff;                                      /* 0235e mov cl,al (save y low byte) */
    /* 02360 and al,0xfe clears only bit0 of AL; AH (y high byte) is left intact,
     * so the multiplicand for "mul dx" is ((a1 & 0xff00) | (a1 & 0xfe)). */
    ax = ((unsigned short)a1 & 0xff00) | ((unsigned short)a1 & 0xfe);
    dx = 0x28;                                           /* 02362 mov dx,0x28 */
    ax = (unsigned short)(ax * dx);                      /* 02365 mul dx (low 16 -> ax) */

    di = (unsigned short)a0;                             /* 02367 mov di,[bp+8] */
    di = (unsigned short)(di >> 1);                      /* 0236a shr di,1 */
    di = (unsigned short)(di >> 1);                      /* 0236c shr di,1 */
    di = (unsigned short)(di + ax);                      /* 0236e add di,ax */

    /* --- read sprite header --- */
    si = (unsigned short)a2;                             /* 02370 mov si,[bp+0xc] */
    bx = (unsigned short)W(si);                          /* 02373 mov bx,[si]  (width) */
    bx = (unsigned short)(bx + 4);                       /* 02375 add bx,4 */
    bx = (unsigned short)(bx >> 1);                      /* 02378 shr bx,1 */
    bl = (unsigned char)((bx & 0xff) >> 1);              /* 0237a shr bl,1 (byte op on BL) */
    bx = (bx & 0xff00) | bl;                             /*        bx low byte updated, high intact */

    ax = (unsigned short)W(si + 2);                      /* 0237c mov ax,[si+2] (height) */
    ax = (ax & 0xff00) | (((ax & 0xff) + 1) & 0xff);     /* 0237f inc al (byte) */
    ax = (ax & 0xff00) | (((ax & 0xff) >> 1) & 0xff);    /* 02381 shr al,1 (byte) */
    ah = (int)(signed char)(ax & 0xff);                  /* 02383 mov ah,al  (row-pair count) */

    si = (unsigned short)(si + 4);                       /* 02385 add si,4 (-> pixel data) */

    dx = (unsigned short)(0x50 - (bx & 0xffff));         /* 02388 mov dx,0x50 / 0238b sub dx,bx */

    if ((cl & 1) == 0) goto L23ba;                       /* 0238d test cl,1 / 02390 je 0x23ba */

    /* ---- odd-y bank path (start in odd bank) ---- */
    di = (unsigned short)(di + 0x2000);                  /* 02392 add di,0x2000 */
L2396:
    {   /* 02396 mov cl,bl ; 02398 lodsb ; xor es:[di],al ; inc di ; loop */
        int cnt = bl;                                    /* CL = BL (CH assumed 0) */
        while (cnt-- > 0) {
            ax = (ax & 0xff00) | B(si);                  /* 02398 lodsb -> al = [si]; si++ */
            si = (unsigned short)(si + 1);
            VIDEO[(unsigned short)di] ^= (ax & 0xff);    /* 02399 xor es:[di],al */
            di = (unsigned short)(di + 1);               /* 0239c inc di */
        }
    }
    di = (unsigned short)(di ^ 0x2000);                  /* 0239f xor di,0x2000 */
    di = (unsigned short)(di + dx);                      /* 023a3 add di,dx */
    {   /* 023a5 mov cl,bl ; 023a7 lodsb ; xor es:[di],al ; inc di ; loop */
        int cnt = bl;
        while (cnt-- > 0) {
            ax = (ax & 0xff00) | B(si);                  /* 023a7 lodsb -> al = [si]; si++ */
            si = (unsigned short)(si + 1);
            VIDEO[(unsigned short)di] ^= (ax & 0xff);    /* 023a8 xor es:[di],al */
            di = (unsigned short)(di + 1);               /* 023ab inc di */
        }
    }
    di = (unsigned short)(di - (bx & 0xffff));           /* 023ae sub di,bx */
    di = (unsigned short)(di ^ 0x2000);                  /* 023b0 xor di,0x2000 */
    ah = (int)(signed char)((ah - 1) & 0xff);            /* 023b4 dec ah (byte) */
    if (ah > 0) goto L2396;                              /* 023b6 jg 0x2396 (signed) */
    goto L23dc;                                          /* 023b8 jmp 0x23dc */

    /* ---- even-y bank path ---- */
L23ba:
    {   /* 023ba mov cl,bl ; 023bc lodsb ; xor es:[di],al ; inc di ; loop */
        int cnt = bl;
        while (cnt-- > 0) {
            ax = (ax & 0xff00) | B(si);                  /* 023bc lodsb -> al = [si]; si++ */
            si = (unsigned short)(si + 1);
            VIDEO[(unsigned short)di] ^= (ax & 0xff);    /* 023bd xor es:[di],al */
            di = (unsigned short)(di + 1);               /* 023c0 inc di */
        }
    }
    di = (unsigned short)(di - (bx & 0xffff));           /* 023c3 sub di,bx */
    di = (unsigned short)(di ^ 0x2000);                  /* 023c5 xor di,0x2000 */
    {   /* 023c9 mov cl,bl ; 023cb lodsb ; xor es:[di],al ; inc di ; loop */
        int cnt = bl;
        while (cnt-- > 0) {
            ax = (ax & 0xff00) | B(si);                  /* 023cb lodsb -> al = [si]; si++ */
            si = (unsigned short)(si + 1);
            VIDEO[(unsigned short)di] ^= (ax & 0xff);    /* 023cc xor es:[di],al */
            di = (unsigned short)(di + 1);               /* 023cf inc di */
        }
    }
    di = (unsigned short)(di ^ 0x2000);                  /* 023d2 xor di,0x2000 */
    di = (unsigned short)(di + dx);                      /* 023d6 add di,dx */
    ah = (int)(signed char)((ah - 1) & 0xff);            /* 023d8 dec ah (byte) */
    if (ah > 0) goto L23ba;                              /* 023da jg 0x23ba (signed) */

L23dc:
    /* AX at ret = (AH<<8)|AL.  AH lives in the `ah` mirror (=0 after the loops
     * exit via "dec ah / jg"; if height was 0 it keeps the header value); AL is
     * the last byte loaded by lodsb (low byte of `ax`).  Callers ignore this. */
    return (int)(unsigned short)(((ah & 0xff) << 8) | (ax & 0xff)); /* 023e1 ret (AX) */
}
