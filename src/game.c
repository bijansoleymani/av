/* game.c — Arcade Volleyball game logic, decompiled from AV.EXE.
 *
 * Each function is a faithful translation of the corresponding 16-bit routine
 * (address noted in the header comment).  Memory is the flat DS[] model from
 * dos.h; a "near pointer" is a dsptr (offset into DS).  Turbo C `int` is 16-bit
 * signed, reproduced with `short`/int and explicit masking on 8-bit ops.
 *
 * This file is built up in layers; see game_protos.h for the full roster.
 */
#include "dos.h"
#include "game_protos.h"

/* IMG(off): a sprite-pointer global is just a word (dsptr) in DS. */
#define IMG(off) UW(off)

/* ============================ render primitives ============================ */

/* 0x022bf — wait for CGA vertical retrace (frame sync). In the port this is the
 * natural place to present the framebuffer and pace/poll the host. */
void wait_vsync(void)
{
    platform_pump();
}

/* 0x022d0 — draw_sprite(x,y,img): copy a pre-shifted CGA sprite to VIDEO.
 * The low 2 bits of x select the pre-shifted copy at the call site; here x>>2
 * gives the byte column and the image rows are woven into the two CGA banks. */
void draw_sprite(int x, int y, dsptr img)
{
    unsigned di;
    dsptr si = img;
    int cl = y & 0xff;
    int bx, al, dx, i;

    di = (unsigned)((y & 0xfe) * 0x28) + ((unsigned)(x & 0xffff) >> 2);
    bx = (UW(si) + 4) >> 2;                       /* bytes per row */
    al = (((UW(si + 2) & 0xff) + 1) & 0xff) >> 1; /* number of row-pairs */
    si += 4;
    dx = 0x50 - bx;

    if (cl & 1) {
        di += 0x2000;
        while (al > 0) {
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = DS[(dsptr)(si + i)];
            di += bx; si += bx;
            di ^= 0x2000;
            di += dx;
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = DS[(dsptr)(si + i)];
            di += bx; si += bx;
            di -= bx;
            di ^= 0x2000;
            al--;
        }
    } else {
        while (al > 0) {
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = DS[(dsptr)(si + i)];
            di += bx; si += bx;
            di -= bx;
            di ^= 0x2000;
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = DS[(dsptr)(si + i)];
            di += bx; si += bx;
            di ^= 0x2000;
            di += dx;
            al--;
        }
    }
}

/* 0x023e2 — draw_sprite2(x,y,img): erase the block a sprite would occupy
 * (rep stosb with 0). Reads img only for its width/height header. */
void draw_sprite2(int x, int y, dsptr img)
{
    unsigned di;
    dsptr si = img;
    int cl = y & 0xff;
    int bx, ah, dx, i;

    di = (unsigned)((y & 0xfe) * 0x28) + ((unsigned)(x & 0xffff) >> 2);
    bx = (UW(si) + 4) >> 2;
    ah = (((UW(si + 2) & 0xff) + 1) & 0xff) >> 1;
    dx = 0x50 - bx;

    if (cl & 1) {
        di += 0x2000;
        while (ah > 0) {
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = 0;
            di += bx;
            di ^= 0x2000;
            di += dx;
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = 0;
            di += bx;
            di -= bx;
            di ^= 0x2000;
            ah--;
        }
    } else {
        while (ah > 0) {
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = 0;
            di += bx;
            di -= bx;
            di ^= 0x2000;
            for (i = 0; i < bx; i++) VIDEO[(di + i) & 0x3fff] = 0;
            di += bx;
            di ^= 0x2000;
            di += dx;
            ah--;
        }
    }
}

/* ============================ data loading ============================ */

/* 0x0041a — read_chunk(n): malloc n bytes in the DS heap and read n bytes from
 * the open AV.DAT into it; returns the buffer (dsptr). */
dsptr read_chunk(int n)
{
    dsptr si = dos_malloc((uint16_t)n);
    sys_read(dat_fd, si, (uint16_t)n);
    return si;
}

/* 0x00345 — preshift_sprite(group): given group[0] = a loaded CGA sprite,
 * generate group[1..3] as copies shifted right by 1,2,3 pixels (2 bits each),
 * so draw_sprite can place the sprite at any horizontal pixel. group is the DS
 * offset of a dsptr[4] array. */
void preshift_sprite(dsptr group)
{
    int bpr, rows, k, row, col;
    dsptr base = UW(group);
    bpr  = (UW(base) + 4) >> 2;                 /* (width+4)/4 */
    rows = UW(base + 2) + 1;                     /* height+1     */

    for (k = 0; k < 3; k++) {
        dsptr src = UW(group + k * 2);
        dsptr dst;
        uint16_t size = bgi_imagesize(0, 0, UW(src), rows - 1);
        UW(group + (k + 1) * 2) = dos_malloc(size);
        dst = UW(group + (k + 1) * 2);
        for (col = 0; col < 4; col++) B(dst++) = B(src++);   /* copy 4-byte header */
        for (row = 0; row < rows; row++) {
            int carry = 0;
            for (col = 0; col < bpr; col++) {
                int v = B(src);
                B(dst++) = (unsigned char)(((v >> 2) + carry) & 0xff);
                carry = (v & 3) << 6;
                src++;
            }
        }
    }
}

/* 0x0043a — load_data(): open AV.DAT, read the 20 sprite chunks into the DS
 * heap, build the pre-shifted copies, and capture the menu "clear strip".
 * Returns 1 on success, 0 if AV.DAT is missing. */
int load_data(void)
{
    int si;
    dat_fd = sys_open(0x2cb /* "av.dat" */, 1);
    if (dat_fd == -1) {                          /* 0xffff */
        bgi_outtext(0x2d2 /* "File ``AV.DAT'' not found." */);
        return 0;
    }
    IMG(0x9da) = read_chunk(0x46);
    IMG(0xa38) = read_chunk(0x46);
    IMG(0x9ba) = read_chunk(0x1a);
    IMG(0x9ee) = read_chunk(0x10a);
    IMG(0x9f6) = read_chunk(0x10a);
    IMG(0x9fe) = read_chunk(0x132);
    IMG(0xa06) = read_chunk(0x132);
    IMG(0x994) = read_chunk(0x10a);
    IMG(0x99c) = read_chunk(0x10a);
    IMG(0x9a4) = read_chunk(0x132);
    IMG(0x9ac) = read_chunk(0x132);
    IMG(0x9b4) = read_chunk(0xca);
    IMG(0xa18) = read_chunk(0x146);
    IMG(0xa20) = read_chunk(0x146);
    IMG(0xa28) = read_chunk(0x146);
    IMG(0xa30) = read_chunk(0x146);
    IMG(0x966) = read_chunk(0xb6);
    IMG(0x96e) = read_chunk(0xb6);
    IMG(0x976) = read_chunk(0xb6);
    IMG(0x97e) = read_chunk(0xb6);
    sys_close(dat_fd);

    for (si = 0; si < 4; si++) {
        preshift_sprite((dsptr)(0x994 + si * 8));
        preshift_sprite((dsptr)(0x9ee + si * 8));
        preshift_sprite((dsptr)(0xa18 + si * 8));
        preshift_sprite((dsptr)(0x966 + si * 8));
    }

    /* Capture the "clear strip" used to blank a line of menu/score text, exactly
     * as the original (0x5aa..0x63e): fill a white bar, snapshot it into font_img
     * (used only for its width/height when erasing), erase it, then RESET the
     * fill colour to black and blank the strip.  That final setfillstyle(1,0) is
     * what leaves the BGI fill colour black for later bar() score clears. */
    bgi_setfillstyle(1, 0xf);
    {
        uint16_t sz = bgi_imagesize(0, 0, 0x70, 7);
        font_img = dos_malloc(sz);
        wait_vsync();
        bgi_bar(0, 0, 0x70, 7);
        bgi_getimage(0, 0, 0x70, 7, font_img);
        draw_sprite2(0, 0, font_img);
        bgi_setfillstyle(1, 0);          /* leaves the fill colour BLACK */
        bgi_bar(0, 0, 0x70, 7);
    }
    return 1;
}
