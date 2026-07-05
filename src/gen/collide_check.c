/* 0x0199b collide_check - test entity X vs target; set entity velocity, return hit flag */
#include "dos.h"
#include "game_protos.h"

int collide_check(int entity, int target_x, int reach)
{
    /* entity==0 addresses the ball; player_x(0) aliases ball_x. */
    int entity_x = player_x(entity);
    /* distance is computed in 16 bits (short) so the subtraction and the
     * negation wrap exactly as the original code did */
    short dist = entity_x - target_x;
    if (dist < 0)
        dist = -dist;

    if (dist < reach) {
        /* within reach: stop the entity and report a hit */
        ctrl_left(entity)  = 0;
        ctrl_right(entity) = 0;
        return 1;
    }

    /* out of reach: steer toward the target */
    if (entity_x < target_x) {
        ctrl_left(entity)  = 0;
        ctrl_right(entity) = 2;
    } else {
        ctrl_left(entity)  = -2;
        ctrl_right(entity) = 0;
    }
    return 0;
}
