/* platform_wasi.c — SDL-free platform layer for standalone WebAssembly (WASI)
 * runtimes such as wasmtime.  No display, no audio, no wall clock: frames run
 * flat out and input comes from the same AV_INJECT script that platform.c
 * honours, so a run is byte-identical to a native SDL_VIDEODRIVER=dummy run.
 *
 * present()/maybe_inject()/maybe_shot() mirror platform.c exactly — same
 * frame counting, same env contract (AV_INJECT, AV_SHOT, AV_SHOT_FRAMES,
 * AV_SHOT_SEQ, AV_SHOT_STEP) — plus AV_MAX_FRAMES (default 100000) as a
 * runaway guard, since a menu waiting forever for a key would otherwise spin.
 *
 *   wasmtime run --dir . --env AV_INJECT="5=1c0d,..." \
 *       --env AV_SHOT=frame.ppm --env AV_SHOT_FRAMES=140 av-wasi.wasm
 */
#include "dos.h"
#include "game_protos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCRW 320
#define SCRH 200

static const unsigned CGA_PAL[4] = { 0x000000, 0x55FFFF, 0xFF55FF, 0xFFFFFF };
static unsigned char g_rgb[SCRW * SCRH * 3];

void platform_shutdown(void) {}
int  platform_should_quit(void) { return 0; }
void cga_init(void) {}
void cga_close(void) {}
int  platform_int33(dsptr in, dsptr out) { (void)in; (void)out; return 0; }
int  joy_read_button(int p) { (void)p; return 0; }
int  joy_read_axis(int p, int *t) { (void)p; (void)t; return 0; }
int  mouse_read(int *b, int *dx, int *dy) { (void)b; (void)dx; (void)dy; return 0; }
void speaker_tone(int freq) { (void)freq; }
void speaker_off(void) {}

static void dump_ppm(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "cannot write %s\n", path); return; }
    fprintf(fp, "P6\n%d %d\n255\n", SCRW, SCRH);
    fwrite(g_rgb, 1, sizeof g_rgb, fp);
    fclose(fp);
}

/* Same env contract as platform.c's maybe_shot(). */
static void maybe_shot(void)
{
    const char *path = getenv("AV_SHOT");
    static int n = 0, thresh = -1;
    if (!path) return;
    if (thresh < 0) { const char *f = getenv("AV_SHOT_FRAMES"); thresh = f ? atoi(f) : 4; }
    {
        const char *seq = getenv("AV_SHOT_SEQ");
        if (seq) {
            const char *st = getenv("AV_SHOT_STEP");
            int step = st ? atoi(st) : 3;
            if (n % step == 0 && n / step < 999) {
                char nm[256]; snprintf(nm, sizeof nm, "%s/%03d.ppm", seq, n / step);
                dump_ppm(nm);
            }
        }
    }
    if (++n < thresh) return;
    dump_ppm(path);
    printf("wrote %s after %d frames\n", path, n);
    exit(0);
}

/* Tiny strtol stand-in: wasi-libc's strtol goes through intscan, which needs
 * 128-bit compiler-rt intrinsics that Homebrew doesn't ship for wasm32-wasi. */
static const char *parse_int(const char *s, int base, int *out)
{
    int v = 0;
    for (;;) {
        int c = *s, d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (base == 16 && c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        v = v * base + d;
        s++;
    }
    *out = v;
    return s;
}

/* Same as platform.c's maybe_inject(). */
static void maybe_inject(int frame)
{
    const char *s = getenv("AV_INJECT");
    if (!s) return;
    while (*s) {
        int f, val;
        s = parse_int(s, 10, &f);
        if (*s == '=') s++;
        s = parse_int(s, 16, &val);
        if (f == frame) {
            int sc = (val >> 8) & 0xff;
            rtl_push_key(val);
            if (sc) av_kbd_scancode((unsigned char)sc);
        }
        if (*s == ',') s++;
    }
}

/* De-interleave VIDEO[] into RGB; count frames identically to platform.c. */
static void present(void)
{
    static int frame = 0, max_frames = -1;
    int y, x;
    if (max_frames < 0) {
        const char *m = getenv("AV_MAX_FRAMES");
        max_frames = m ? atoi(m) : 100000;
    }
    if (frame >= max_frames) {
        fprintf(stderr, "AV_MAX_FRAMES (%d) reached, exiting\n", max_frames);
        exit(0);
    }
    maybe_inject(frame++);
    for (y = 0; y < SCRH; y++) {
        const unsigned char *row =
            VIDEO + ((y >> 1) * 80) + ((y & 1) ? 0x2000 : 0);
        for (x = 0; x < SCRW; x++) {
            unsigned byte = row[x >> 2];
            unsigned pix  = (byte >> (6 - 2 * (x & 3))) & 3;
            unsigned c    = CGA_PAL[pix];
            unsigned char *o = g_rgb + (y * SCRW + x) * 3;
            o[0] = (c >> 16) & 0xff; o[1] = (c >> 8) & 0xff; o[2] = c & 0xff;
        }
    }
    maybe_shot();
}

void platform_frame(void) { present(); }       /* no pacing: run flat out  */
void platform_pump(void)  { present(); }
void platform_delay(int ms) { (void)ms; present(); }  /* one present, like platform.c */

int main(void)
{
    dos_data_init();
    kb_install();
    game_main();
    platform_shutdown();
    return 0;
}
