#!/bin/bash
../rgbds/rgbgfx -t bg.tilemap -u bg.png -o bg.bin
../rgbds/rgbgfx -u shared.png -o shared.bin
../rgbds/rgbgfx -u sprites.png -o sprites.bin
python ref/bin2c.py bg.bin -t -o tilebg.c
python ref/bin2c.py shared.bin -t -o tileshared.c
python ref/bin2c.py sprites.bin -t -o tilesprites.c
../gbdk/bin/lcc -Wa-l main.c -c -o main.o
../gbdk/bin/lcc -Wa-l util.s -c -o util.o
../gbdk/bin/lcc -Wl-m main.o util.o -o main.gb
