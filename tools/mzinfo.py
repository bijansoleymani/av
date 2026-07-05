#!/usr/bin/env python3
"""Parse the MZ header of AV.EXE, list relocations, and locate key strings."""
import struct, sys

data = open('/Users/bijan/src/av/AV.EXE','rb').read()
(sig, lastpage, pages, nreloc, hdrpar, minalloc, maxalloc,
 ss, sp, cksum, ip, cs, reloc_off, overlay) = struct.unpack_from('<2sHHHHHHHHHHHHH', data, 0)

print(f"signature      {sig}")
print(f"file size      {len(data)}")
img_end = (pages-1)*512 + (lastpage if lastpage else 512)
print(f"image bytes    {img_end}  (pages={pages}, lastpage={lastpage})")
print(f"relocations    {nreloc} @ 0x{reloc_off:x}")
print(f"header paras   {hdrpar} -> load image starts at 0x{hdrpar*16:x}")
print(f"initial CS:IP  {cs:04x}:{ip:04x}")
print(f"initial SS:SP  {ss:04x}:{sp:04x}")
print(f"minalloc {minalloc} maxalloc {maxalloc:04x}")

load = hdrpar*16
print(f"\nEntry file offset = load(0x{load:x}) + CS*16 + IP = 0x{load + cs*16 + ip:x}")

# Relocations
print("\nRelocations (offset:seg -> file offset of the word to patch):")
relocs = []
for i in range(nreloc):
    off, seg = struct.unpack_from('<HH', data, reloc_off + i*4)
    fo = load + seg*16 + off
    relocs.append((seg, off, fo))
for seg, off, fo in relocs:
    w = struct.unpack_from('<H', data, fo)[0]
    print(f"  {seg:04x}:{off:04x}  fileoff 0x{fo:05x}  word=0x{w:04x}")

# Locate notable strings
print("\nKey string offsets:")
for s in [b'Arcade Volleyball', b'av.dat', b'AV.DAT', b'BGI Device Driver',
          b'CGA mode not available', b'Play', b'PL1 Keyboard', b'Define  Keys',
          b'Turbo-C', b'COMPAQ', b'Center stick']:
    idx = data.find(s)
    print(f"  {s!r:40} @ 0x{idx:x}" if idx>=0 else f"  {s!r} NOT FOUND")
