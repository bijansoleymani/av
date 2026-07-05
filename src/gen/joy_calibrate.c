/* 0x00724 joy_calibrate - measure joystick game-port axis pulse timing and set joy_val/joy_timeout */
#include "dos.h"
#include "game_protos.h"

/* Strobes the game port (0x201) to fire the axis one-shot, then times how long
 * the axis line's bit-0 stays high.  The original does raw out/in on port 0x201;
 * here we start the one-shot with the 0x7f strobe (a no-op in this model) and
 * sample bit 0 of the axis line via joy_read_button(0) (returns 0/1).
 *
 * Returns 1 and records the pulse width in joy_val (and sets joy_timeout to
 * 1.5x the width) on success; returns 0 if the measurement fails. */
int joy_calibrate(void)
{
    int count;   /* timing counter (signed 16-bit) */

    joy_timeout = 0x7d00;
    count = 0;

    /* strobe the game port to start the one-shot conversion */
    (void)0x7f;

    /* Wait for the axis line (bit 0) to go high, bounded by joy_timeout. */
    while ((joy_read_button(0) & 1) == 0 && count < joy_timeout)
        count++;

    /* If the wait counter went negative, calibration failed. */
    if (count < 0)
        return 0;

    /* Measure how long the axis line stays high, bounded by joy_timeout. */
    count = 0;
    while ((joy_read_button(0) & 1) != 0 && count < joy_timeout)
        count++;

    /* A negative width means failure; otherwise record the result. */
    if (count < 0)
        return 0;

    joy_val = count;
    /* joy_timeout = count + count/2  (arithmetic >>, 16-bit wraparound) */
    joy_timeout = (short)((count >> 1) + count);
    return 1;
}
