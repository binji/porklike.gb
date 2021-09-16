import math

time = 8
height = 12
ly = 0
for i in range(1, time + 1):
  y = height*-math.sin(i * 2 * math.pi / time / 2)
  print(int(y) - ly, end= ' ')
  ly = int(y)
print()
