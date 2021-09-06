flags = []
with open('flags.txt', 'r') as f:
  for line in f.readlines():
    if line[-1] == '\n': line = line[:-1]
    while line:
      tile, line = line[:2], line[2:]
      flags.append(int(tile, 16))

with open('../bg.tilemap', 'rb') as f:
  tilemap = f.read()

new_flags = []
for i in range(len(flags)):
  j = tilemap[i]
  if j >= len(new_flags):
    new_flags.extend([0] * (j + 1 - len(new_flags)))
  new_flags[j] = flags[i]

with open('../flags.bin', 'wb') as f:
  f.write(bytes(new_flags))
