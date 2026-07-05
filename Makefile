# Arcade Volleyball (1988) — reconstructed from AV.EXE / AV.DAT
CC      = cc
CFLAGS  = -std=c99 -O2 -g -Wall -Wextra -Wno-unused-parameter -Isrc $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lm
SRC     = src/dos.c src/shim.c src/game.c src/input.c src/rtl.c src/platform.c $(wildcard src/gen/*.c)
OBJ     = $(SRC:.c=.o)

av: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)

# Layer-1 pipeline validator: loads real AV.DAT and renders every sprite.
render-test: $(SRC)
	$(CC) $(CFLAGS) -DRENDER_TEST $(SRC) -o $@ $(LDFLAGS)

clean:
	rm -f av render-test $(OBJ)

.PHONY: clean
