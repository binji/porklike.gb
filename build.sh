#!/bin/bash
../rgbds/rgbgfx -t bg.tilemap -u bg.png -o bg.bin
../rgbds/rgbgfx -u shared.png -o shared.bin
../rgbds/rgbgfx -u sprites.png -o sprites.bin
cd ref
python flags.py
python map.py
cd ..
python ref/bin2c.py bg.bin -t -o tilebg.c
python ref/bin2c.py shared.bin -t -o tileshared.c
python ref/bin2c.py sprites.bin -t -o tilesprites.c
python ref/bin2c.py map.bin -o map.c
python ref/bin2c.py flags.bin -o flags.c
../gbdk/bin/lcc -Wa-l main.c -c -o main.o
../gbdk/bin/lcc -Wl-m main.o -o main.gb
