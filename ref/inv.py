import math

y = 40
desty = 128
a = []

while desty - y > 1:
  y += (desty - y) / 10
  a.append(math.ceil(y))

print(a[::-1])

y = 128
desty = 40
a = []

while y - desty > 1:
  y -= (y - desty) / 10
  a.append(math.floor(y))

print(a[::-1])
