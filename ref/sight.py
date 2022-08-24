import math
import pprint

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

def PrintDepthFirstLosTraversal():
  paths = []
  for j in range(-4, 5):
    for i in range(-4, 5):
      dist = math.ceil(math.sqrt(i*i+j*j))
      if 0 < dist <= 4:
        fwd = los((0, 0), (i, j))
        rev = los((i, j), (0, 0))[::-1]
        if fwd != rev:
          paths.append(fwd + ((i, j),))
          paths.append(rev + ((i, j),))
        else:
          paths.append(fwd + ((i, j),))

  # pprint.pp(paths)

  def recurse(index, curpath):
    total = []
    count = []
    for pos in sorted(set([path[index] for path in paths
                          if len(path) > index and path[:index] == curpath])):
      total.append(pos)
      count.append(len(total))
      p = len(count) - 1
      t, c = recurse(index + 1, curpath + (pos,))
      total.extend(t)
      count.extend(c)
      count[p] = len(total) - count[p]
    return total, count

  def h(n):
    return hex((n + 16) & 15)[2:]

  t, c = recurse(0, ())
  print('const u8 sightdiff[] = {{{}}};'.format(', '.join(f'0x{h(x[1])}{h(x[0])}' for x in t)))
  print('const u8 sightskip[] = {{{}}};'.format(', '.join(map(str, c))))


PrintDepthFirstLosTraversal()
