#!/usr/bin/env python3
"""Linear disassembly of AV.EXE code segment + near-CALL target histogram."""
import struct, re, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

data = open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; CODE_END=0x88a0
img = data[LOAD:CODE_END]
DGROUP_FO=0x88a0; ds=data[DGROUP_FO:]
strtab={}
for m in re.finditer(rb'[\x20-\x7e]{3,}', ds):
    strtab[m.start()]=m.group().decode()

# 1) Harvest all near CALL (E8) targets by brute scan
calltgt={}
for i in range(len(img)-2):
    if img[i]==0xe8:
        rel=struct.unpack_from('<h', img, i+1)[0]
        t=(i+3+rel)&0xffff
        if 0<=t<len(img):
            calltgt.setdefault(t,[]).append(i)
# functions = targets called at least once
funcs=sorted(calltgt)
print(f"{len(funcs)} distinct near-call targets")
# histogram of most-called (library helpers) — informative
top=sorted(calltgt.items(), key=lambda kv:-len(kv[1]))[:25]
print("Most-called targets (offset : ncalls):")
for t,cs in top:
    print(f"  {t:05x} : {len(cs)}")

# 2) Full linear disassembly to file with string annotations
md=Cs(CS_ARCH_X86, CS_MODE_16); md.detail=True
def ann(insn):
    out=[]
    for op in insn.operands:
        if op.type==3:
            mm=op.mem
            if mm.base==0 and mm.index==0:
                d=mm.disp & 0xffff
                if d in strtab: out.append(f"'{strtab[d][:20]}'")
    # call target label
    if insn.mnemonic=='call' and insn.operands and insn.operands[0].type==1:
        out.append(f"-> sub_{insn.operands[0].imm:05x}")
    return "  ; "+", ".join(out) if out else ""

funcset=set(funcs)
with open('/Users/bijan/src/av/build/av.asm','w') as f:
    for insn in md.disasm(img, 0):
        lbl = f"\nsub_{insn.address:05x}:\n" if insn.address in funcset else ""
        f.write(f"{lbl}  {insn.address:05x}: {insn.mnemonic:6} {insn.op_str}{ann(insn)}\n")
print("wrote build/av.asm")
