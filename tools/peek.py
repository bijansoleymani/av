#!/usr/bin/env python3
"""Disassemble N instructions at given code offsets (to ID library routines)."""
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; img=data[LOAD:0x88a0]
md=Cs(CS_ARCH_X86, CS_MODE_16)
targets=[int(x,16) for x in sys.argv[1:]] or [0x5aa2]
for t in targets:
    print(f"\n=== {t:05x} ===")
    n=0
    for insn in md.disasm(img[t:t+80], t):
        print(f"  {insn.address:05x}: {insn.mnemonic:6} {insn.op_str}")
        n+=1
        if n>=18 or insn.mnemonic in ('ret','retf','iret'): break
