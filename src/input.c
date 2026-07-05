/* input.c — keyboard input, faithful to AV.EXE's INT 9 handler.
 *
 * The original installs a keyboard ISR that reads raw XT scan codes from port
 * 0x60 and, for each keyboard-controlled player, sets three control words from
 * the player's bound scan codes.  We reproduce that logic here and feed it XT
 * scan codes translated from SDL key events (see xt_from_sdl / platform.c).
 *
 * Control words per player p:  0x9c8+p*6 = {left(-2), right(+2), jump(1)}
 * Bound scan codes per player: 0x9e +p*6 = {left, right, jump}
 * Input method per player:     (p+1)*0x3c + 0xaa   (0 == keyboard)
 * Esc release (0x81) sets the pause/quit flag at 0x9d4.
 */
#include "dos.h"
#include <SDL.h>

void kb_install(void) { /* SDL delivers key events; no vector to hook */ }

/* 0x00c5d — the INT 9 ISR body, as a normal function fed one scan code. */
void av_kbd_scancode(unsigned char sc)
{
    int player;
    if (sc == 0x81) W(0x9d4) = 1;                 /* Esc released */
    for (player = 0; player < 2; player++) {
        dsptr keys, ctrl;
        if (W((player + 1) * 0x3c + 0xaa) != 0) continue;   /* not keyboard */
        keys = (dsptr)(0x9e  + player * 6);
        ctrl = (dsptr)(0x9c8 + player * 6);
        if (sc == W(keys))          W(ctrl)     = -2;
        if (sc == W(keys) + 0x80)   W(ctrl)     = 0;
        if (sc == W(keys + 2))      W(ctrl + 2) = 2;
        if (sc == W(keys + 2) + 0x80) W(ctrl + 2) = 0;
        if (sc == W(keys + 4))      W(ctrl + 4) = 1;
        if (sc == W(keys + 4) + 0x80) W(ctrl + 4) = 0;
    }
}

/* SDL physical scancode -> IBM XT set-1 make code. Covers letters, digits,
 * keypad, arrows and common keys so the default bindings (Z/X/C and keypad
 * 1/2/3) and the "Define Keys" screen both work. */
int xt_from_sdl(int s)
{
    switch (s) {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03;
    case SDL_SCANCODE_3: return 0x04; case SDL_SCANCODE_4: return 0x05;
    case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
    case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09;
    case SDL_SCANCODE_9: return 0x0a; case SDL_SCANCODE_0: return 0x0b;
    case SDL_SCANCODE_MINUS: return 0x0c; case SDL_SCANCODE_EQUALS: return 0x0d;
    case SDL_SCANCODE_BACKSPACE: return 0x0e; case SDL_SCANCODE_TAB: return 0x0f;
    case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11;
    case SDL_SCANCODE_E: return 0x12; case SDL_SCANCODE_R: return 0x13;
    case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17;
    case SDL_SCANCODE_O: return 0x18; case SDL_SCANCODE_P: return 0x19;
    case SDL_SCANCODE_LEFTBRACKET: return 0x1a; case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
    case SDL_SCANCODE_RETURN: return 0x1c; case SDL_SCANCODE_LCTRL: return 0x1d;
    case SDL_SCANCODE_A: return 0x1e; case SDL_SCANCODE_S: return 0x1f;
    case SDL_SCANCODE_D: return 0x20; case SDL_SCANCODE_F: return 0x21;
    case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25;
    case SDL_SCANCODE_L: return 0x26; case SDL_SCANCODE_SEMICOLON: return 0x27;
    case SDL_SCANCODE_APOSTROPHE: return 0x28; case SDL_SCANCODE_GRAVE: return 0x29;
    case SDL_SCANCODE_LSHIFT: return 0x2a; case SDL_SCANCODE_BACKSLASH: return 0x2b;
    case SDL_SCANCODE_Z: return 0x2c; case SDL_SCANCODE_X: return 0x2d;
    case SDL_SCANCODE_C: return 0x2e; case SDL_SCANCODE_V: return 0x2f;
    case SDL_SCANCODE_B: return 0x30; case SDL_SCANCODE_N: return 0x31;
    case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_COMMA: return 0x33;
    case SDL_SCANCODE_PERIOD: return 0x34; case SDL_SCANCODE_SLASH: return 0x35;
    case SDL_SCANCODE_RSHIFT: return 0x36; case SDL_SCANCODE_LALT: return 0x38;
    case SDL_SCANCODE_SPACE: return 0x39; case SDL_SCANCODE_CAPSLOCK: return 0x3a;
    case SDL_SCANCODE_F1: return 0x3b; case SDL_SCANCODE_F2: return 0x3c;
    case SDL_SCANCODE_F3: return 0x3d; case SDL_SCANCODE_F4: return 0x3e;
    case SDL_SCANCODE_F5: return 0x3f; case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42;
    case SDL_SCANCODE_F9: return 0x43; case SDL_SCANCODE_F10: return 0x44;
    case SDL_SCANCODE_KP_7: return 0x47; case SDL_SCANCODE_KP_8: return 0x48;
    case SDL_SCANCODE_KP_9: return 0x49; case SDL_SCANCODE_KP_MINUS: return 0x4a;
    case SDL_SCANCODE_KP_4: return 0x4b; case SDL_SCANCODE_KP_5: return 0x4c;
    case SDL_SCANCODE_KP_6: return 0x4d; case SDL_SCANCODE_KP_PLUS: return 0x4e;
    case SDL_SCANCODE_KP_1: return 0x4f; case SDL_SCANCODE_KP_2: return 0x50;
    case SDL_SCANCODE_KP_3: return 0x51; case SDL_SCANCODE_KP_0: return 0x52;
    case SDL_SCANCODE_KP_PERIOD: return 0x53;
    /* arrow keys double as the keypad codes the game expects */
    case SDL_SCANCODE_UP: return 0x48; case SDL_SCANCODE_DOWN: return 0x50;
    case SDL_SCANCODE_LEFT: return 0x4b; case SDL_SCANCODE_RIGHT: return 0x4d;
    default: return 0;
    }
}
