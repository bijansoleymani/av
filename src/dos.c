/* dos.c — the data segment, video memory, and small-model heap allocator. */
#include "dos.h"
#include "ds_init.h"
#include <string.h>

unsigned char DS[0x10000];
unsigned char VIDEO[0x4000];

/* The original malloc carves from the DGROUP heap just past BSS.  We reproduce a
 * simple bump allocator over DS[] starting above the game's BSS end (0xb75).
 * Sprites and captured images live here as dsptr offsets — exactly as in the
 * 16-bit build, so a "pointer" stored in a global is just an offset into DS. */
#define HEAP_START 0x0c00u
static uint16_t heap_top = HEAP_START;

void dos_data_init(void)
{
    memset(DS, 0, sizeof DS);
    memcpy(DS, DS_INIT, DS_INIT_LEN);      /* strings, menu tables, key defaults */
    heap_top = HEAP_START;
}

dsptr dos_malloc(uint16_t n)
{
    dsptr p;
    n = (n + 1) & ~1u;                     /* word align, like the RTL */
    if ((uint32_t)heap_top + n >= 0xF000u) /* leave room; never happens in AV */
        return 0;
    p = heap_top;
    heap_top += n;
    return p;
}

void movedata_ds(dsptr src, dsptr dst, uint16_t n)
{
    memmove(DS + dst, DS + src, n);
}
