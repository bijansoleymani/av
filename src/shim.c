/* shim.c — RTL + Borland BGI graphics shim, implemented over DS[]/VIDEO[].
 *
 * The game reaches these through far/near calls in the original; here they are
 * ordinary C functions with the semantics reverse-engineered from AV.EXE's
 * library (INT 21h file I/O, BGI imagesize/getimage/putimage/bar, etc).  The
 * BGI image buffer format matches AV.DAT: {u16 width; u16 height; rows...},
 * width=x2-x1, height=y2-y1, bytes/row = (width+4)>>2 (same as draw_sprite). */
#include "dos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "font8x8.h"

/* ---------------- AV.DAT file I/O ---------------- */
static FILE *g_files[8];

int sys_open(dsptr name, int mode)
{
    int i;
    char path[64];
    (void)mode;
    strncpy(path, (char *)FARPTR(name), sizeof path - 1);
    path[sizeof path - 1] = 0;
    for (i = 1; i < 8; i++) if (!g_files[i]) break;
    if (i >= 8) return -1;
    g_files[i] = fopen(path, "rb");
    if (!g_files[i]) {                       /* try lower/upper-case variants */
        char alt[64]; size_t k;
        for (k = 0; path[k]; k++) alt[k] = (char)toupper((unsigned char)path[k]);
        alt[k] = 0; g_files[i] = fopen(alt, "rb");
    }
    if (!g_files[i]) return -1;
    return i;
}
int sys_read(int fd, dsptr buf, uint16_t len)
{
    if (fd < 1 || fd >= 8 || !g_files[fd]) return -1;
    return (int)fread(DS + buf, 1, len, g_files[fd]);
}
int sys_close(int fd)
{
    if (fd < 1 || fd >= 8 || !g_files[fd]) return -1;
    fclose(g_files[fd]); g_files[fd] = NULL; return 0;
}

/* The game only ever calls printf with a bare message string (e.g. the CGA
 * error), so a plain fputs is faithful and avoids variadic-promotion UB. */
int dos_printf(dsptr fmt)
{
    return fputs((char *)FARPTR(fmt), stderr);
}
void dos_exit(int code) { extern void platform_shutdown(void); platform_shutdown(); exit(code); }
void registerbgidriver(void) { /* driver is our shim; nothing to register */ }

/* ---------------- BGI graphics over VIDEO[] ---------------- */
static int g_fillcolor = 3, g_curx, g_cury, g_color = 3;

int graphresult(void) { return 0; }          /* always grOk in the port */

/* bytes-per-row / size, matching draw_sprite and the AV.DAT layout exactly. */
static int bpr_of(int width) { return (width + 4) >> 2; }

uint16_t bgi_imagesize(int x1, int y1, int x2, int y2)
{
    int w = x2 - x1, h = y2 - y1;
    return (uint16_t)(4 + bpr_of(w) * (h + 1) + 2);
}

/* Read a byte (2bpp, 4 px) from CGA VIDEO at pixel column-byte cb, row y. */
static unsigned video_row_byte(int cb, int y)
{
    unsigned di = (unsigned)((y & 0xfffe) * 0x28) + cb + ((y & 1) ? 0x2000 : 0);
    return VIDEO[di & 0x3fff];
}
static void video_put_byte(int cb, int y, unsigned v)
{
    unsigned di = (unsigned)((y & 0xfffe) * 0x28) + cb + ((y & 1) ? 0x2000 : 0);
    VIDEO[di & 0x3fff] = (unsigned char)v;
}

void bgi_getimage(int x1, int y1, int x2, int y2, dsptr buf)
{
    int w = x2 - x1, h = y2 - y1, bpr = bpr_of(w), r, c;
    dsptr p = buf;
    UW(p) = (uint16_t)w; UW(p + 2) = (uint16_t)h; p += 4;
    for (r = 0; r <= h; r++)
        for (c = 0; c < bpr; c++)
            B(p++) = (unsigned char)video_row_byte((x1 >> 2) + c, y1 + r);
    UW(p) = 0;                                  /* trailing pad word */
}

void bgi_setfillstyle(int pattern, int color) { (void)pattern; g_fillcolor = color; }

/* single-pixel plot into the CGA framebuffer (2 bits/pixel). */
static void video_put_pixel(int x, int y, int color)
{
    unsigned di, sh; unsigned char m, v;
    if (x < 0 || x >= 320 || y < 0 || y >= 200) return;
    di = (unsigned)((y & 0xfffe) * 0x28) + (x >> 2) + ((y & 1) ? 0x2000 : 0);
    sh = 6 - 2 * (x & 3);
    m  = (unsigned char)~(3u << sh);
    v  = (unsigned char)((color & 3) << sh);
    VIDEO[di & 0x3fff] = (unsigned char)((VIDEO[di & 0x3fff] & m) | v);
}

void bgi_bar(int x1, int y1, int x2, int y2)
{
    int y, cb;
    unsigned v = (g_fillcolor & 3);
    v |= v << 2; v |= v << 4;                    /* replicate 2bpp across byte */
    for (y = y1; y <= y2; y++)
        for (cb = (x1 >> 2); cb <= (x2 >> 2); cb++)
            video_put_byte(cb, y, v);
}

void bgi_setcolor(int c) { g_color = c; }
void bgi_cleardevice(void) { memset(VIDEO, 0, sizeof VIDEO); }
void bgi_settextstyle_dir(int d) { (void)d; }

void bgi_rectangle(int x1, int y1, int x2, int y2)
{
    int x, y;
    for (x = x1; x <= x2; x++) { video_put_pixel(x, y1, g_color); video_put_pixel(x, y2, g_color); }
    for (y = y1; y <= y2; y++) { video_put_pixel(x1, y, g_color); video_put_pixel(x2, y, g_color); }
}

/* filled ellipse in the current draw colour (used for the serving-side dot). */
void bgi_fillellipse(int x, int y, int xr, int yr)
{
    int dx, dy;
    if (xr < 1) xr = 1;
    if (yr < 1) yr = 1;
    for (dy = -yr; dy <= yr; dy++)
        for (dx = -xr; dx <= xr; dx++)
            if (dx * dx * yr * yr + dy * dy * xr * xr <= xr * xr * yr * yr)
                video_put_pixel(x + dx, y + dy, g_color);
}

/* BGI 8x8 bitmapped text (default font). */
void bgi_outtextxy(int x, int y, dsptr s)
{
    const char *p = (const char *)FARPTR(s);
    int cx = x, gy, gx;
    for (; *p; p++) {
        int c = (unsigned char)*p;
        if (c >= 0x20 && c < 0x7f) {
            const unsigned char *g = FONT8X8[c - 0x20];
            for (gy = 0; gy < 8; gy++)
                for (gx = 0; gx < 8; gx++)
                    if (g[gy] & (0x80 >> gx))
                        video_put_pixel(cx + gx, y + gy, g_color);
        }
        cx += 8;
    }
    g_curx = cx; g_cury = y;
}
void bgi_outtext(dsptr s) { bgi_outtextxy(g_curx, g_cury, s); }

/* int86 — only used for the mouse (INT 33h); dispatched by platform.c. */
int dos_int86(int intno, dsptr in, dsptr out)
{
    extern int platform_int33(dsptr in, dsptr out);
    if (intno == 0x33) return platform_int33(in, out);
    return 0;
}
