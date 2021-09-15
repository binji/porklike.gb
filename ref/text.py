msgs = [
'JUMP (2)',
'BOLT (2)',
'PUSH (2)',
'GRAPPLE (2)',
'SPEAR (2)',
'SMASH (2)',
'HOOK (2)',
'SPIN (2)',
'SUPLEX (2)',
'SLAP (2)',
]

chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789()'
first = 0xcb

def find(c):
  if c == ' ': return 255
  return chars.index(c) + first

total = 0
start = []
for msg in msgs:
  tiles = []
  for c in msg:
    tiles.append(find(c))
  start.append(total)
  total += len(tiles)
  print(msg, tiles)

print(start)
