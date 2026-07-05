export const meta = {
  name: 'av-refactor',
  description: 'Refactor decompiled Arcade Volleyball functions into idiomatic C (behavior-preserving)',
  phases: [
    { title: 'Refactor', detail: 'rewrite each function into idiomatic C' },
    { title: 'Verify', detail: 'check behavioral equivalence vs the original + fix' },
  ],
}

const ROOT = '/Users/bijan/src/av'
const fns = typeof args === 'string' ? JSON.parse(args) : args

const SHARED = `
This is a READABILITY refactor of one decompiled C function from the game Arcade
Volleyball. The goal: turn assembly-like C (register-named vars ax/bx/si/di,
goto+labels, raw W(0xNNN) offsets) into idiomatic C — meaningful variable names,
structured control flow (for/while/do-while/if-else/switch), and the named
accessors from game.h.

*** ABSOLUTE RULE: preserve behavior EXACTLY. ***
This must not change what the code does — only how it reads. Same arithmetic and
operand widths, same signed-vs-unsigned operations (>> on an int is arithmetic;
UW() reads are unsigned; 16-bit wraparound via (short)/(unsigned short) casts
must be kept where the original wraps), same order of reads/writes and side
effects, same function calls with the same arguments, same return value. The game
is verified against a golden frame-by-frame reference; any behavioral drift fails.

Read these first (Read tool):
  ${ROOT}/src/game.h            - named state accessors (USE THESE), all backed by DS[]
  ${ROOT}/src/dos.h             - the DS[]/VIDEO[] memory model + B/SB/W/UW accessors
  ${ROOT}/src/game_protos.h     - signatures of the other functions you may call

game.h gives you: player_x(p) player_y(p) player_frame(p) player_state(p),
ctrl_left(p)/ctrl_right(p)/ctrl_jump(p), score(s), ball_x ball_y ball_vx ball_vy
ball_xf ball_yf ball_prev_x ball_prev_y ball_frame ball_high, server lead_side
hit_count game_over sound_on side_swap touches serve_state bounce_shift,
player_sprite(p,frame,shift) ball_sprite(frame,shift) smallball_sprite(frame,shift)
net_sprite post_left_sprite post_right_sprite, dat_fd font_img joy_timeout joy_val,
and IMG(o).  (Indices: 0 = left player/side, 1 = right.)

How to refactor:
- Reconstruct the control-flow graph from the goto/labels and re-express it as
  natural loops and conditionals. A "xor si,si; jmp test; body...; inc si; test:
  cmp si,N; jl body" idiom is 'for (i = 0; i < N; i++) { body }'. Down-counters,
  do/while, early-outs -> if/return, etc. Jump tables are already shown as a
  switch. If a fragment is genuinely irreducible, keeping ONE goto is acceptable —
  correctness beats purity.
- Name locals for what they hold: loop indices (i, player, side, col, row),
  positions/velocities, sprite indices, temporaries. Drop ax/bx/si/di/loc2/bp_m4.
- Replace raw W()/UW()/IMG() offsets with the game.h accessor whenever it matches
  (e.g. W(0x98c + si*2) -> player_x(i); W(0x9c8 + p*6) -> ctrl_left(p);
  UW(0x994 + frame*8 + (x&3)*2) -> player_sprite(0, frame, x & 3)). For offsets
  with NO game.h name, keep W(0xNNN)/UW(0xNNN) and add a short comment on its role
  — do NOT invent new global names.
- Keep the exact function signature and the leading "/* 0xADDR name - purpose */".
- Start the file with: #include "dos.h" then #include "game_protos.h". Do NOT add
  '#define IMG(o) UW(o)' (game.h already defines IMG).
- Drop the noisy per-line "/* 00abc: mov ... */" trace comments; keep a short
  comment only where the intent isn't obvious.
`

phase('Refactor')
const results = await pipeline(
  fns,
  (name) => agent(
    `Refactor the decompiled function "${name}" into idiomatic, behavior-preserving C.
${SHARED}

The current (assembly-like) version to rewrite is ${ROOT}/src/gen/${name}.c — Read
it. Its byte-accurate disassembly is ${ROOT}/build/dis/${name}.txt — Read it too
and use it to resolve any ambiguity about signedness/widths/flow. A pristine copy
of the original is at ${ROOT}/build/gen_orig/${name}.c for reference.

Write the refactored function back to ${ROOT}/src/gen/${name}.c (overwrite).
Then return a 3-6 line report: the control structures you reconstructed, the main
variables you named, and anything you had to keep as a goto or raw offset.`,
    { label: `refactor:${name}`, phase: 'Refactor' }
  ),
  (report, name) => agent(
    `Adversarially verify that ${ROOT}/src/gen/${name}.c (just refactored) is
BEHAVIORALLY IDENTICAL to the original ${ROOT}/build/gen_orig/${name}.c and to the
disassembly ${ROOT}/build/dis/${name}.txt for the Arcade Volleyball function "${name}".
${SHARED}

Go through it carefully:
- every branch condition matches (signed jl/jg/jle/jge vs unsigned jb/ja; ==/!=),
- loop bounds and iteration counts are identical,
- arithmetic widths & signedness match (arithmetic vs logical shift; 8-bit al/ah
  masking; 16-bit wrap via (short)/(unsigned short)),
- the SAME DS offsets are read/written (game.h accessor must expand to the same
  offset — verify player_x(1) is 0x98e, ctrl_jump(p) is 0x9cc+p*6, etc.),
- argument order and return value are unchanged,
- no reordered side effects.
If you find ANY discrepancy, FIX ${ROOT}/src/gen/${name}.c directly (keep it
idiomatic). Return: CONFIRMED (identical) or FIXED (what you corrected).`,
    { label: `verify:${name}`, phase: 'Verify' }
  ),
)

log(`refactored+verified ${results.filter(Boolean).length}/${fns.length}`)
return { done: results.filter(Boolean).length, total: fns.length }
