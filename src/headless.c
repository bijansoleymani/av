/* headless.c — validate the data/render pipeline with no display: load the real
 * AV.DAT, draw sprites into VIDEO[], and dump the CGA framebuffer to a PPM. */
#include "dos.h"
#include "game_protos.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define IMG(off) UW(off)

/* platform stubs so we can link without SDL */
void platform_pump(void) {}
void platform_shutdown(void) {}
int  platform_int33(dsptr in, dsptr out){ (void)in;(void)out; return 0; }
void cga_init(void){}
int  kb_scan_pressed(int s){ (void)s; return 0; }
int  joy_read_button(int p){ (void)p; return 0; }
int  joy_read_axis(int p,int*t){ (void)p;(void)t; return 0; }
int  mouse_read(int*b,int*x,int*y){ (void)b;(void)x;(void)y; return 0; }
void speaker_tone(int d){ (void)d; }
void speaker_off(void){}

static const unsigned CGA_PAL[4] = {0x000000,0x55FFFF,0xFF55FF,0xFFFFFF};

static void dump_ppm(const char *path)
{
    FILE *f = fopen(path,"wb");
    int y,x; fprintf(f,"P6\n320 200\n255\n");
    for (y=0;y<200;y++){
        const unsigned char *row = VIDEO + ((y>>1)*80) + ((y&1)?0x2000:0);
        for (x=0;x<320;x++){
            unsigned b=row[x>>2], p=(b>>(6-2*(x&3)))&3, c=CGA_PAL[p];
            unsigned char rgb[3]={ (c>>16)&0xff,(c>>8)&0xff,c&0xff };
            fwrite(rgb,1,3,f);
        }
    }
    fclose(f);
}

int main(void)
{
    dos_data_init();
    if (!load_data()){ fprintf(stderr,"load_data failed\n"); return 1; }
    memset(VIDEO,0,sizeof VIDEO);

    int f;
    for (f=0;f<4;f++) draw_sprite(10+f*45, 20,  IMG(0x994 + f*8));  /* player A */
    for (f=0;f<4;f++) draw_sprite(10+f*45, 60,  IMG(0x9ee + f*8));  /* player B */
    for (f=0;f<4;f++) draw_sprite(10+f*45, 100, IMG(0xa18 + f*8));  /* ball     */
    for (f=0;f<4;f++) draw_sprite(10+f*45, 140, IMG(0x966 + f*8));  /* decor    */
    draw_sprite(0,   168, IMG(0x9da));   /* left post  */
    draw_sprite(300, 168, IMG(0xa38));   /* right post */
    draw_sprite(150, 80,  IMG(0x9b4));   /* net        */

    /* also show sub-pixel shifts of one frame to prove pre-shift works */
    for (f=0;f<4;f++) draw_sprite(40+f, 185, IMG(0xa18 + f*8));

    dump_ppm("build/render_test.ppm");
    printf("OK: rendered pipeline to build/render_test.ppm\n");
    return 0;
}
