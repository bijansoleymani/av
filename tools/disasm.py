#!/usr/bin/env python3
"""Recursive-descent disassembler for AV.EXE code segment (16-bit real mode).

Emits offsets relative to the load image (file offset - 0x200), which equals the
segment offset for the single small-model code segment. Annotates:
  - INT calls
  - near CALL targets (builds the call graph / function list)
  - DS-relative memory operands that hit known DGROUP string offsets
"""
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

data = open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD = 0x200
CODE_END = 0x88a0            # DGROUP start (file)
img = data[LOAD:CODE_END]    # code bytes; index i == segment offset i

# DGROUP string table (DS offset -> text) from layout.py
DGROUP_FO = 0x88a0
import re
strtab = {}
ds = data[DGROUP_FO:]
for m in re.finditer(rb'[\x20-\x7e]{3,}', ds):
    strtab[m.start()] = m.group().decode()

md = Cs(CS_ARCH_X86, CS_MODE_16)
md.detail = True

def strref(insn):
    """If an operand is [imm16] matching a DGROUP string, return the text."""
    hits=[]
    for op in insn.operands:
        if op.type == 3:  # X86_OP_MEM
            m = op.mem
            if m.base==0 and m.index==0 and m.disp in strtab:
                hits.append(f"DS:{m.disp:04x}='{strtab[m.disp][:24]}'")
            elif m.base==0 and m.index==0 and 0 <= m.disp < len(ds):
                hits.append(f"DS:{m.disp:04x}")
    return "  ; "+", ".join(hits) if hits else ""

# Recursive descent from entry (offset 0) plus collect all CALL targets.
visited=set()
funcs=set([0x0])
calls={}  # target -> list of callers
worklist=[0x0]
insns_at={}

def disas_run(start):
    off=start
    while 0 <= off < len(img) and off not in visited:
        chunk = img[off:off+16]
        try:
            insn = next(md.disasm(chunk, off))
        except StopIteration:
            break
        visited.add(off)
        insns_at[off]=insn
        m=insn.mnemonic
        # follow control flow
        if m in ('jmp',):
            if insn.operands and insn.operands[0].type==1: # imm
                t=insn.operands[0].imm
                if 0<=t<len(img): worklist.append(t)
            break  # unconditional
        if m.startswith('j'):  # conditional jump
            if insn.operands and insn.operands[0].type==1:
                t=insn.operands[0].imm
                if 0<=t<len(img): worklist.append(t)
        if m=='call':
            if insn.operands and insn.operands[0].type==1:
                t=insn.operands[0].imm
                if 0<=t<len(img):
                    funcs.add(t); calls.setdefault(t,[]).append(off)
                    worklist.append(t)
        if m in ('ret','retf','iret'):
            break
        off += insn.size

while worklist:
    disas_run(worklist.pop())

print(f"visited {len(visited)} instrs, {len(funcs)} call targets")
# Sort functions, print with caller counts and first-instruction string refs
flist=sorted(funcs)
print("\n== Functions (offset : #callers : first bytes) ==")
for f in flist:
    ncall=len(calls.get(f,[]))
    insn=insns_at.get(f)
    txt=f"{insn.mnemonic} {insn.op_str}" if insn else "?"
    print(f"  {f:05x} : {ncall:3d} callers : {txt}")

# Which functions reference which strings (to locate menu/main/game code)
print("\n== Functions referencing DGROUP strings ==")
for f in flist:
    # walk linearly until ret to gather string refs (approximate)
    refs=[]
    off=f
    for _ in range(400):
        insn=insns_at.get(off)
        if not insn: break
        r=strref(insn)
        if r: refs.append(r.strip('; ').strip())
        if insn.mnemonic in ('ret','retf','iret','jmp'): break
        off+=insn.size
    if refs:
        print(f"  fn {f:05x}: " + " | ".join(refs[:8]))
