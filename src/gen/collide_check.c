/* 0x0199b collide_check - test entity X vs target; set entity velocity, return hit flag */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int collide_check(int a0, int a1, int a2)
{
    dsptr si;           /* per-entity struct ptr, stride 6 */
    int di;             /* entity X position */
    int ax;

    /* si = a0*6 + 0x9c8 */
    si = (dsptr)((unsigned)(a0 * 6) + 0x9c8);          /* 019a0..019aa */

    /* di = W(0x98c + a0*2)  (entity X array; a0==0 => ball_x) */
    di = W((unsigned)((a0 << 1) & 0xffff) + 0x98c);    /* 019ae..019b3 */

    ax = (short)((di - a1) & 0xffff);                  /* 019b7..019bc */
    if (ax < 0)                                        /* jge 0x19c2 */
        ax = (short)((-ax) & 0xffff);                  /* 019c0 neg */

    if (ax < a2)                                       /* cmp ax,a2 ; jge 0x19d5 */
        goto L19c7;
    goto L19d5;

L19c7:
    W(si) = 0;                                         /* 019c7 */
    W((dsptr)(si + 2)) = 0;                            /* 019cb */
    ax = 1;                                            /* 019d0 */
    goto L19f0;                                        /* 019d3 */

L19d5:
    if (di < a1)                                       /* cmp di,a1 ; jge 0x19e5 */
        goto L19da;
    goto L19e5;

L19da:
    W(si) = 0;                                         /* 019da */
    W((dsptr)(si + 2)) = 2;                            /* 019de */
    goto L19ee;                                        /* 019e3 */

L19e5:
    W(si) = (short)0xfffe;                             /* 019e5  -2 */
    W((dsptr)(si + 2)) = 0;                            /* 019e9 */

L19ee:
    ax = 0;                                            /* 019ee xor ax,ax */

L19f0:
    return ax;                                         /* 019f0..019f3 */
}
