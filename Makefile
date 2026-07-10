# Arcade Volleyball (1988) — reconstructed from AV.EXE / AV.DAT
#
#   make            build ./av (native SDL2)
#   make web        WebAssembly browser build into web/ (needs emscripten)
#   make serve      build web/ and serve it at http://localhost:8001
#   make wasi       headless WASI build (av-wasi.wasm) for wasmtime etc.
#   make run-wasi   run a scripted 140-frame match under wasmtime -> frame.ppm
CC      = cc
CFLAGS  = -std=c99 -O2 -g -Wall -Wextra -Wno-unused-parameter -Isrc $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lm
SRC     = src/dos.c src/shim.c src/game.c src/input.c src/rtl.c src/platform.c $(wildcard src/gen/*.c)
HDR     = $(wildcard src/*.h)
OBJ     = $(SRC:.c=.o)

av: $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)

# Layer-1 pipeline validator: loads real AV.DAT and renders every sprite.
render-test: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -DRENDER_TEST $(SRC) -o $@ $(LDFLAGS)

# --- WebAssembly browser build: the same sources against emscripten's SDL2
# port, AV.DAT packed into the page.  The decompiled game owns its control
# flow (menus busy-wait in dos_getch/kbhit), so -sASYNCIFY lets the blocking
# code yield to the browser inside SDL_Delay instead of restructuring it.
# No ALLOW_MEMORY_GROWTH: Chrome's WebGL rejects texSubImage2D from a
# resizable heap, and DS[]+VIDEO[] fit trivially in the default 16 MB. ---
EMCC ?= emcc

web/index.html: $(SRC) $(HDR) src/shell.html AV.DAT
	mkdir -p web
	$(EMCC) -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Isrc \
	    -sUSE_SDL=2 -sASYNCIFY \
	    --shell-file src/shell.html --preload-file AV.DAT \
	    -o $@ $(SRC)

web: web/index.html

serve: web
	python3 -m http.server 8001 -d web

# --- Headless WASI build for standalone runtimes (wasmtime): platform_wasi.c
# replaces platform.c (no SDL/display/audio), same AV_INJECT/AV_SHOT env
# contract, so a scripted run is byte-identical to a native dummy-video run.
# Links real wasi-libc (emscripten's STANDALONE_WASM stubs out fopen);
# -nodefaultlibs -lc because Homebrew ships no wasm32-wasi compiler-rt and
# this code needs none. ---
WASI_CC      ?= $(shell brew --prefix emscripten)/libexec/llvm/bin/clang
WASI_SYSROOT ?= $(shell brew --prefix wasi-libc)/share/wasi-sysroot
WASISRC = src/dos.c src/shim.c src/game.c src/input.c src/rtl.c \
          src/platform_wasi.c $(wildcard src/gen/*.c)

av-wasi.wasm: $(WASISRC) $(HDR)
	$(WASI_CC) --target=wasm32-wasip1 --sysroot=$(WASI_SYSROOT) \
	    -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Isrc -DAV_NO_SDL \
	    -nodefaultlibs -lc -o $@ $(WASISRC)

wasi: av-wasi.wasm

run-wasi: av-wasi.wasm
	wasmtime run --dir . \
	    --env AV_INJECT="5=1c0d,40=2d00,40=2e00,72=2e80,95=2d00,96=2d80,120=2e00" \
	    --env AV_SHOT=frame.ppm --env AV_SHOT_FRAMES=140 av-wasi.wasm

clean:
	rm -f av render-test av-wasi.wasm frame.ppm $(OBJ)
	rm -rf web

.PHONY: web serve wasi run-wasi clean
