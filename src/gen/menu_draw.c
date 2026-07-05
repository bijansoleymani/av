/* 0x00779 menu_draw - draw the options menu, run its input loop, apply +/-/enter, and clear on exit */
#include "dos.h"
#include "game_protos.h"

/* RTL / library helpers reached by this function.  Their addresses (0x432b,
 * 0x4104, 0x4b2b, 0x4b57) lie in the Turbo C runtime region above the game
 * code, so they are not decompiled game functions.  By usage:
 *   sub_0432b() -> AX flag : input poll (kbhit-style; nonzero when a key is up)
 *   sub_04104() -> AX      : blocking key read (getch-style), returns scancode/char
 *   sub_04b2b(int ms)      : delay(ms) busy-wait
 *   sub_04b57(void)        : delay/sound teardown paired with sub_04b2b
 * They are called here by their sub name to stay faithful to the bytes. */
extern int  sub_0432b(void);
extern int  sub_04104(void);
extern void sub_04b2b(int ms);
extern void sub_04b57(void);

/* The menu is an array of 7 rows, stride 0x3c bytes, based at DS offset 0xaa:
 *   row[i].value   = W(i*0x3c + 0xaa)   current choice index for the row
 *   row[i].nchoices= W(i*0x3c + 0xac)   number of selectable choices (0 => exit row)
 *   row[i] labels  = i*0x3c + 0xae      base of the value strings (each 0xe bytes)
 * The label string for the current value is at value*0xe + i*0x3c + 0xae.
 * No game.h names exist for these, so the raw W()/offsets are kept. */
#define ROW_VALUE(i)     W((unsigned)(i) * 0x3c + 0xaa)
#define ROW_NCHOICES(i)  W((unsigned)(i) * 0x3c + 0xac)
#define ROW_LABEL(i, v)  ((dsptr)((unsigned)(v) * 0xe + (unsigned)(i) * 0x3c + 0xae))

#define MENU_ROWS 7
#define TEXT_X0   0x68   /* highlight-strip / erase X coord (104)      */
#define TEXT_X1   0x70   /* value-label text X coord (112)             */
#define ROW_Y0    0x28   /* row Y base (40); row i is at y = i*8 + Y0   */

int menu_draw(void)
{
    int row;   /* currently selected menu row */
    int i;
    unsigned char key;

    /* Erase/prime all 7 menu-row strips. */
    for (i = 0; i < MENU_ROWS; i++)
        draw_sprite2(TEXT_X0, (i << 3) + ROW_Y0, font_img);

    wait_vsync();

    /* Draw each row's current-value label. */
    for (i = 0; i < MENU_ROWS; i++)
        bgi_outtextxy(TEXT_X1, (i << 3) + ROW_Y0, ROW_LABEL(i, ROW_VALUE(i)));

    /* Highlight the initially-selected row (row 0). */
    sub_0234e(TEXT_X0, ROW_Y0, font_img);

    row = 0;
    key = 0;

    for (;;) {
        /* Idle/serve animation; returns a key in AL (Enter ends the menu). */
        key = (unsigned char)(sub_0063f() & 0xff);
        if (key == 0xd) {
            /* Enter: advance the selected row's value, or exit if it has none. */
            if (ROW_NCHOICES(row) <= 0)
                break;

            ROW_VALUE(row) = ROW_VALUE(row) + 1;
            ROW_VALUE(row) = ROW_VALUE(row) % ROW_NCHOICES(row);

            /* Erase the old label and draw the new one. */
            draw_sprite2(TEXT_X0, (row << 3) + ROW_Y0, font_img);
            bgi_outtextxy(TEXT_X1, (row << 3) + ROW_Y0, ROW_LABEL(row, ROW_VALUE(row)));

            /* Re-highlight the selected row and continue. */
            sub_0234e(TEXT_X0, (row << 3) + ROW_Y0, font_img);
            continue;
        }

        /* Non-Enter: if another key is queued, read it (getch-style). */
        if (sub_0432b() != 0)
            key = (unsigned char)(sub_04104() & 0xff);

        sub_04b2b(0xfa0);
        sub_04b57();
        wait_vsync();

        /* Redraw the highlight strip for the current row. */
        sub_0234e(TEXT_X0, (row << 3) + ROW_Y0, font_img);

        /* Down ('2' / keypad-down): move to next row. */
        if (key == 0x32 || key == 0x50)
            row = (row + 1) % MENU_ROWS;

        /* Up ('8' / keypad-up): move to previous row. */
        if (key == 0x38 || key == 0x48)
            row = (row + 6) % MENU_ROWS;

        /* Re-highlight the (possibly new) selected row and loop. */
        sub_0234e(TEXT_X0, (row << 3) + ROW_Y0, font_img);
    }

    /* Exit: clear all 7 rows and return the selected row index. */
    for (i = 0; i < MENU_ROWS; i++)
        draw_sprite2(TEXT_X0, (i << 3) + ROW_Y0, font_img);

    return row;
}
