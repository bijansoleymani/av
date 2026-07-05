/* rtl.c — Turbo C runtime helpers used by the game (menu key input, PC speaker,
 * BIOS equipment).  Backed by an SDL-fed key queue so the menu and "define keys"
 * screens work.  During gameplay the custom ISR path (input.c) drives movement;
 * these BIOS-style calls drive menu navigation, exactly as in the original where
 * the menu ran with the normal BIOS keyboard handler installed. */
#include "dos.h"
#include <string.h>

/* ---- key queue: each entry is a BIOS keystroke = (scancode<<8)|ascii ---- */
#define QN 64
static int q[QN], qh, qt;
void rtl_push_key(int biosval) { int n=(qt+1)%QN; if (n!=qh){ q[qt]=biosval; qt=n; } }
static int q_empty(void) { return qh==qt; }
static int q_pop(void) { int v=q[qh]; qh=(qh+1)%QN; return v; }

/* 0x0432b — kbhit(): DOS AH=0Bh -> AL=0xFF if a key waits, else 0 (cwde). */
int sub_0432b(void)
{
    platform_pump();
    return q_empty() ? 0 : -1;
}

/* 0x04104 — getch(): DOS AH=07h -> AL = character (or 0 then scancode for
 * extended keys, classic two-read behaviour). Blocks (pumping the host). */
static int pending_scan = 0;
int sub_04104(void)
{
    int v, ascii;
    if (pending_scan) { int s=pending_scan; pending_scan=0; return s; }
    for (;;) {
        platform_pump();
        if (platform_should_quit()) return 0x1b;   /* treat window-close as Esc */
        if (!q_empty()) break;
    }
    v = q_pop();
    ascii = v & 0xff;
    if (ascii == 0) { pending_scan = (v >> 8) & 0xff; return 0; }
    return ascii;
}

/* 0x03c3b — bioskey(mode): INT 16h. mode 0 = wait, return AX=(scan<<8)|ascii.
 * mode 1 = peek, return 0 if none (else the key, left queued). */
int sub_03c3b(int mode)
{
    if (mode == 1) {
        platform_pump();
        return q_empty() ? 0 : q[qh];
    }
    for (;;) {
        platform_pump();
        if (platform_should_quit()) return 0x011b;  /* Esc */
        if (!q_empty()) break;
    }
    return q_pop();
}

/* 0x03c21 — equipment word (INT 11h). We report no game adapter, so the game
 * skips joystick auto-calibration. */
int sub_03c21(void) { return 0; }

/* 0x04b2b — sound(freq): program the PC speaker. 0x04b57 — nosound(). */
void sub_04b2b(int freq) { speaker_tone(freq); }
void sub_04b57(void)     { speaker_off(); }

/* lib_5fca — BGI rectangle(x1,y1,x2,y2) (kept for any residual references). */
void lib_5fca(int x1, int y1, int x2, int y2) { bgi_rectangle(x1, y1, x2, y2); }

/* 0x03f7a — delay(ms): Turbo C millisecond delay (BIOS tick busy-wait). */
void sub_03f7a(int ms) { extern void platform_delay(int ms); platform_delay(ms); }

/* 0x04439 — itoa(value, buf, radix): number -> string in DS[buf]; returns buf. */
dsptr sub_04439(int value, dsptr buf, int radix)
{
    char tmp[24]; int i = 0, neg = 0; unsigned uv;
    if (radix == 10 && value < 0) { neg = 1; uv = (unsigned)(-(short)value); }
    else uv = (unsigned short)value;
    if (uv == 0) tmp[i++] = '0';
    while (uv) { int d = uv % (unsigned)radix; tmp[i++] = (char)(d < 10 ? '0'+d : 'a'+d-10); uv /= (unsigned)radix; }
    if (neg) tmp[i++] = '-';
    { char *o = (char *)FARPTR(buf); int j; for (j = 0; j < i; j++) o[j] = tmp[i-1-j]; o[i] = 0; }
    return buf;
}
