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

msgs = [
#  0123456789abcd
  'JUMP 2 SPACES',
  'STOP ENEMIES',
  'YOU SKIP OVER',

  'RANGED ATTACK',
  'DO 1 DAMAGE',
  'STOP ENEMY',

  'RANGED ATTACK',
  'PUSH ENEMY',
  '1 SPACE AND',
  'STOP THEM',

  'PULL YOURSELF',
  'UP TO THE NEXT',
  'OCCUPIED SPACE',

  'HIT 2 SPACES',
  'IN ANY',
  'DIRECTION',

  'SMASH A WALL',
  'OR DO 2',
  'DAMAGE',

  'PULL AN ENEMY',
  '1 SPACE TOWARD',
  'YOU AND STOP',
  'THEM',

  'HIT 4 SPACES',
  'AROUND YOU',

  'LIFT AND THROW',
  'AN ENEMY',
  'BEHIND YOU AND',
  'STOP THEM',

  'HIT AN ENEMY',
  'AND HOP',
  'BACKWARD',
  'STOP ENEMY',
]

chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789()'
first = 0xcb

def find(c):
  if c == ' ': return 0
  return chars.index(c) + first

total = 0
start = []
lens = []
for msg in msgs:
  tiles = []
  for c in msg:
    tiles.append(find(c))
  start.append(total)
  lens.append(len(tiles))
  total += len(tiles)
  print(f'// {msg}')
  print(f"{', '.join(map(str, tiles))},")

print(start)
print(lens)
