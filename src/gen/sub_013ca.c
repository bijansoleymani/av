/* 0x013ca sub_013ca - draw edge/wall sprites when a clamped (x,y) point nears the four borders */
#include "dos.h"
#include "game_protos.h"

int sub_013ca(int x, int y)
{
    /* Shift the point and clamp it into the playfield rectangle. The clamped
     * coordinates are where the border sprites get drawn; the unclamped x/y
     * decide whether a given border is near enough to draw. */
    int cx = x - 4;
    int cy = y - 5;

    if (cx < 4)      cx = 4;
    if (cx > 0x110)  cx = 0x110;

    if (cy < 0xb)    cy = 0xb;
    if (cy > 0xa7)   cy = 0xa7;

    if (x < 8)                       /* near left edge -> post at x=0 */
        draw_sprite(0, cy, post_left_sprite);

    if (x > 0x117)                   /* near right edge -> post at x=0x138 */
        draw_sprite(0x138, cy, post_right_sprite);

    if (y < 0x11)                    /* near top edge -> wall strip at y=0xb */
        draw_sprite(cx, 0xb, IMG(0x9ba));   /* 0x9ba: horizontal edge/wall sprite */

    if (y > 0xab)                    /* near bottom edge -> wall strip at y=0xc7 */
        draw_sprite(cx, 0xc7, IMG(0x9ba));  /* 0x9ba: horizontal edge/wall sprite */

    return 0;
}
