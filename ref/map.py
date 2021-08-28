rows = []
with open('map.txt', 'r') as f:
  for line in f.readlines():
    if line[-1] == '\n': line = line[:-1]
    row = []
    while line:
      tile, line = line[:2], line[2:]
      row.append(int(tile, 16))
    rows.append(row)

with open('../tiles-bg.tilemap', 'rb') as f:
  tilemap = f.read()

maps = []
for my in range(2):
  for mx in range(8):
    mp = []
    for ty in range(16):
      for tx in range(16):
        mp.append(tilemap[rows[my * 16 + ty][mx * 16 + tx]])
    maps.append(mp)

with open('../map.bin', 'wb') as f:
  for mp in maps:
    f.write(bytes(mp))
