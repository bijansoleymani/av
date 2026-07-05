/* 0x00300 sub_300 - detect mouse driver via INT 33h (AX=0 reset), then hide cursor (AX=2); return 1 if present else 0 */
#include "dos.h"
#include "game_protos.h"

/* The original allocates a 16-byte REGS block as a stack local at [bp-0x10].
 * In the small model SS==DS, so that local is itself a DS offset; we model it
 * with a fixed DS scratch region.  Only word 0 (the AX register) is used. */
#define REG_BLK 0xffe0            /* 16-byte int86 register block in DS scratch */

int sub_300(void)
{
    dsptr r = REG_BLK;           /* lea ax,[bp-0x10] : &regs */

    W(r + 0) = 0;                /* mov word ptr [bp-0x10], 0  ; AX = 0 (reset) */
    dos_int86(0x33, r, r);       /* push &out; push &in; push 0x33; call int86  */

    if (W(r + 0) == 0)           /* cmp word ptr [bp-0x10], 0 ; jne 0x327       */
        return 0;                /* xor ax,ax ; jmp ret  (no mouse driver)      */

    W(r + 0) = 2;                /* mov word ptr [bp-0x10], 2  ; AX = 2 (hide)  */
    dos_int86(0x33, r, r);       /* call int86                                  */

    return 1;                    /* mov ax, 1                                   */
}
