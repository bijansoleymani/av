/* 0x0234e sub_0234e - XOR-blit a sprite into CGA video memory (draw_sprite via XOR) */
#include "dos.h"
#include "game_protos.h"

/* XOR-blits the sprite at dsptr `img` to screen position (x, y).
 *
 *   x, y : top-left pixel position (x is divided by 4 -> CGA byte column)
 *   img  : sprite dsptr; header is W(img)=pixel width, W(img+2)=pixel height,
 *          followed by the packed 2-bits-per-pixel row bytes.
 *
 * CGA memory has two interleaved 8 KB banks: even scanlines live at the base,
 * odd scanlines at +0x2000.  Each loop iteration draws one row into each bank
 * (a scanline pair), toggling the bank bit (^0x2000) between them.  The two y
 * parities differ only in which bank they start with and the order of the
 * stride adjustments, so the code keeps two near-mirror-image paths.
 *
 * Return value is the leftover AX (last sprite byte in the low half, row
 * counter in the high half); all callers ignore it. */
int sub_0234e(int x, int y, int img)
{
    unsigned dst;               /* VIDEO offset being written */
    unsigned src;              /* sprite data pointer (dsptr) */
    unsigned char byte_width;  /* bytes per row = loop count (BL only) */
    unsigned row_span;         /* full 16-bit BX used by the "sub di,bx" strides */
    unsigned next_line;        /* stride from end of one row to start of next */
    int rows;                  /* remaining scanline-pair count (signed dec/jg) */
    unsigned pix = 0;          /* last sprite byte read (low half of return AX) */
    int col;

    /* Starting VIDEO offset: (y & ~1 in AL, y-high intact) * 40 + x/4.
     * The "and al,0xfe" clears only bit0 of the y low byte, so the row index
     * that gets multiplied by 40 is ((y & 0xff00) | (y & 0xfe)). */
    unsigned y_mul = ((unsigned short)y & 0xff00) | ((unsigned short)y & 0xfe);
    unsigned row_base = (unsigned short)(y_mul * 0x28);
    dst = (unsigned short)(((unsigned short)x >> 2) + row_base);

    /* Sprite header: width bytes = ((pixel_width + 4) >> 1) then an 8-bit shr
     * of the low byte only.  The loop count uses that low byte (byte_width),
     * but the stride subtractions below use the full 16-bit result (row_span);
     * for real sprite widths the high byte is 0 so the two agree. */
    src = (unsigned short)img;
    {
        unsigned w = (unsigned short)((unsigned short)W(src) + 4);
        w = (unsigned short)(w >> 1);
        byte_width = (unsigned char)((w & 0xff) >> 1);   /* 8-bit shr on BL */
        row_span = (w & 0xff00) | byte_width;            /* BH intact, BL updated */
    }

    /* Height header -> scanline-pair count: ((pixel_height + 1) >> 1), all 8-bit.
     * The "inc al" wraps within the low byte (0xff -> 0x00) before the "shr al,1",
     * so keep the & 0xff after the increment. */
    rows = (signed char)(((((unsigned short)W(src + 2) & 0xff) + 1) & 0xff) >> 1);

    src = (unsigned short)(src + 4);                     /* skip 4-byte header */

    /* Advance from the end of a byte-row to the start of the next (0x50 = full
     * two-bank scanline width). */
    next_line = (unsigned short)(0x50 - row_span);

    if ((y & 1) != 0) {
        /* Odd y: start in the odd bank (+0x2000). */
        dst = (unsigned short)(dst + 0x2000);
        do {
            for (col = 0; col < byte_width; col++) {
                pix = B(src);
                src = (unsigned short)(src + 1);
                VIDEO[(unsigned short)dst] ^= pix;
                dst = (unsigned short)(dst + 1);
            }
            dst = (unsigned short)(dst ^ 0x2000);        /* -> even bank */
            dst = (unsigned short)(dst + next_line);
            for (col = 0; col < byte_width; col++) {
                pix = B(src);
                src = (unsigned short)(src + 1);
                VIDEO[(unsigned short)dst] ^= pix;
                dst = (unsigned short)(dst + 1);
            }
            dst = (unsigned short)(dst - row_span);
            dst = (unsigned short)(dst ^ 0x2000);        /* back to odd bank */
            rows = (signed char)(rows - 1);
        } while (rows > 0);
    } else {
        /* Even y: start in the even (base) bank. */
        do {
            for (col = 0; col < byte_width; col++) {
                pix = B(src);
                src = (unsigned short)(src + 1);
                VIDEO[(unsigned short)dst] ^= pix;
                dst = (unsigned short)(dst + 1);
            }
            dst = (unsigned short)(dst - row_span);
            dst = (unsigned short)(dst ^ 0x2000);        /* -> odd bank */
            for (col = 0; col < byte_width; col++) {
                pix = B(src);
                src = (unsigned short)(src + 1);
                VIDEO[(unsigned short)dst] ^= pix;
                dst = (unsigned short)(dst + 1);
            }
            dst = (unsigned short)(dst ^ 0x2000);        /* back to even bank */
            dst = (unsigned short)(dst + next_line);
            rows = (signed char)(rows - 1);
        } while (rows > 0);
    }

    /* AX = (rows << 8) | last sprite byte; callers ignore it. */
    return (int)(unsigned short)(((rows & 0xff) << 8) | (pix & 0xff));
}
