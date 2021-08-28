#!/bin/bash
../rgbds/rgbgfx -t tiles-bg.tilemap -u tiles-bg.png -o tiles-bg.2bpp
xxd -i tiles-bg.2bpp > tiles.c
sed -i "s/char/const char/" tiles.c
xxd -i map.bin > map.c
sed -i "s/char/const char/" map.c
xxd -i flags.bin > flags.c
sed -i "s/char/const char/" flags.c
../gbdk/bin/lcc -Wa-l -Wl-m tiles.c map.c flags.c main.c -o main.gb
