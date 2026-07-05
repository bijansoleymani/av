/* 0x01121 sub_01121 - ball/player collision + wall/net bounce resolution */
#include "dos.h"
#include "game_protos.h"
#define IMG(o) UW(o)

int sub_01121(void)
{
    /* locals: [bp-2] loc2, [bp-4] loc4, [bp-6] loc6 */
    int loc2, loc4, loc6;
    int si;          /* loop / side index */
    int di;          /* scratch (dx-like distance) */
    int ax, dx, bx;
    unsigned int uax;        /* for unsigned mul */
    int cl;

    si = 0;                                   /* xor si,si */
    goto L125c;                               /* jmp 0x125c */

L112e:
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    di = W(0x9c6);                            /* mov di,[0x9c6] */
    ax = W(0x98c + (unsigned)bx);             /* mov ax,[bx+0x98c] */
    di = (short)(di - ax);                    /* sub di,ax */
    ax = si;                                  /* mov ax,si */
    dx = 7;                                    /* mov dx,7 */
    ax = (short)(ax * dx);                    /* mul dx (low 16) */
    di = (short)(di - ax);                    /* sub di,ax */
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    ax = W(0x98a);                            /* mov ax,[0x98a] */
    dx = W(0x9e4 + (unsigned)bx);             /* mov dx,[bx+0x9e4] */
    ax = (short)(ax - dx);                    /* sub ax,dx */
    ax = ax >> 1;                             /* sar ax,1 */
    loc6 = ax;                                /* mov [bp-6],ax */
    ax = di;                                  /* mov ax,di */
    ax = ax >> 1;                             /* sar ax,1 */
    ax = ax >> 1;                             /* sar ax,1 */
    uax = (unsigned short)((unsigned short)ax * (unsigned short)di); /* mul di -> ax (low) */
    /* push ax */
    {
        unsigned int prod_di = uax;
        uax = (unsigned short)((unsigned short)loc6 * (unsigned short)loc6); /* mul [bp-6] */
        dx = (short)(prod_di + uax);          /* pop dx(=prod_di); add dx,ax */
    }
    loc4 = dx;                                /* mov [bp-4],dx */
    if ((short)dx < 0x6e) goto L1174;         /* cmp dx,0x6e; jl 0x1174 */
    goto L1243;                               /* jmp 0x1243 */

L1174:
    ax = hit_count;                           /* mov ax,[0x9e2] */
    ax = ax & 0xf;                            /* and ax,0xf */
    dx = 8;                                    /* mov dx,8 */
    dx = (short)(dx - ax);                    /* sub dx,ax */
    loc2 = dx;                                /* mov [bp-2],dx */
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    if (W(0x9e8 + (unsigned)bx) <= -1) goto L11ba; /* cmp [bx+0x9e8],-1; jle */
    ax = W(0x9b6);                            /* mov ax,[0x9b6] */
    if (ax >= 0) goto L1196;                  /* or ax,ax; jge 0x1196 */
    ax = (short)(-ax);                        /* neg ax */
L1196:
    ax = (short)(-ax);                        /* neg ax */
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    bx = W(0x9e8 + (unsigned)bx);             /* mov bx,[bx+0x9e8] */
    bx = bx << 1;                             /* shl bx,1 */
    dx = W(0x264 + (unsigned)bx);             /* mov dx,[bx+0x264] */
    bx = 3;                                    /* mov bx,3 */
    cl = W(0x9bc);                            /* mov cx,[0x9bc] */
    bx = bx << (cl & 0x1f);                   /* shl bx,cl */
    cl = bx;                                   /* mov cx,bx */
    dx = dx << (cl & 0x1f);                   /* shl dx,cl */
    ax = (short)(ax + dx);                    /* add ax,dx */
    W(0x9b6) = ax;                            /* mov [0x9b6],ax */
    goto L11c8;                               /* jmp 0x11c8 */

L11ba:
    ax = W(0x9b6);                            /* mov ax,[0x9b6] */
    if (ax >= 0) goto L11c3;                  /* or ax,ax; jge 0x11c3 */
    ax = (short)(-ax);                        /* neg ax */
L11c3:
    ax = (short)(-ax);                        /* neg ax */
    W(0x9b6) = ax;                            /* mov [0x9b6],ax */

L11c8:
    ax = loc2;                                /* mov ax,[bp-2] */
    W(0x9b6) = (short)(W(0x9b6) + ax);        /* add [0x9b6],ax */
    ax = di;                                  /* mov ax,di */
    if (ax >= 0) goto L11d7;                  /* or ax,ax; jge 0x11d7 */
    ax = (short)(-ax);                        /* neg ax */
L11d7:
    uax = (unsigned short)((unsigned short)ax * (unsigned short)di); /* mul di -> ax */
    {
        unsigned int t_push1 = uax;           /* push ax */
        ax = si;                              /* mov ax,si */
        dx = 6;                               /* mov dx,6 */
        ax = (short)(ax * dx);                /* mul dx (low) */
        bx = ax;                              /* mov bx,ax */
        ax = W(0x9ca + (unsigned)bx);         /* mov ax,[bx+0x9ca] */
        {
            unsigned int t_push2 = (unsigned short)ax; /* push ax */
            ax = si;                          /* mov ax,si */
            dx = 6;                           /* mov dx,6 */
            ax = (short)(ax * dx);            /* mul dx (low) */
            bx = ax;                          /* mov bx,ax */
            ax = (short)t_push2;              /* pop ax */
            ax = (short)(ax + W(0x9c8 + (unsigned)bx)); /* add ax,[bx+0x9c8] */
            cl = W(0x9bc);                    /* mov cx,[0x9bc] */
            cl = (short)(cl + 4);             /* add cx,4 */
            ax = ax << (cl & 0x1f);           /* shl ax,cl */
            dx = (short)t_push1;              /* pop dx */
            dx = (short)(dx + ax);            /* add dx,ax */
            dx = (short)(dx + loc2);          /* add dx,[bp-2] */
            W(0x988) = (short)(W(0x988) + dx);/* add [0x988],dx */
        }
    }
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    if (W(0x9de + (unsigned)bx) != 0) goto L125b; /* cmp [bx+0x9de],0; jne */
    W(0xa4d) = 0xc8;                          /* mov [0xa4d],0xc8 */
    W(0xa4b) = 0;                             /* mov [0xa4b],0 */
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    W(0x9de + (unsigned)bx) = 1;              /* mov [bx+0x9de],1 */
    ax = server;                              /* mov ax,[0x9d6] */
    if (ax == si) goto L123d;                 /* cmp ax,si; je 0x123d */
    server = si;                              /* mov [0x9d6],si */
    W(0x9dc) = 0;                             /* mov [0x9dc],0 */
    goto L125b;                               /* jmp 0x125b */
L123d:
    W(0x9dc) = (short)(W(0x9dc) + 1);         /* inc [0x9dc] */
    goto L125b;                               /* jmp 0x125b */

L1243:
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    if (W(0x9de + (unsigned)bx) == 0) goto L125b; /* cmp [bx+0x9de],0; je */
    ax = 0;                                    /* xor ax,ax */
    W(0x9bc) = ax;                            /* mov [0x9bc],ax */
    bx = si << 1;                             /* mov bx,si; shl bx,1 */
    W(0x9de + (unsigned)bx) = ax;             /* mov [bx+0x9de],ax */

L125b:
    si = (short)(si + 1);                     /* inc si */
L125c:
    if (si >= 2) goto L1264;                  /* cmp si,2; jge 0x1264 */
    goto L112e;                               /* jmp 0x112e */

L1264:
    si = 1;                                   /* mov si,1 */
    if (W(0x98a) <= 0x5b) goto L12bb;         /* cmp [0x98a],0x5b; jle */
    if (W(0xa3a) >= 0x80) goto L1295;         /* cmp [0xa3a],0x80; jge */
    if (W(0x9c6) <= 0x7f) goto L1295;         /* cmp [0x9c6],0x7f; jle */
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L1286;                  /* or ax,ax; jge 0x1286 */
    ax = (short)(-ax);                        /* neg ax */
L1286:
    ax = (short)(-ax);                        /* neg ax */
    ax = ax >> 1;                             /* sar ax,1 */
    W(0x988) = ax;                            /* mov [0x988],ax */
    W(0xa40) = 0x1fc0;                        /* mov [0xa40],0x1fc0 */
    goto L12b9;                               /* jmp 0x12b9 */

L1295:
    if (W(0xa3a) <= 0x9f) goto L12bb;         /* cmp [0xa3a],0x9f; jle */
    if (W(0x9c6) >= 0xa0) goto L12bb;         /* cmp [0x9c6],0xa0; jge */
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L12ae;                  /* or ax,ax; jge 0x12ae */
    ax = (short)(-ax);                        /* neg ax */
L12ae:
    ax = ax >> 1;                             /* sar ax,1 */
    W(0x988) = ax;                            /* mov [0x988],ax */
    W(0xa40) = 0x2800;                        /* mov [0xa40],0x2800 */
L12b9:
    si = 0;                                    /* xor si,si */
L12bb:
    if (si != 0) goto L12c2;                   /* or si,si; jne 0x12c2 */
    goto L13c4;                                /* jmp 0x13c4 */
L12c2:
    if (W(0x98a) > 0x51) goto L12cc;           /* cmp [0x98a],0x51; jg */
    goto L13c4;                                /* jmp 0x13c4 */
L12cc:
    if (W(0x9c6) > 0x7f) goto L12d6;           /* cmp [0x9c6],0x7f; jg */
    goto L13c4;                                /* jmp 0x13c4 */
L12d6:
    if (W(0x9c6) < 0xa0) goto L12e1;           /* cmp [0x9c6],0xa0; jl */
    goto L13c4;                                /* jmp 0x13c4 */
L12e1:
    if (W(0x98a) <= 0x5b) goto L1310;          /* cmp [0x98a],0x5b; jle */
    if (W(0x9c6) >= 0x94) goto L1301;          /* cmp [0x9c6],0x94; jge */
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L12f9;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L12f9:
    ax = (short)(-ax);                         /* neg ax */
    W(0x988) = ax;                            /* mov [0x988],ax */
    goto L13c4;                                /* jmp 0x13c4 */
L1301:
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L130a;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L130a:
    W(0x988) = ax;                            /* mov [0x988],ax */
    goto L13c4;                                /* jmp 0x13c4 */

L1310:
    if (W(0x9c6) <= 0x93) goto L1330;          /* cmp [0x9c6],0x93; jle */
    bx = 0x5b;                                 /* mov bx,0x5b */
    bx = (short)(bx - W(0x98a));               /* sub bx,[0x98a] */
    bx = bx << 1;                              /* shl bx,1 */
    ax = W(0x250 + (unsigned)bx);              /* mov ax,[bx+0x250] */
    dx = 0xa1;                                 /* mov dx,0xa1 */
    dx = (short)(dx - W(0x9c6));               /* sub dx,[0x9c6] */
    if (ax <= dx) goto L1357;                  /* cmp ax,dx; jle 0x1357 */
L1330:
    if (W(0x9c6) < 0x94) goto L133b;           /* cmp [0x9c6],0x94; jl */
    goto L13c4;                                /* jmp 0x13c4 */
L133b:
    bx = 0x5b;                                 /* mov bx,0x5b */
    bx = (short)(bx - W(0x98a));               /* sub bx,[0x98a] */
    bx = bx << 1;                              /* shl bx,1 */
    ax = W(0x250 + (unsigned)bx);              /* mov ax,[bx+0x250] */
    dx = W(0x9c6);                             /* mov dx,[0x9c6] */
    dx = (short)(dx + 0xff7b);                 /* add dx,0xff7b (-133) */
    if (ax <= dx) goto L1357;                  /* cmp ax,dx; jle 0x1357 */
    goto L13c4;                                /* jmp 0x13c4 */

L1357:
    if (W(0x9b6) <= 0) goto L1398;             /* cmp [0x9b6],0; jle */
    di = W(0x9c6);                             /* mov di,[0x9c6] */
    di = (short)(di + 0xff6f);                 /* add di,0xff6f (-145) */
    if (di >= -5) goto L1379;                  /* cmp di,-5; jge */
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L1374;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L1374:
    ax = (short)(-ax);                         /* neg ax */
    W(0x988) = ax;                            /* mov [0x988],ax */
L1379:
    if (di <= 5) goto L138a;                   /* cmp di,5; jle */
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L1387;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L1387:
    W(0x988) = ax;                            /* mov [0x988],ax */
L138a:
    ax = W(0x9b6);                            /* mov ax,[0x9b6] */
    if (ax >= 0) goto L1393;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L1393:
    ax = (short)(-ax);                         /* neg ax */
    W(0x9b6) = ax;                            /* mov [0x9b6],ax */

L1398:
    ax = W(0x988);                            /* mov ax,[0x988] */
    if (ax >= 0) goto L13a1;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L13a1:
    if (ax <= 0x20) goto L13ae;                /* cmp ax,0x20; jle */
    ax = W(0x988);                            /* mov ax,[0x988] */
    ax = ax >> 1;                             /* sar ax,1 */
    W(0x988) = ax;                            /* mov [0x988],ax */
L13ae:
    ax = W(0x9b6);                            /* mov ax,[0x9b6] */
    if (ax >= 0) goto L13b7;                   /* or ax,ax; jge */
    ax = (short)(-ax);                         /* neg ax */
L13b7:
    if (ax <= 0x20) goto L13c4;                /* cmp ax,0x20; jle */
    ax = W(0x9b6);                            /* mov ax,[0x9b6] */
    ax = ax >> 1;                             /* sar ax,1 */
    W(0x9b6) = ax;                            /* mov [0x9b6],ax */

L13c4:
    return 0;                                  /* mov sp,bp; pop bp/di/si; ret */
}
