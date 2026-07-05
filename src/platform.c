/* platform.c — SDL2 host: presents the CGA framebuffer, pumps input, times
 * frames, and provides the program entry point. */
#include "dos.h"
#include "game_protos.h"
#include <SDL.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SCRW 320
#define SCRH 200
#define SCALE 3

void platform_shutdown(void);

/* ---- PC-speaker emulation: a square-wave SDL audio device ----
 *
 * The game drives the speaker with sound()/nosound() (speaker_tone/off).  The
 * real DOS point-scored sound is simply a clean, steady ~5 kHz tone held for the
 * ball-settle (~0.3 s) — verified by spectrum-analysing the actual game audio.
 * The catch: the settle loop calls nosound() at the top of every frame and
 * sound() again mid-frame, so a naive model chops the tone into clicks.  We
 * bridge those instantaneous re-asserts with a short release envelope, giving a
 * continuous glitch-free tone that stops only when nosound() is left un-renewed. */
#define AUDIO_RATE 44100
#define SPK_RELEASE 1800                  /* ~41 ms grace: spans one settle frame
                                             so per-frame nosound()/sound() stays
                                             continuous; also the tone's tail      */
#define SPK_AMP 8000
#define SPK_LP_A 0.457                    /* one-pole low-pass, ~5.5 kHz cutoff.
                                             The real DOS speaker/recording rolls
                                             the square's 3rd harmonic (15 kHz)
                                             down to ~0.18 of the fundamental; a
                                             raw square is 0.33 (too bright). This
                                             filter matches the measured timbre.   */
static SDL_AudioDeviceID g_audio;
static volatile int g_spk_freq;           /* frequency set by sound(); 0 = none   */
static volatile int g_spk_on;             /* 1 after sound(), 0 after nosound()    */
static double       g_phase;

static void audio_cb(void *ud, Uint8 *stream, int len)
{
    Sint16 *out = (Sint16 *)stream;
    int n = len / (int)sizeof(Sint16), i;
    static int rel;                       /* release samples remaining            */
    static double lp;                     /* low-pass filter state                */
    (void)ud;
    for (i = 0; i < n; i++) {
        int f, playing;
        double raw;
        if (g_spk_on) { rel = SPK_RELEASE; playing = 1; }   /* asserted */
        else if (rel > 0) { rel--; playing = 1; }           /* within release grace */
        else playing = 0;
        f = g_spk_freq;
        if (playing && f > 0) {
            raw = (g_phase < 0.5) ? (double)SPK_AMP : (double)-SPK_AMP;
            g_phase += (double)f / AUDIO_RATE;
            if (g_phase >= 1.0) g_phase -= 1.0;
        } else {
            raw = 0.0;
        }
        lp = (1.0 - SPK_LP_A) * raw + SPK_LP_A * lp;   /* soften harsh harmonics */
        out[i] = (Sint16)lp;
    }
}

static SDL_Window   *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture  *g_tex;
static uint32_t      g_pixels[SCRW * SCRH];
static int           g_quit;

/* CGA palette 1, high intensity: 0=black 1=cyan 2=magenta 3=white. */
static const uint32_t CGA_PAL[4] = {
    0x000000, 0x55FFFF, 0xFF55FF, 0xFFFFFF
};

/* Optional headless screenshot: after AV_SHOT_FRAMES presents, dump the RGB
 * buffer to $AV_SHOT (PPM) and exit.  Lets us validate rendering without a GUI. */
static void maybe_shot(void)
{
    const char *path = getenv("AV_SHOT");
    static int n = 0, thresh = -1;
    int i;
    FILE *fp;
    if (!path) return;
    if (thresh < 0) { const char *f = getenv("AV_SHOT_FRAMES"); thresh = f ? atoi(f) : 4; }
    /* sequence mode: dump every AV_SHOT_STEP-th frame as build/seq/NNN.ppm */
    {
        const char *seq = getenv("AV_SHOT_SEQ");
        if (seq) {
            const char *st = getenv("AV_SHOT_STEP");
            int step = st ? atoi(st) : 3;
            if (n % step == 0 && n / step < 999) {
                char nm[256]; snprintf(nm, sizeof nm, "%s/%03d.ppm", seq, n / step);
                fp = fopen(nm, "wb");
                if (fp) {
                    fprintf(fp, "P6\n%d %d\n255\n", SCRW, SCRH);
                    for (i = 0; i < SCRW * SCRH; i++) {
                        unsigned c = g_pixels[i];
                        unsigned char rgb[3] = { (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff };
                        fwrite(rgb, 1, 3, fp);
                    }
                    fclose(fp);
                }
            }
        }
    }
    if (++n < thresh) return;
    fp = fopen(path, "wb");
    fprintf(fp, "P6\n%d %d\n255\n", SCRW, SCRH);
    for (i = 0; i < SCRW * SCRH; i++) {
        unsigned c = g_pixels[i];
        unsigned char rgb[3] = { (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff };
        fwrite(rgb, 1, 3, fp);
    }
    fclose(fp);
    platform_shutdown();
    exit(0);
}

/* Scripted key injection for headless testing. AV_INJECT="frame=biosval,..."
 * where biosval = (scancode<<8)|ascii. Feeds both the menu key queue and the
 * gameplay ISR path. Held movement keys use a matching "frame=..80" to release. */
static void maybe_inject(int frame)
{
    const char *s = getenv("AV_INJECT");
    if (!s) return;
    while (*s) {
        int f = (int)strtol(s, (char **)&s, 10);
        if (*s == '=') s++;
        int val = (int)strtol(s, (char **)&s, 16);
        if (f == frame) {
            int sc = (val >> 8) & 0xff;
            rtl_push_key(val);
            if (sc) av_kbd_scancode((unsigned char)sc);
        }
        if (*s == ',') s++;
    }
}

/* De-interleave VIDEO[] (two CGA banks, 2bpp) into the RGBA present buffer. */
static void present(void)
{
    static int frame = 0;
    int y, x;
    maybe_inject(frame++);
    for (y = 0; y < SCRH; y++) {
        const unsigned char *row =
            VIDEO + ((y >> 1) * 80) + ((y & 1) ? 0x2000 : 0);
        for (x = 0; x < SCRW; x++) {
            unsigned byte = row[x >> 2];
            unsigned pix  = (byte >> (6 - 2 * (x & 3))) & 3;
            g_pixels[y * SCRW + x] = 0xFF000000u | CGA_PAL[pix];
        }
    }
    if (g_ren && g_tex) {
        SDL_UpdateTexture(g_tex, NULL, g_pixels, SCRW * sizeof(uint32_t));
        SDL_RenderClear(g_ren);
        SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
        SDL_RenderPresent(g_ren);
    }
    maybe_shot();
}

void cga_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); exit(1); }
    {
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = AUDIO_RATE; want.format = AUDIO_S16SYS;
        want.channels = 1; want.samples = 512; want.callback = audio_cb;
        g_audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (g_audio) SDL_PauseAudioDevice(g_audio, 0);   /* start (silent) */
    }
    g_win = SDL_CreateWindow("Arcade Volleyball (1988) — reconstructed",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCRW * SCALE, SCRH * SCALE, SDL_WINDOW_SHOWN);
    if (g_win) {
        g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!g_ren) g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (g_ren)
        g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, SCRW, SCRH);
}
void cga_close(void) { }
void platform_shutdown(void)
{
    if (g_tex) SDL_DestroyTexture(g_tex);
    if (g_ren) SDL_DestroyRenderer(g_ren);
    if (g_win) SDL_DestroyWindow(g_win);
    SDL_Quit();
}

extern int xt_from_sdl(int sdl_scancode);

void platform_pump(void)
{
    SDL_Event e;
    present();
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { g_quit = 1; W(0x9d4) = 1; }
        else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            int xt = xt_from_sdl(e.key.keysym.scancode);
            int sym = e.key.keysym.sym, ascii = 0;
            if (sym >= 32 && sym < 127) ascii = sym;
            else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) ascii = 13;
            else if (sym == SDLK_ESCAPE) ascii = 27;
            else if (sym == SDLK_BACKSPACE) ascii = 8;
            if (xt) av_kbd_scancode((unsigned char)xt);
            rtl_push_key((xt << 8) | ascii);       /* also feed the menu key queue */
        } else if (e.type == SDL_KEYUP) {
            int xt = xt_from_sdl(e.key.keysym.scancode);
            if (xt) av_kbd_scancode((unsigned char)(xt | 0x80));
        }
    }
    SDL_Delay(8);
}
int platform_should_quit(void) { return g_quit; }

void platform_delay(int ms)
{
    Uint32 t0;
    if (ms <= 0) { platform_pump(); return; }
    present();
    t0 = SDL_GetTicks();
    while ((int)(SDL_GetTicks() - t0) < ms) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { g_quit = 1; W(0x9d4) = 1; }
            else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                int xt = xt_from_sdl(e.key.keysym.scancode);
                if (xt) av_kbd_scancode((unsigned char)xt);
            } else if (e.type == SDL_KEYUP) {
                int xt = xt_from_sdl(e.key.keysym.scancode);
                if (xt) av_kbd_scancode((unsigned char)(xt | 0x80));
            }
        }
        SDL_Delay(2);
    }
}

/* stubs for input methods not wired yet (joystick/mouse players stay idle) */
int  platform_int33(dsptr in, dsptr out) { (void)in; (void)out; return 0; }
int  joy_read_button(int p) { (void)p; return 0; }
int  joy_read_axis(int p, int *t) { (void)p; (void)t; return 0; }
int  mouse_read(int *b, int *dx, int *dy) { (void)b; (void)dx; (void)dy; return 0; }
void speaker_tone(int freq) { g_spk_freq = (freq > 0) ? freq : 0; g_spk_on = 1; }
void speaker_off(void) { g_spk_on = 0; }

/* ---- Layer 1 test harness: load the real AV.DAT and show every sprite. ---- */
#ifdef RENDER_TEST
#define IMG(off) UW(off)
int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    dos_data_init();
    cga_init();
    if (!load_data()) { fprintf(stderr, "load_data failed (AV.DAT missing?)\n"); return 1; }

    /* draw the four animation frames of each pre-shifted group + singles */
    memset(VIDEO, 0, sizeof VIDEO);
    {
        int f;
        /* player 1 frames (group base 0x994), 4 frames of 8 bytes apart */
        for (f = 0; f < 4; f++) draw_sprite(10 + f * 45, 20,  IMG(0x994 + f * 8));
        for (f = 0; f < 4; f++) draw_sprite(10 + f * 45, 60,  IMG(0x9ee + f * 8));
        for (f = 0; f < 4; f++) draw_sprite(10 + f * 45, 100, IMG(0xa18 + f * 8)); /* ball */
        for (f = 0; f < 4; f++) draw_sprite(10 + f * 45, 140, IMG(0x966 + f * 8)); /* decor */
        draw_sprite(0,   170, IMG(0x9da));   /* left post  */
        draw_sprite(300, 170, IMG(0xa38));   /* right post */
        draw_sprite(150, 90,  IMG(0x9b4));   /* net */
    }
    while (!g_quit) platform_pump();
    platform_shutdown();
    return 0;
}
#else
/* ---- real entry point: run the decompiled game ---- */
int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    dos_data_init();
    kb_install();
    game_main();          /* opens the window via cga_init/initgraph, runs menu+play */
    platform_shutdown();
    return 0;
}
#endif
