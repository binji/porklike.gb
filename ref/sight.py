import math

def sgn(n): return 1 if n >= 0 else -1

def los(pos1, pos2):
  res = []
  x1,y1 = pos1
  x2,y2 = pos2
  dx,dy = abs(x2-x1), abs(y2-y1)
  sx,sy = sgn(x2-x1), sgn(y2-y1)
  err = dx-dy
  first = True
  while not (x1==x2 and y1==y2):
    if not first:
      res.append((x1, y1))
    first = False
    e2 = err+err
    if e2 > -dy:
      err -= dy
      x1 += sx
    if e2 < dx:
      err += dx
      y1 += sy
  return tuple(res)

def los2(pos1, pos2):
  fwd = los(pos1, pos2)
  rev = los(pos2, pos1)
  if fwd == rev[::-1]:
    return fwd + ((0, 0), (0, 0))
  else:
    return fwd + ((0, 0),) + rev + ((0, 0),)

for j in range(-4, 5):
  for i in range(-4, 5):
    dist = math.ceil(math.sqrt(i*i+j*j))
    if 0 < dist <= 4:
      a = los2((0, 0), (i, j))
      # a = [y*16+x for x, y in a]
      print(i, j, a)
  print()
