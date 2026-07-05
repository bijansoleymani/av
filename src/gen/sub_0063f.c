/* 0x0063f sub_0063f - idle/serve animation loop; run frames until a key is hit, return that key char */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

/* Turbo C console RTL pair used by this loop (not game/BGI functions):
 *   sub_0432b == kbhit()  -> AX nonzero when a key is waiting
 *   sub_04104 == getch()  -> AL = the key character                    */
extern int sub_0432b(void);
extern int sub_04104(void);

int sub_0063f(void)
{
    int si, di;      /* saved regs used as ball_x/ball_y snapshots */
    int ax, bx, dx;  /* work registers */

L641:
    ax = sub_0432b();            /* 00641 call kbhit */
    if ((ax & 0xffff) == 0)      /* 00644 or ax,ax ; 00646 je 0x64b */
        goto L64b;
    goto L71e;                   /* 00648 jmp 0x71e */

L64b:
    hit_count = hit_count + 1;   /* 0064b inc [0x9e2] */

    bx = server;                 /* 0064f mov bx,[server] */
    bx = bx << 1;                /* 00653 shl bx,1 */
    if (W(0x9e8 + (unsigned short)bx) != -1)  /* 00655 cmp [bx+0x9e8],-1 ; 0065a jne */
        goto L683;

    /* server side scored/out marker was -1: flip the serving side */
    ax = server;                 /* 0065c mov ax,[server] */
    dx = 6;                      /* 0065f mov dx,6 */
    bx = (int)((unsigned)(ax & 0xffff) * (unsigned)(dx & 0xffff)); /* 00662 mul dx ; 00664 mov bx,ax */
    W(0x9cc + (unsigned short)bx) = 0;   /* 00666 mov [bx+0x9cc],0 */

    ax = 1;                      /* 0066c mov ax,1 */
    ax = (short)(ax - server);   /* 0066f sub ax,[server] */
    server = ax;                 /* 00673 mov [server],ax */
    dx = 6;                      /* 00676 mov dx,6 */
    bx = (int)((unsigned)(ax & 0xffff) * (unsigned)(dx & 0xffff)); /* 00679 mul dx ; 0067b mov bx,ax */
    W(0x9cc + (unsigned short)bx) = 1;   /* 0067d mov [bx+0x9cc],1 */

L683:
    sub_00e7a();                 /* 00683 call sub_00e7a (advance ball physics) */

    si = ball_x;                 /* 00686 mov si,[ball_x] */
    di = ball_y;                 /* 0068a mov di,[ball_y] */

    wait_vsync();                /* 0068e call wait_vsync */

    /* draw player 1 sprite */
    bx = p1_frame;               /* 00691 mov bx,[p1_frame] */
    bx = bx << 1;                /* 00695 shl bx,1 */
    bx = bx << 1;                /* 00697 shl bx,1 */
    bx = bx << 1;                /* 00699 shl bx,1  -> p1_frame*8 */
    ax = si;                     /* 0069b mov ax,si */
    ax = ax & 3;                 /* 0069d and ax,3 */
    ax = ax << 1;                /* 006a0 shl ax,1 */
    bx = bx + ax;                /* 006a2 add bx,ax */
    draw_sprite(si, p1_y, W(0x994 + (unsigned short)bx)); /* 006a4..006ad push img,p1_y,si; call draw_sprite */
                                 /* 006b0 add sp,6 */

    /* draw player 2 sprite */
    bx = p2_frame;               /* 006b3 mov bx,[p2_frame] */
    bx = bx << 1;                /* 006b7 shl bx,1 */
    bx = bx << 1;                /* 006b9 shl bx,1 */
    bx = bx << 1;                /* 006bb shl bx,1  -> p2_frame*8 */
    ax = di;                     /* 006bd mov ax,di */
    ax = ax & 3;                 /* 006bf and ax,3 */
    ax = ax << 1;                /* 006c2 shl ax,1 */
    bx = bx + ax;                /* 006c4 add bx,ax */
    draw_sprite(di, p2_y, W(0x9ee + (unsigned short)bx)); /* 006c6..006cf push img,p2_y,di; call draw_sprite */
                                 /* 006d2 add sp,6 */

    /* left net/pole top when ball near left edge */
    if (si >= 6)                 /* 006d5 cmp si,6 ; 006d8 jge 0x6ee */
        goto L6ee;
    ax = p1_y;                   /* 006de mov ax,[p1_y] */
    ax = (short)(ax + 0xfffe);   /* 006e1 add ax,0xfffe  (p1_y - 2) */
    draw_sprite(0, ax, IMG(0x9da)); /* 006da..006e8 push [0x9da], (p1_y-2), 0; call draw_sprite */
                                 /* 006eb add sp,6 */

L6ee:
    /* right net/pole top when ball near right edge */
    if (di <= 0x113)             /* 006ee cmp di,0x113 ; 006f2 jle 0x709 */
        goto L709;
    ax = p2_y;                   /* 006f8 mov ax,[p2_y] */
    ax = (short)(ax + 0xfffe);   /* 006fb add ax,0xfffe  (p2_y - 2) */
    draw_sprite(0x138, ax, IMG(0xa38)); /* 006f4..00703 push [0xa38], (p2_y-2), 0x138; call draw_sprite */
                                 /* 00706 add sp,6 */

L709:
    draw_sprite(0x9e, 0x67, IMG(0x9b4)); /* 00709..00715 push [0x9b4], 0x67, 0x9e; call draw_sprite */
                                 /* 00718 add sp,6 */
    goto L641;                   /* 0071b jmp 0x641 */

L71e:
    ax = sub_04104();            /* 0071e call getch -> AL */
    return ax;                   /* 00721 pop di ; 00722 pop si ; 00723 ret (AX) */
}
