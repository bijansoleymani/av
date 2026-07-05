#!/usr/bin/env python3
"""Map AV.EXE segment layout: code seg, DGROUP, embedded BGI driver, fonts."""
import struct

data = open('/Users/bijan/src/av/AV.EXE','rb').read()
LOAD = 0x200
# DGROUP paragraph from reloc word 0x086a
DGROUP_PARA = 0x086a
dgroup_fo = LOAD + DGROUP_PARA*16
print(f"Code seg  : file 0x{LOAD:05x} .. 0x{dgroup_fo:05x}  ({dgroup_fo-LOAD} bytes)")
print(f"DGROUP    : file 0x{dgroup_fo:05x} .. 0x{len(data):05x}  ({len(data)-dgroup_fo} bytes init data)")

# BGI driver blob: header 'BGI Device Driver' — the BGI file format begins with
# a signature. Real .BGI starts with 0x08 0x80 ('\x08\x80') then description.
bgi = data.find(b'BGI Device Driver')
# back up to find the driver start (the header byte pattern before the desc string)
start = bgi
while start>LOAD and data[start-1] not in (0x1a,):
    start-=1
print(f"\nBGI desc string @ file 0x{bgi:05x} (load-rel 0x{bgi-LOAD:05x})")
# Show 32 bytes before desc
print("bytes before desc:", data[bgi-16:bgi].hex())

# Find all .CHR / .BGI style and font markers
for marker in [b'PK\x08\x08', b'\x08\x80']:
    i=0
    hits=[]
    while True:
        i=data.find(marker,i)
        if i<0: break
        hits.append(i); i+=1
    print(f"marker {marker!r}: {[hex(h) for h in hits[:20]]}")

# Show region around DGROUP start (initialized data / string table)
print("\n--- DGROUP head (file 0x%05x) ---" % dgroup_fo)
seg = data[dgroup_fo:dgroup_fo+16]
import re
# dump printable strings in DGROUP with offsets (DS-relative)
print("\nDGROUP strings (DS offset : text):")
ds = data[dgroup_fo:]
for m in re.finditer(rb'[\x20-\x7e]{4,}', ds):
    print(f"  {m.start():04x}: {m.group().decode()}")
