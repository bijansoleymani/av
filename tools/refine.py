#!/usr/bin/env python3
"""Clean function detection: linear-decode the contiguous game code region
[0x115,0x2474) and collect CALL targets only from decoded instructions.
Regenerate build/dis/*, build/all_funcs.dis, build/symbols.txt, build/protos.txt."""
import struct, re, os
from collections import defaultdict
from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_OP_MEM, CS_OP_IMM, CS_AC_READ, CS_AC_WRITE

data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; img=data[LOAD:0x88a0]
DS_INIT=data[0x88a0:]
strtab={}
for m in re.finditer(rb'[\x20-\x7e]{3,}', DS_INIT): strtab[m.start()]=m.group().decode()
GAME_LO,GAME_HI=0x115,0x2474
md=Cs(CS_ARCH_X86,CS_MODE_16); md.detail=True

# linear decode, collect call targets + all instructions keyed by addr
ins_by_addr={}
calltgt=defaultdict(list)
for insn in md.disasm(img[GAME_LO:GAME_HI], GAME_LO):
    ins_by_addr[insn.address]=insn
    if insn.mnemonic=='call' and insn.operands and insn.operands[0].type==CS_OP_IMM:
        t=insn.operands[0].imm
        if GAME_LO<=t<GAME_HI: calltgt[t].append(insn.address)
entries=sorted(set(calltgt)|{0x1a5,0x115,0x155,0x171,0x185})
fstarts=entries+[GAME_HI]
print(f"{len(entries)} real functions")

NAMED={0x1a5:'game_main',0x300:'sub_300',0x345:'preshift_sprite',0x41a:'read_chunk',
 0x43a:'load_data',0x563:'init_sprites2',0x691:'draw_scene',0x724:'joy_calibrate',
 0x779:'menu_draw',0x9af:'play_round',0xbee:'setup_round',0x199b:'collide_check',
 0x22bf:'wait_vsync',0x22d0:'draw_sprite',0x23e2:'draw_sprite2',0xd50:'read_joystick',
 0xde9:'read_mouse',0x115:'_exit',0x155:'_startup_b',0x171:'_startup_c',0x185:'_startup_d'}
LIB={0x44b8:'malloc',0x45f4:'sys_open',0x464e:'sys_read',0x3cbd:'sys_close',0x3e36:'printf',
 0x402f:'exit',0x420c:'registerbgidriver',0x466c:'movedata',0x4230:'int86',0x3e83:'bios10',
 0x42f2:'_ioerror',0x4b5e:'_vprintf',0x3cdf:'_write',
 0x5aa2:'initgraph',0x539d:'graphresult',0x5ce3:'lib_5ce3',0x6035:'setfillstyle',0x6a19:'bar',
 0x6324:'imagesize',0x6d73:'getimage',0x6c42:'putimage',0x63f4:'outtext',0x5fca:'lib_5fca',
 0x5d80:'lib_5d80',0x6129:'lib_6129',0x6b04:'lib_6b04',0x5409:'lib_5409'}
def fname(a): return NAMED.get(a) or LIB.get(a) or f'sub_{a:05x}'

SPRITE_SLOTS=set()
for base in (0x966,0x994,0x9ee,0xa18):
    for grp in range(4):
        for sh in range(4): SPRITE_SLOTS.add(base+grp*8+sh*2)
for s in (0x9b4,0x9ba,0x9da,0xa38): SPRITE_SLOTS.add(s)
GNAME={0x98c:'ball_x',0x98e:'ball_y',0x9e4:'p1_y',0x9e6:'p2_y',0x9d6:'server',0x9d8:'side_swap',
 0x9e2:'hit_count',0x9be:'p1_frame',0x9c0:'p2_frame',0x9c2:'font_img',0xa50:'dat_fd',
 0xa42:'joy_timeout',0xa44:'joy_val',0x24e:'quit_flag'}
def gname(d):
    if d in SPRITE_SLOTS: return f'IMG(0x{d:x})'
    if d in GNAME: return GNAME[d]
    if d in strtab: return f'STR(0x{d:x})'
    return f'g_{d:04x}'

os.makedirs('/Users/bijan/src/av/build/dis', exist_ok=True)
# clear old dis
for fn in os.listdir('/Users/bijan/src/av/build/dis'): os.remove('/Users/bijan/src/av/build/dis/'+fn)

def dis_func(start,end):
    out=[]; off=start
    while off<end:
        insn=ins_by_addr.get(off)
        if insn is None: off+=1; continue
        txt=f"{insn.mnemonic} {insn.op_str}".strip(); ann=[]
        if insn.mnemonic=='call' and insn.operands and insn.operands[0].type==CS_OP_IMM:
            ann.append('-> '+fname(insn.operands[0].imm))
        if insn.mnemonic=='lcall':
            m=re.search(r'0x([0-9a-f]+)$', insn.op_str)
            if m: ann.append('-> '+fname(int(m.group(1),16)))
        for op in insn.operands:
            if op.type==CS_OP_MEM and op.mem.base==0 and op.mem.index==0:
                d=op.mem.disp&0xffff; nm=gname(d)
                if d in strtab: ann.append(f'[0x{d:x}]={nm} "{strtab[d][:22]}"')
                elif not nm.startswith('g_'): ann.append(f'[0x{d:x}]={nm}')
        out.append(f"  {insn.address:05x}: {txt}"+(("   ; "+", ".join(ann)) if ann else ""))
        off+=insn.size
    return out

allf=[]
for f in entries:
    end=fstarts[fstarts.index(f)+1]
    nm=fname(f)
    hdr=f"; ==== {nm}  (0x{f:05x}..0x{end:05x}, {end-f} bytes, {len(calltgt.get(f,[]))} callers) ===="
    block=hdr+"\n"+"\n".join(dis_func(f,end))+"\n"
    allf.append(block)
    open(f'/Users/bijan/src/av/build/dis/{nm}.txt','w').write(block)
open('/Users/bijan/src/av/build/all_funcs.dis','w').write("\n".join(allf))

# protos: frame + arg estimate
def proto(f,end):
    pushed=0; frame=False; i=f
    seq=[ins_by_addr[a] for a in sorted(x for x in ins_by_addr if f<=x<end)]
    k=0
    while k<len(seq):
        m=seq[k].mnemonic; o=seq[k].op_str
        if m=='push' and o in('si','di'): pushed+=1; k+=1; continue
        if m=='push' and o=='bp' and k+1<len(seq) and seq[k+1].mnemonic=='mov' and seq[k+1].op_str.replace(' ','')=='bp,sp':
            frame=True
        break
    arg0=4+2*pushed
    maxN=None
    for insn in seq:
        for x in re.findall(r'\[bp \+ (0x[0-9a-f]+|\d+)\]', insn.op_str):
            v=int(x,16) if x.startswith('0x') else int(x)
            if frame and v>=arg0: maxN=v if maxN is None else max(maxN,v)
    nargs=((maxN-arg0)//2+1) if (frame and maxN is not None) else (0 if frame else '?reg')
    return pushed,frame,nargs,arg0

with open('/Users/bijan/src/av/build/protos.txt','w') as fh:
    for f in entries:
        end=fstarts[fstarts.index(f)+1]
        p,fr,na,a0=proto(f,end)
        fh.write(f"{fname(f):16} 0x{f:05x} sz={end-f:<4} callers={len(calltgt.get(f,[])):<2} frame={int(fr)} arg0=bp+{a0} args~{na}\n")
print(open('/Users/bijan/src/av/build/protos.txt').read())
