import math

# distance table
for j in range(16):
  for i in range(16):
    print(f'{math.ceil(math.sqrt(i*i+j*j)):2},', end='')
  print()

# distance <= 4
a = []
for j in range(-15, 16):
  for i in range(-15, 16):
    dist = math.ceil(math.sqrt(i*i+j*j))
    if 0 < dist <= 4:
      print(f'{dist:2},', end='')
      a.append(((j * 16 + i + 256) % 256, i, j))
    else:
      print(f'  ,', end='')
  print()

for x, _, _ in a:
  print(f'0x{x:02x},', end='')
print()

# map coordinates for distance < 4
mp = {}
for n, x, y in a:
  mp[(x, y)] = n

# Build combined arrays for 4 tile sight
total = 0
start = []
count = []

for top in range(0, -9, -1):
  # The tile addend for the surrounding tiles
  print()
  cols = []
  for x in range(4, -5, -1):
    col = 0
    for y in range(top, top + 9):
      if (x, y) in mp:
        print(f'0x{mp[(x, y)]:02x},', end='')
        col += 1
    print()
    cols.append(col)

  # The start of the region
  n = 0
  start.extend([0, 0, 0, 0, 0])
  for i in range(0, 4):
    n += cols[i]
    start.append(n)

  # The count for this region
  n = sum(cols[0:4])
  for i in range(4, 9):
    n += cols[i]
    count.append(n)

  for i in range(0, 4):
    n -= cols[i]
    count.append(n)

print(start)
print(count)
