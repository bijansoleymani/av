#!/usr/bin/env python3
"""Decode AV.DAT: 20 concatenated BGI getimage() buffers (CGA 320x200 4-color).
Chunk size list is taken from the loader sub_0043a/sub_004a3 in AV.EXE.
BGI getimage buffer = { word width(=x2-x1); word height(=y2-y1); pixel rows }.
Row bytes derived from loader math: bpr = (width + 4) >> 2 ; rows = height + 1.
Renders each sprite to a scaled PNG for visual confirmation."""
import struct, os

dat = open('/Users/bijan/src/av/AV.DAT','rb').read()
# order + destination global (DS offset) from the disassembly of sub_0043a/4a3
CHUNKS = [
 (70,0x9da),(70,0xa38),(26,0x9ba),
 (266,0x9ee),(266,0x9f6),(306,0x9fe),(306,0xa06),
 (266,0x994),(266,0x99c),(306,0x9a4),(306,0x9ac),
 (202,0x9b4),
 (326,0xa18),(326,0xa20),(326,0xa28),(326,0xa30),
 (182,0x966),(182,0x96e),(182,0x976),(182,0x97e),
]
assert sum(s for s,_ in CHUNKS)==len(dat), (sum(s for s,_ in CHUNKS),len(dat))
print(f"AV.DAT = {len(dat)} bytes across {len(CHUNKS)} chunks (matches file size)\n")

# CGA palette 1 (default high-intensity): 0=black,1=cyan,2=magenta,3=white
PAL=[(0,0,0),(85,255,255),(255,85,255),(255,255,255)]

os.makedirs('/Users/bijan/src/av/build/sprites', exist_ok=True)
off=0
print(f"{'#':>2} {'global':>6} {'size':>4} {'w':>3} {'h':>3} {'bpr':>3} {'rows':>4} {'calc':>5} ok")
sprites=[]
for i,(size,g) in enumerate(CHUNKS):
    b=dat[off:off+size]; off+=size
    w,h=struct.unpack_from('<HH',b,0)
    pixels=w+1
    rowbytes=(pixels*2+7)//8
    bpr=rowbytes+(rowbytes&1)          # word-align to even bytes
    rows=h+1
    calc=4+bpr*rows+2                   # 4-byte header + pixels + 2 trailing pad
    ok = 'OK' if calc==size else f'!={calc}'
    print(f"{i:2d} {g:6x} {size:4d} {w:3d} {h:3d} {bpr:3d} {rows:4d} {calc:5d} {ok}")
    sprites.append((i,g,w,h,bpr,rows,b))

# Render with best-fit bpr: use the derived bpr; decode 2bpp big-endian pixels.
try:
    from PIL import Image
    have_pil=True
except Exception:
    have_pil=False
    print("\n(PIL not installed; skipping PNG render)")

def render(name,w,h,bpr,rows,b):
    px_w=bpr*4
    img=Image.new('RGB',(px_w,rows),(30,30,30))
    pi=img.load()
    p=4
    for r in range(rows):
        for cb in range(bpr):
            if p>=len(b): break
            byte=b[p]; p+=1
            for k in range(4):
                pix=(byte>>(6-2*k))&3
                x=cb*4+k
                if x<px_w: pi[x,r]=PAL[pix]
    scale=6
    img=img.resize((px_w*scale,rows*scale),Image.NEAREST)
    img.save(f'/Users/bijan/src/av/build/sprites/{name}.png')

if have_pil:
    for i,g,w,h,bpr,rows,b in sprites:
        render(f'{i:02d}_{g:x}',w,h,bpr,rows,b)
    print("\nrendered PNGs to build/sprites/")
