import math

def output(name, a):
  s = ', '.join(map(str, a[::-1]))
  print(f"const u8 {name}[] = {{{s}}};")

def down(speed, y, desty, a=None):
  if a is None: a = []
  while desty - y > 1:
    y += (desty - y) / speed
    a.append(math.ceil(y))
  return a

def up(speed, y, desty, a=None):
  if a is None: a = []
  while y - desty > 1:
    y -= (y - desty) / speed
    a.append(math.floor(y))
  return a

output('inventory_up_y', up(6, 128, 40))
output('inventory_down_y', down(6, 40, 128))

output('restart_menu_up_y', up(5, 144, 60, down(5, 128, 144)))
output('restart_menu_down_y', up(5, 144, 128, down(5, 60, 144)))
