export const meta = {
  name: 'av-decompile',
  description: 'Decompile Arcade Volleyball game functions from disassembly to faithful C',
  phases: [
    { title: 'Decompile', detail: 'translate each function to C (src/gen/<name>.c)' },
    { title: 'Verify', detail: 'adversarially check each C function against its disassembly and fix' },
  ],
}

const ROOT = '/Users/bijan/src/av'
const fns = typeof args === 'string' ? JSON.parse(args) : args // function names

const SHARED = `
Read these first (Read tool), they define the contract:
  ${ROOT}/build/SPEC.md          - the translation contract; FOLLOW IT EXACTLY
  ${ROOT}/src/dos.h              - memory model (DS[]/VIDEO[]), accessors B/SB/W/UW, shim API you may call
  ${ROOT}/src/game_protos.h      - names/return types of other game functions you may call

Model facts (already validated against the real binary):
- The whole 64KB data segment is DS[]; a near pointer is a dsptr = offset into DS.
- Accessors: B(o) unsigned char, SB(o) signed char, W(o) 16-bit SIGNED int, UW(o) 16-bit unsigned. IMG(o)=UW(o).
- Named globals (macros): ball_x ball_y p1_y p2_y p1_frame p2_frame server side_swap hit_count font_img dat_fd joy_timeout joy_val quit_flag.
- Turbo C int is 16-bit signed; char signed. Reproduce 8-bit (al/ah) masking and 16-bit wrap.
- cdecl: first arg at [bp+arg0] (arg0 given in the disasm header/comments); model args as int params a0,a1,....
- Control flow: use goto + labels named L<hex> (e.g. L1a54). Signed jumps jl/jg/jle/jge; unsigned jb/ja/jbe/jae; je/jne equality.
- Library/RTL call mapping: initgraph->cga_init, graphresult->graphresult, imagesize->bgi_imagesize, getimage->bgi_getimage,
  putimage (0x6c42) -> bgi_outtextxy(int x,int y,dsptr str)  [it draws TEXT, not an image; the pushed far ptr is a string],
  outtext (0x63f4) -> bgi_outtext(dsptr str), bar->bgi_bar, setfillstyle->bgi_setfillstyle, lib_6129->bgi_setcolor,
  lib_5d80->bgi_cleardevice, lib_5fca->bgi_rectangle(int x1,int y1,int x2,int y2), lib_6b04->bgi_settextstyle_dir,
  lib_5409->(BGI internal; ignore/stub), registerbgidriver->registerbgidriver,
  malloc->dos_malloc, sys_open/sys_read/sys_close as named, sys_close->sys_close, printf->dos_printf, exit->dos_exit,
  movedata->movedata_ds, int86->dos_int86, bios10->(BIOS INT 10h; see call site, usually mode/palette set - you may stub with a comment).
- Game primitives you may call (exact sigs in dos.h): draw_sprite(int x,int y,dsptr img), draw_sprite2(int x,int y,dsptr img), wait_vsync(void).
- Joystick game port 0x201 (in/out dx where dx=0x201): call the provided helpers joy_read_button(player)/joy_read_axis(player,&timeout) if the logic matches; otherwise translate the port timing loop literally but read the button/axis via those helpers. The CGA retrace port 0x3da busy-wait means "wait for vsync" -> call wait_vsync().
- Keyboard: if the function installs interrupt vectors (writes to the IVT / setvect) or IS an INT 9 scancode ISR (reads port 0x60, writes 0x20/0x61, ends in iret), DO NOT emulate it. Replace the vector-install sequence with a single call to kb_install(); (declare 'extern void kb_install(void);'). Skip/By pass the ISR body and note it in your report. The game reads key state from a DS array which SDL will populate.
- PC speaker (ports 0x42/0x43/0x61): call speaker_tone(divisor) / speaker_off() instead of the raw out instructions.
`

phase('Decompile')
const results = await pipeline(
  fns,
  // stage 1: decompile
  (name) => agent(
    `Decompile the 16-bit function "${name}" of Arcade Volleyball into faithful C.
${SHARED}

Now Read ${ROOT}/build/dis/${name}.txt (the annotated disassembly of THIS function) and translate it.

Output: Write ONLY the C to ${ROOT}/src/gen/${name}.c with this shape:
  /* 0xADDR ${name} - one-line purpose */
  #include "dos.h"
  #include "game_protos.h"
  #define IMG(o) UW(o)
  int ${name}(<your params>) { ... }
The function MUST be 'int ${name}(...)' and return the value in AX at the ret (return 0 if it returns nothing meaningful).
Reproduce EVERY instruction's effect; do not guess or "improve" logic. If a jump table appears, emit a switch to the case labels.
After writing, return a 3-6 line report: chosen signature, purpose, lib/input calls used, uncertainties.`,
    { label: `decompile:${name}`, phase: 'Decompile' }
  ),
  // stage 2: verify + fix in place
  (report, name) => agent(
    `Adversarially verify the decompiled C in ${ROOT}/src/gen/${name}.c against the disassembly ${ROOT}/build/dis/${name}.txt for the Arcade Volleyball function "${name}".
${SHARED}

Read both files. Check, instruction by instruction, that the C faithfully reproduces the assembly:
- arithmetic widths (8/16-bit), signed vs unsigned compares and shifts (sar vs shr, jl vs jb),
- correct DS offsets in accessors, correct struct strides (player=0x3c, control=6),
- control flow / loop bounds / jump-table cases, argument order, return value,
- correct mapping of lib/shim calls.
If you find ANY discrepancy, FIX ${ROOT}/src/gen/${name}.c directly (Write the corrected full file). Keep the same signature/shape.
Return a short verdict: CONFIRMED (faithful) or FIXED (list what you corrected).`,
    { label: `verify:${name}`, phase: 'Verify' }
  ),
)

log(`decompiled+verified ${results.filter(Boolean).length}/${fns.length} functions`)
return { done: results.filter(Boolean).length, total: fns.length }
