ty = 0
y = 10
t = 0
ly = y

a = []
while t < 70:
  y += (ty - y) / 10
  t += 1
  a.append(ly - int(y))
  ly = int(y)

print(a[::-1])
