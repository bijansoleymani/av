#!/usr/bin/env python3
"""Inventory game-region functions (0x1a5..0x2470): boundaries, size, INT/lcall
usage, DGROUP string refs, and near-call edges — to plan decompilation."""
import struct, re
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; CODE_END=0x88a0; img=data[LOAD:CODE_END]
DGROUP_FO=0x88a0; dsb=data[DGROUP_FO:]
strtab={}
for m in re.finditer(rb'[\x20-\x7e]{3,}', dsb):
    strtab[m.start()]=m.group().decode()

# harvest near-call targets
calltgt={}
for i in range(len(img)-2):
    if img[i]==0xe8:
        rel=struct.unpack_from('<h',img,i+1)[0]; t=(i+3+rel)&0xffff
        if 0<=t<len(img): calltgt.setdefault(t,[]).append(i)

md=Cs(CS_ARCH_X86,CS_MODE_16); md.detail=True
GAME_LO,GAME_HI=0x115,0x2474    # from startup (0x115) through end of game code
game_funcs=sorted(t for t in calltgt if GAME_LO<=t<GAME_HI)
game_funcs=[0x1a5]+[t for t in game_funcs if t!=0x1a5]
game_funcs=sorted(set(game_funcs))

# lcall (far-call) library target names we can guess later
def disfn(start, stop):
    """disasm from start until a ret/retf that returns to caller depth 0-ish;
    approximate: stop at next function start or a lone ret."""
    out=[]
    off=start
    while off<stop:
        try: insn=next(md.disasm(img[off:off+16], off))
        except StopIteration: break
        out.append(insn)
        if insn.mnemonic in ('ret','retf'):
            break
        off+=insn.size
    return out

# Build ordered boundaries: a function extends to the next function start.
fstarts=game_funcs+[GAME_HI]
print(f"{len(game_funcs)} game functions in [0x{GAME_LO:x},0x{GAME_HI:x})\n")
print(f"{'fn':>7} {'end':>6} {'sz':>5} {'ncall':>5}  ints/lcalls / string refs")
for idx,f in enumerate(game_funcs):
    end=fstarts[idx+1]
    ncall=len(calltgt.get(f,[]))
    ints=[]; lcalls=[]; srefs=set(); ncalls=[]
    off=f
    while off<end:
        try: insn=next(md.disasm(img[off:off+16], off))
        except StopIteration: break
        m=insn.mnemonic
        if m=='int': ints.append(insn.op_str)
        if m in ('lcall','call') and insn.operands:
            op=insn.operands[0]
            if m=='lcall': lcalls.append(insn.op_str.replace(' ',''))
            elif op.type==1: ncalls.append(f"{op.imm:x}")
        for op in insn.operands:
            if op.type==3 and op.mem.base==0 and op.mem.index==0:
                d=op.mem.disp&0xffff
                if d in strtab: srefs.add(strtab[d][:16])
        off+=insn.size
    info=[]
    if ints: info.append("INT["+",".join(sorted(set(ints)))+"]")
    if lcalls: info.append("LCALL["+",".join(sorted(set(lcalls))[:6])+"]")
    if srefs: info.append("STR["+",".join(sorted(srefs)[:5])+"]")
    print(f"{f:7x} {end:6x} {end-f:5d} {ncall:5d}  " + " ".join(info))
