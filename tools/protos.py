#!/usr/bin/env python3
"""Derive a best-effort C prototype (arg count, stack-frame base) for each game
function from its prologue and [bp+N] argument accesses. Emits build/protos.txt."""
import struct, re
from collections import defaultdict
from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_OP_MEM

data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; img=data[LOAD:0x88a0]
calltgt=defaultdict(list)
for i in range(len(img)-2):
    if img[i]==0xe8:
        rel=struct.unpack_from('<h',img,i+1)[0]; t=(i+3+rel)&0xffff
        if 0<=t<len(img): calltgt[t].append(i)
GAME_LO,GAME_HI=0x115,0x2474
game=sorted(set([t for t in calltgt if GAME_LO<=t<GAME_HI])|{0x1a5,0x115,0x155,0x171,0x185})
fstarts=game+[GAME_HI]
import importlib.util, os
# names from gen_ctx
NAMED={0x1a5:'game_main',0x300:'sub_300',0x345:'preshift_sprite',0x41a:'read_chunk',
 0x43a:'load_data',0x563:'init_sprites2',0x691:'draw_scene',0x724:'joy_calibrate',
 0x779:'menu_draw',0x9af:'play_round',0xbee:'setup_round',0x199b:'collide_check',
 0x22bf:'wait_vsync',0x22d0:'draw_sprite',0x23e2:'draw_sprite2',0xd50:'read_joystick',
 0xde9:'read_mouse',0x115:'_exit',0x155:'_startup_b',0x171:'_startup_c',0x185:'_startup_d'}
def nm(a): return NAMED.get(a, f'sub_{a:05x}')

md=Cs(CS_ARCH_X86,CS_MODE_16); md.detail=True
rows=[]
for f in game:
    if f<0x115: continue
    end=fstarts[fstarts.index(f)+1]
    # prologue: count regs pushed before 'push bp; mov bp,sp'
    off=f; pushed=0; has_frame=False
    ins=list(md.disasm(img[f:end], f))
    i=0
    while i<len(ins):
        m=ins[i].mnemonic; o=ins[i].op_str
        if m=='push' and o in ('si','di','ax','bx','cx','dx','es','ds'):
            pushed+=1; i+=1; continue
        if m=='push' and o=='bp' and i+1<len(ins) and ins[i+1].mnemonic=='mov' and ins[i+1].op_str.replace(' ','')=='bp,sp':
            has_frame=True
        break
    arg0 = 4 + 2*pushed if has_frame else None
    # scan max [bp+N] and whether args read as byte(al) etc
    maxN=None; uses=set()
    for insn in ins:
        for op in insn.operands:
            if op.type==CS_OP_MEM and op.mem.base in (0x23,) : pass  # not used
        mm=re.findall(r'\[bp \+ (0x[0-9a-f]+|\d+)\]', insn.op_str)
        for x in mm:
            v=int(x,16) if x.startswith('0x') else int(x)
            if arg0 is not None and v>=arg0:
                maxN = v if maxN is None else max(maxN,v)
    if arg0 is not None and maxN is not None:
        nargs=(maxN-arg0)//2 + 1
    elif arg0 is not None:
        nargs=0
    else:
        nargs='?'
    rows.append((nm(f), f, end-f, len(calltgt.get(f,[])), pushed, has_frame, nargs))

with open('/Users/bijan/src/av/build/protos.txt','w') as fh:
    fh.write("name / addr / size / callers / regs_saved / has_frame / est_args\n")
    for r in rows:
        fh.write(f"{r[0]:16} 0x{r[1]:05x} sz={r[2]:<4} callers={r[3]:<3} saved={r[4]} frame={int(r[5])} args~{r[6]}\n")
print(open('/Users/bijan/src/av/build/protos.txt').read())
