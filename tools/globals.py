#!/usr/bin/env python3
"""Catalog every DS-relative direct global access ([imm16]) in the game code,
plus detect the per-player struct stride/base pattern. Output a symbol worksheet."""
import struct, re
from collections import defaultdict
from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_OP_MEM, CS_AC_READ, CS_AC_WRITE

data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; CODE_END=0x88a0; img=data[LOAD:CODE_END]
DGROUP_FO=0x88a0; dsb=data[DGROUP_FO:]
DS_SIZE_INIT=len(dsb)   # initialized data size; BSS extends beyond
strtab={}
for m in re.finditer(rb'[\x20-\x7e]{3,}', dsb):
    strtab[m.start()]=m.group().decode()

# game function starts (near-call targets in range)
calltgt=defaultdict(list)
for i in range(len(img)-2):
    if img[i]==0xe8:
        rel=struct.unpack_from('<h',img,i+1)[0]; t=(i+3+rel)&0xffff
        if 0<=t<len(img): calltgt[t].append(i)
GAME_LO,GAME_HI=0x115,0x2474
fstarts=sorted([t for t in calltgt if GAME_LO<=t<GAME_HI]+[0x1a5,0x115,0x155,0x171,0x185])
fstarts=sorted(set(fstarts))+[GAME_HI]

def fn_of(addr):
    prev=None
    for f in fstarts:
        if f>addr: return prev
        prev=f
    return prev

md=Cs(CS_ARCH_X86, CS_MODE_16); md.detail=True
glob=defaultdict(lambda: {'r':0,'w':0,'sz':set(),'fns':set()})
# scan only plausible code (linear over game range); accept [imm16] with base==0,index==0
off=GAME_LO
end=GAME_HI
buf=img[:end]
for insn in md.disasm(buf, 0):
    if insn.address<GAME_LO: continue
    if insn.address>=GAME_HI: break
    for op in insn.operands:
        if op.type==CS_OP_MEM and op.mem.base==0 and op.mem.index==0:
            d=op.mem.disp & 0xffff
            # heuristic: DS globals live above the string/const area; include all
            g=glob[d]
            g['sz'].add(op.size)
            g['fns'].add(fn_of(insn.address))
            if op.access & CS_AC_WRITE: g['w']+=1
            if op.access & CS_AC_READ: g['r']+=1

print("DS offset : size : R/W : #fns : (string?) : accessing fns")
for d in sorted(glob):
    g=glob[d]
    s=",".join(str(x) for x in sorted(g['sz']))
    st=strtab.get(d,'')
    tag=f"'{st[:18]}'" if st else ''
    fns=" ".join(f"{f:x}" for f in sorted(x for x in g['fns'] if x is not None))
    nf=len(g['fns'])
    print(f"  {d:04x} : sz{s:5} : r{g['r']:>3} w{g['w']:>3} : n{nf:>2} : {tag:20} : {fns[:70]}")
