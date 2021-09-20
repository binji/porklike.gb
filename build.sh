#!/bin/bash
../rgbds/rgbgfx -t bg.tilemap -u bg.png -o bg.bin
../rgbds/rgbgfx -u shared.png -o shared.bin
../rgbds/rgbgfx -u sprites.png -o sprites.bin
../gbdk/bin/gbcompress bg.bin bg.binz
../gbdk/bin/gbcompress shared.bin shared.binz
../gbdk/bin/gbcompress sprites.bin sprites.binz
python ref/bin2c.py bg.binz -t -o tilebg.c
python ref/bin2c.py shared.binz -t -o tileshared.c
python ref/bin2c.py sprites.binz -t -o tilesprites.c
../gbdk/bin/lcc -Wa-l main.c -c -o main.o
../gbdk/bin/lcc -Wa-l util.s -c -o util.o
../gbdk/bin/lcc -Wl"-m -w" main.o util.o -o main.gb
