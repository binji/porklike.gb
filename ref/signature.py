# Carve
# sigs = [255,214,124,179,233]
# msks = [0,9,3,12,6]

# Door
# sigs = [192,48]
# msks = [15,15]

# Freestanding
# sigs=[0,0,0,0,16,64,32,128,161,104,84,146]
# msks=[8,4,2,1,6,12,9,3,10,5,10,5]

# Walls
sigs=[251,233,253,84,146,80,16,144,112,208,241,248,210,177,225,120,179,0,124,104,161,64,240,128,224,176,242,244,116,232,178,212,247,214,254,192,48,96,32,160,245,250,243,249,246,252]
msks=[0,6,0,11,13,11,15,13,3,9,0,0,9,12,6,3,12,15,3,7,14,15,0,15,6,12,0,0,3,6,12,9,0,9,0,15,15,7,15,14,0,0,0,0,0,0]

l = len(sigs)

rows = [[0, 5, 3], [7, 8, 6], [1, 4, 2]]

for i in range(len(sigs)):
  s = f'{sigs[i]}/{msks[i]}'
  s += ' ' * (8 - len(s))
  print(s, end='')

print('\n' + '-----' * l + '---' * (l-1))

for row in rows:
  s = ''
  for i in range(len(sigs)):
    for col in row:
      sig = sigs[i]
      msk = msks[i]
      if msk & (1<<col):
        s += '* '
      elif sig & (1<<col):
        s += '1 '
      else:
        s += '0 '
    s += '  '
  print(s)

for i in range(len(sigs)):
  sigs[i] |= msks[i]
print(sigs)
