import math

speed = 6
y = 40
desty = 128
a = []

while desty - y > 1:
  y += (desty - y) / speed
  a.append(math.ceil(y))

print(f"#{len(a)} {a[::-1]}")

y = 128
desty = 40
a = []

while y - desty > 1:
  y -= (y - desty) / speed
  a.append(math.floor(y))

print(f"#{len(a)} {a[::-1]}")
