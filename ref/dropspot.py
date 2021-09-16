import math
import collections

def mdist(x, y):
  return abs(x) + abs(y)

def dist2(x1, y1, x2, y2):
  return (x2 - x1) ** 2 + (y2 - y1) ** 2

def pos(p):
  return (p[1] * 16 + p[0] + 256) % 256

dists = collections.defaultdict(list)

for j in range(-8, 8):
  for i in range(-8, 8):
    d = mdist(i, j)
    dists[d].append((i, j))

a = []
p = (0, 0)
for d in range(1, 31):
  ps = set(dists[d])
  while ps:
    if len(ps) > 1:
      np = min(*ps, key=lambda x: dist2(p[0], p[1], x[0], x[1]))
      ps.discard(np)
    else:
      np = ps.pop()
    a.append(pos(np))
    p = np

print(', '.join(f'0x{x:02x}' for x in a))
