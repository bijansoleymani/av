#!/usr/bin/env python3
"""Function detection + disassembly with embedded switch-jump-table handling.
A Turbo C switch compiles to:  jmp word ptr cs:[bx + T]  where T is the address
immediately after the jmp; the near-offset table lives at [T, T+2*ncases).
ncases = (min(table entries) - T)//2  (entries point to code after the table)."""
import struct, re, os
from collections import defaultdict
from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_OP_MEM, CS_OP_IMM

data=open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD=0x200; img=data[LOAD:0x88a0]
DS_INIT=data[0x88a0:]
strtab={}
for m in re.finditer(rb'[\x20-\x7e]{3,}', DS_INIT): strtab[m.start()]=m.group().decode()
GAME_LO,GAME_HI=0x115,0x2474
md=Cs(CS_ARCH_X86,CS_MODE_16); md.detail=True

# ---- pre-scan for jump tables ----
tables={}   # T -> (ncases, [targets], table_end)
def find_tables():
    for insn in md.disasm(img[GAME_LO:GAME_HI], GAME_LO):
        if insn.mnemonic=='jmp' and 'cs:[' in insn.op_str and insn.operands and insn.operands[0].type==CS_OP_MEM:
            m=insn.operands[0].mem
            T=m.disp & 0xffff
            if T==insn.address+insn.size:      # table right after the jmp
                # read entries until one points before T (heuristic min-target)
                entries=[]
                p=T
                while p+2<=GAME_HI:
                    e=struct.unpack_from('<H', img, p)[0]
                    entries.append(e); p+=2
                    mn=min(entries)
                    if mn<=p:            # would-be table_end passed the min target
                        entries.pop(); break
                    if len(entries)>64: break
                ncases=(min(entries)-T)//2 if entries else 0
                entries=entries[:ncases]
                tables[T]=(ncases, entries, T+2*ncases)
find_tables()
skip=set()
for T,(n,ent,end) in tables.items():
    for a in range(T,end): skip.add(a)

# ---- decode, skipping tables, following case targets ----
ins_by_addr={}
def decode_from(addr, seen):
    while addr<GAME_HI:
        if addr in skip:                 # jump over table bytes
            addr=[e for t,(n,ent,e) in tables.items() if t<=addr<e][0]; continue
        if addr in seen: return
        seen.add(addr)
        try: insn=next(md.disasm(img[addr:addr+16], addr))
        except StopIteration: return
        ins_by_addr[addr]=insn
        addr+=insn.size

# linear decode but skip table bytes (recompute cleanly)
addr=GAME_LO
while addr<GAME_HI:
    if addr in skip:
        addr=[e for t,(n,ent,e) in tables.items() if t<=addr<e][0]; continue
    try: insn=next(md.disasm(img[addr:addr+16], addr))
    except StopIteration: break
    ins_by_addr[addr]=insn
    addr+=insn.size

calltgt=defaultdict(list)
for a,insn in ins_by_addr.items():
    if insn.mnemonic=='call' and insn.operands and insn.operands[0].type==CS_OP_IMM:
        t=insn.operands[0].imm
        if GAME_LO<=t<GAME_HI: calltgt[t].append(a)
entries=sorted(set(calltgt)|{0x1a5,0x115,0x155,0x171,0x185})
fstarts=entries+[GAME_HI]
print(f"{len(entries)} functions; {len(tables)} jump tables:")
for T,(n,ent,end) in sorted(tables.items()):
    print(f"  table@0x{T:x} ncases={n} -> "+", ".join(f'0x{e:x}' for e in ent))

NAMED={0x1a5:'game_main',0x300:'sub_300',0x345:'preshift_sprite',0x41a:'read_chunk',
 0x43a:'load_data',0x691:'draw_scene',0x724:'joy_calibrate',0x779:'menu_draw',
 0x9af:'play_round',0xbee:'setup_round',0x199b:'collide_check',0x22bf:'wait_vsync',
 0x22d0:'draw_sprite',0x23e2:'draw_sprite2',0xd50:'read_joystick',0xde9:'read_mouse',
 0x115:'_exit',0x155:'_startup_b',0x171:'_startup_c',0x185:'_startup_d'}
LIB={0x44b8:'malloc',0x45f4:'sys_open',0x464e:'sys_read',0x3cbd:'sys_close',0x3e36:'printf',
 0x402f:'exit',0x420c:'registerbgidriver',0x466c:'movedata',0x4230:'int86',0x3e83:'bios10',
 0x42f2:'_ioerror',0x4b5e:'_vprintf',0x3cdf:'_write',0x5aa2:'initgraph',0x539d:'graphresult',
 0x5ce3:'lib_5ce3',0x6035:'setfillstyle',0x6a19:'bar',0x6324:'imagesize',0x6d73:'getimage',
 0x6c42:'putimage',0x63f4:'outtext',0x5fca:'lib_5fca',0x5d80:'lib_5d80',0x6129:'lib_6129',
 0x6b04:'lib_6b04',0x5409:'lib_5409'}
def fname(a): return NAMED.get(a) or LIB.get(a) or f'sub_{a:05x}'
SPRITE_SLOTS=set()
for base in (0x966,0x994,0x9ee,0xa18):
    for g in range(4):
        for s in range(4): SPRITE_SLOTS.add(base+g*8+s*2)
for s in (0x9b4,0x9ba,0x9da,0xa38): SPRITE_SLOTS.add(s)
GNAME={0x98c:'ball_x',0x98e:'ball_y',0x9e4:'p1_y',0x9e6:'p2_y',0x9d6:'server',0x9d8:'side_swap',
 0x9e2:'hit_count',0x9be:'p1_frame',0x9c0:'p2_frame',0x9c2:'font_img',0xa50:'dat_fd',
 0xa42:'joy_timeout',0xa44:'joy_val',0x24e:'quit_flag'}
def gname(d):
    if d in SPRITE_SLOTS: return f'IMG(0x{d:x})'
    return GNAME.get(d) or (f'STR(0x{d:x})' if d in strtab else f'g_{d:04x}')

os.makedirs('/Users/bijan/src/av/build/dis', exist_ok=True)
for fn in os.listdir('/Users/bijan/src/av/build/dis'): os.remove('/Users/bijan/src/av/build/dis/'+fn)
def dis_func(start,end):
    out=[]; off=start
    while off<end:
        if off in tables:
            n,ent,tend=tables[off]
            out.append(f"  {off:05x}: ; --- switch jump table, {n} cases ---")
            for i,e in enumerate(ent): out.append(f"           case {i}: -> 0x{e:05x}")
            off=tend; continue
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
    end=fstarts[fstarts.index(f)+1]; nm=fname(f)
    hdr=f"; ==== {nm}  (0x{f:05x}..0x{end:05x}, {end-f} bytes, {len(calltgt.get(f,[]))} callers) ===="
    block=hdr+"\n"+"\n".join(dis_func(f,end))+"\n"
    allf.append(block); open(f'/Users/bijan/src/av/build/dis/{nm}.txt','w').write(block)
open('/Users/bijan/src/av/build/all_funcs.dis','w').write("\n".join(allf))
open('/Users/bijan/src/av/build/func_list.txt','w').write("\n".join(fname(f) for f in entries))
print("\nfunctions:", ", ".join(fname(f) for f in entries))
