#!/usr/bin/env python3
"""Generate the decompilation context package:
  build/ds_init.bin        exact DGROUP initialized image (DS 0..0x966)
  build/symbols.txt        symbol table (globals, funcs, libs, sprite slots)
  build/dis/<name>.txt     per-function symbolized disassembly
  build/all_funcs.dis      all game functions concatenated
"""
import struct, re, os
from collections import defaultdict
from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_OP_MEM, CS_OP_IMM

data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; CODE_END=0x88a0; img=data[LOAD:CODE_END]
DGROUP_FO=0x88a0; DS_INIT=data[DGROUP_FO:]        # 0x966 bytes initialized
os.makedirs('/Users/bijan/src/av/build/dis', exist_ok=True)
open('/Users/bijan/src/av/build/ds_init.bin','wb').write(DS_INIT)

strtab={}
for m in re.finditer(rb'[\x20-\x7e]{3,}', DS_INIT):
    strtab[m.start()]=m.group().decode()

# ---- near-call targets -> function starts ----
calltgt=defaultdict(list)
for i in range(len(img)-2):
    if img[i]==0xe8:
        rel=struct.unpack_from('<h',img,i+1)[0]; t=(i+3+rel)&0xffff
        if 0<=t<len(img): calltgt[t].append(i)

# Named RTL / library far+near routines (target offset -> name, is this a call we keep as lib)
LIB = {
 0x115:'_exit', 0x155:'_startup_b', 0x171:'_startup_c', 0x185:'_startup_d',
 0x44b8:'malloc', 0x45f4:'sys_open', 0x464e:'sys_read', 0x3cbd:'sys_close',
 0x3e36:'printf', 0x402f:'exit', 0x420c:'registerbgidriver', 0x466c:'movedata',
 0x4230:'int86', 0x3e83:'bios10', 0x42f2:'_ioerror', 0x4b5e:'_vprintf', 0x3cdf:'_write',
 0x432b:'sub_432b',  # small helper (used by play loop)
 # BGI far (segment 0) library:
 0x5aa2:'initgraph', 0x539d:'graphresult', 0x5ce3:'lib_5ce3',
 0x6035:'setfillstyle', 0x6a19:'bar', 0x6324:'imagesize', 0x6d73:'getimage',
 0x6c42:'putimage', 0x63f4:'outtext', 0x5fca:'lib_5fca', 0x5d80:'lib_5d80',
 0x6129:'lib_6129', 0x6b04:'lib_6b04', 0x5409:'lib_5409',
}
# Named game functions
NAMED = {
 0x1a5:'game_main', 0x300:'sub_300', 0x345:'preshift_sprite', 0x41a:'read_chunk',
 0x43a:'load_data', 0x563:'init_sprites2', 0x691:'draw_scene', 0x724:'joy_calibrate',
 0x779:'menu_draw', 0x9af:'play_round', 0xbee:'setup_round',
 0x199b:'collide_check', 0x22bf:'wait_vsync', 0x22d0:'draw_sprite', 0x23e2:'draw_sprite2',
 0xd50:'read_joystick', 0xde9:'read_mouse',
}

GAME_LO,GAME_HI=0x115,0x2474
game_funcs=sorted(set([t for t in calltgt if GAME_LO<=t<GAME_HI]) | {0x1a5,0x115,0x155,0x171,0x185})
fstarts=game_funcs+[GAME_HI]

def fname(a):
    if a in NAMED: return NAMED[a]
    if a in LIB: return LIB[a]
    return f'sub_{a:05x}'

# Global naming (scalar DS globals). Sprite-pointer slots handled specially by IMG().
SPRITE_SLOTS=set()
for base in (0x966,0x994,0x9ee,0xa18):
    for grp in range(4):
        for sh in range(4):
            SPRITE_SLOTS.add(base+grp*8+sh*2)
for s in (0x9b4,0x9ba,0x9da,0xa38):
    SPRITE_SLOTS.add(s)
GNAME={
 0x98c:'ball_x',0x98e:'ball_y',0x9e4:'p1_x',0x9e6:'p2_x',
 0x9d6:'server',0x9d8:'side_swap',0x9e2:'hit_count',
 0x9be:'p1_frame',0x9c0:'p2_frame',0x9c2:'font_img',0xa50:'dat_fd',
 0xa42:'joy_timeout',0xa44:'joy_val',0x24e:'quit_flag',
}
def gname(d):
    if d in SPRITE_SLOTS: return f'IMG(0x{d:x})'
    if d in GNAME: return GNAME[d]
    if d in strtab: return f'STR(0x{d:x})'
    return f'g_{d:04x}'

md=Cs(CS_ARCH_X86, CS_MODE_16); md.detail=True

def dis_func(start,end):
    lines=[]
    off=start
    while off<end:
        try: insn=next(md.disasm(img[off:off+16], off))
        except StopIteration: break
        txt=f"{insn.mnemonic} {insn.op_str}".strip()
        ann=[]
        # symbolize call targets
        if insn.mnemonic in('call',) and insn.operands and insn.operands[0].type==CS_OP_IMM:
            ann.append('-> '+fname(insn.operands[0].imm))
        if insn.mnemonic=='lcall':
            m=re.search(r'0x([0-9a-f]+)$', insn.op_str)
            if m: ann.append('-> '+fname(int(m.group(1),16)))
        # symbolize [imm16] globals + string refs
        for op in insn.operands:
            if op.type==CS_OP_MEM and op.mem.base==0 and op.mem.index==0:
                d=op.mem.disp & 0xffff
                nm=gname(d)
                if d in strtab: ann.append(f"[0x{d:x}]={nm} \"{strtab[d][:22]}\"")
                elif not nm.startswith('g_'): ann.append(f"[0x{d:x}]={nm}")
        a=("   ; "+", ".join(ann)) if ann else ""
        lines.append(f"  {insn.address:05x}: {txt}{a}")
        if insn.mnemonic in('ret','retf','iret') and off+insn.size>=end: break
        off+=insn.size
    return lines

# emit per-function + combined
allf=[]
for idx,f in enumerate([x for x in game_funcs if x>=0x115]):
    end=fstarts[fstarts.index(f)+1]
    nm=fname(f)
    hdr=f"; ==== {nm}  (0x{f:05x}..0x{end:05x}, {end-f} bytes, {len(calltgt.get(f,[]))} callers) ===="
    body=dis_func(f,end)
    block=hdr+"\n"+"\n".join(body)+"\n"
    allf.append(block)
    with open(f'/Users/bijan/src/av/build/dis/{nm}.txt','w') as fh:
        fh.write(block)
open('/Users/bijan/src/av/build/all_funcs.dis','w').write("\n".join(allf))

# symbol table
with open('/Users/bijan/src/av/build/symbols.txt','w') as fh:
    fh.write("FUNCTIONS (game + startup):\n")
    for f in [x for x in game_funcs if x>=0x115]:
        end=fstarts[fstarts.index(f)+1]
        fh.write(f"  {fname(f):16} 0x{f:05x} sz={end-f:<4} callers={len(calltgt.get(f,[]))}\n")
    fh.write("\nNAMED SCALAR GLOBALS:\n")
    for d,n in sorted(GNAME.items()): fh.write(f"  0x{d:04x} {n}\n")
    fh.write("\nSPRITE SLOTS (IMG): "+ " ".join(f"0x{s:x}" for s in sorted(SPRITE_SLOTS))+"\n")

print("wrote build/dis/*, build/all_funcs.dis, build/symbols.txt, build/ds_init.bin")
print("game functions:", len([x for x in game_funcs if x>=0x115]))
print("total game bytes:", GAME_HI-0x115)
