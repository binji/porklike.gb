#!/bin/bash
set -e

../rgbds/rgbgfx -u bg.png -o bg.bin
../rgbds/rgbgfx -u shared.png -o shared.bin
../rgbds/rgbgfx -u sprites.png -o sprites.bin
../rgbds/rgbgfx -u dead.png -o dead.bin
../rgbds/rgbgfx -t title.tilemap -u title.png -o title.bin

../gbdk/bin/gbcompress bg.bin bg.binz
../gbdk/bin/gbcompress shared.bin shared.binz
../gbdk/bin/gbcompress sprites.bin sprites.binz
../gbdk/bin/gbcompress dead.bin dead.binz
../gbdk/bin/gbcompress title.bin title.binz

python ref/bin2c.py bg.binz -t -o tilebg.c
python ref/bin2c.py shared.binz -t -o tileshared.c
python ref/bin2c.py sprites.binz -t -o tilesprites.c
python ref/bin2c.py dead.binz -t -o tiledead.c
python ref/bin2c.py title.binz -t -o tiletitle.c

../gbdk/bin/lcc -Wa-l util.s -c -o util.o
../gbdk/bin/lcc -Wa-l bank0.c -c -o bank0.o
../gbdk/bin/lcc -Wa-l title.c -c -o title.o
../gbdk/bin/lcc -Wa-l gen.c -c -o gen.o
../gbdk/bin/lcc -Wa-l main.c -c -o main.o
../gbdk/bin/lcc -Wl-j -Wl-m -Wl-w -Wl-yo4 -Wl-yt1 -Wm-ynPORKLIKE -Wm-yS bank0.o main.o util.o title.o gen.o -o porklike.gb
