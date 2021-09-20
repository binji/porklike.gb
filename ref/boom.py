import math
import random

def tofixed(n):
  n = math.floor(n * 256)
  if n < 0:
    n = 65536 + n
  return n

fs = []
afs = []
xs = []
ys = []
dxs = []
dys = []

for i in range(16):
  ang = random.random() * math.pi * 2
  dist = random.random() * 5
  spd = 0.5 + random.random() * 0.5
  frames = random.randint(15, 45)
  anim_frames = math.ceil(frames / 4)
  x = tofixed(math.cos(ang) * dist)
  y = tofixed(math.sin(ang) * dist)
  dx = tofixed(math.cos(ang) * spd)
  dy = tofixed(math.sin(ang) * spd)
  fs.append(frames)
  afs.append(anim_frames)
  xs.append(x)
  ys.append(y)
  dxs.append(dx)
  dys.append(dy)

print(f"const u8 boom_spr_speed[] = {{{', '.join(map(str, fs))}}};")
print(f"const u8 boom_spr_anim_speed[] = {{{', '.join(map(str, afs))}}};")
print(f"const u16 boom_spr_x[] = {{{', '.join(map(str, xs))}}};")
print(f"const u16 boom_spr_y[] = {{{', '.join(map(str, ys))}}};")
print(f"const u16 boom_spr_dx[] = {{{', '.join(map(str, dxs))}}};")
print(f"const u16 boom_spr_dy[] = {{{', '.join(map(str, dys))}}};")
