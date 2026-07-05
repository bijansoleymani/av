/* 0x0d50 read_joystick - poll one player's game-port joystick: store button + X-axis direction bits */
#include "dos.h"
#include "game_protos.h"

/* Reads the game port (0x201) for one player and writes the three control
 * words used by the game logic.  The original does raw in/out on port 0x201;
 * bit 4 (0x10) is the button (active-low) and bit 0 is the X-axis one-shot
 * line.  We read those bits via joy_read_button(player):  &0x10 -> button,
 * &1 -> axis line level (0/1).
 *
 * The two busy-loops measure the axis one-shot pulse width exactly as the
 * binary does.  The first loop waits out the initial LOW phase and its count
 * is discarded; only the second loop (HIGH-pulse width) drives the axis and
 * timeout results.  Both loops are capped at joy_timeout, and every "cmp
 * count,joy_timeout ; jge" is a signed 16-bit compare. */
int read_joystick(int player)
{
    int count;        /* axis timing counter (signed 16-bit) */
    int button;       /* jump value stored for this player */
    int axis;         /* right-direction value stored for this player */
    int timed_out;    /* left-direction value stored for this player */

    /* Button: active-low, bit 0x10.  Pressed (bit clear) -> jump = 1. */
    button = (joy_read_button(player) & 0x10) ? 0 : 1;
    ctrl_jump(player) = (short)button;

    /* Strobe the game port to start the axis one-shot (out 0x201,0xff);
     * no observable effect in this model. */

    /* Wait out the initial LOW phase of the axis line (count discarded). */
    for (count = 0;
         (joy_read_button(player) & 1) == 0 && count < joy_timeout;
         count++)
        ;

    /* Measure the HIGH pulse width, capped at joy_timeout. */
    for (count = 0;
         (joy_read_button(player) & 1) != 0 && count < joy_timeout;
         count++)
        ;

    /* Axis threshold: pulse shorter than half the calibrated value counts as
     * a deflection.  (joy_val >> 1) is a signed arithmetic shift; result is
     * {0, 2} stored as the right-direction control word. */
    axis = (((short)joy_val >> 1) >= count) ? 0 : 1;
    axis = (short)((unsigned)axis << 1);      /* 0 or 2 */
    ctrl_right(player) = (short)axis;

    /* Timeout flag: if the HIGH phase never reached joy_timeout, encode a
     * left deflection.  1 -> neg -> -1 -> <<1 -> -2; result is {0, -2}. */
    timed_out = (count >= joy_timeout) ? 0 : 1;
    timed_out = (short)(-timed_out);
    timed_out = (short)((unsigned)timed_out << 1);   /* 0 or -2 */
    ctrl_left(player) = (short)timed_out;

    return timed_out;    /* AX at ret = last stored value {0, -2} */
}
