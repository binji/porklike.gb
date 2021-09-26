import math

# from easings.net/#easeOutBounce
def bounce(x):
  n1 = 7.5625
  d1 = 2.75

  if x < 1 / d1:
    return n1 * x * x
  elif x < 2 / d1:
    x -= 1.5 / d1
    return n1 * x * x + 0.75
  elif x < 2.5 / d1:
    x -= 2.25 / d1
    return n1 * x * x + 0.9375
  else:
    x -= 2.625 / d1
    return n1 * x * x + 0.984375

# https://easings.net/#easeOutElastic
def elastic(x):
  c4 = 2 * math.pi / 3

  if x == 0:
    return 0
  elif x == 1:
    return 1
  else:
    return math.pow(2, -10 * x) * math.sin((x * 10 - 0.75) * c4) + 1

# https://easings.net/#easeOutCubic
def cubic(x):
  return 1 - math.pow(1 - x, 3)

def lerp(a, t, b):
  return a + t * (b - a)

def rnd(x):
  return (256 + round(x)) & 255


# drop PORKLIKE bounce
TOP = 112
MIDDLE = -64
FRAMES = 60
a = []
for f in range(FRAMES+1):
  t = f / FRAMES
  a.append(rnd(lerp(TOP, bounce(t), MIDDLE)))

print('bounce:')
print(', '.join(map(str, a)))

# slide right elastic
OBJ_X_OFFSET = 8
RIGHT = 160 + OBJ_X_OFFSET
MIDDLE = 56 + OBJ_X_OFFSET
FRAMES = 60
a = []
for f in range(FRAMES+1):
  t = f / FRAMES
  a.append(rnd(lerp(RIGHT, elastic(t), MIDDLE)))

print('elastic:')
print(', '.join(map(str, a)))

# slide up cubic
BOTTOM = 0
TOP = -48
FRAMES = 45
a = []
for f in range(FRAMES+1):
  t = f / FRAMES
  a.append(rnd(lerp(BOTTOM, cubic(t), TOP)))

print('cubic:')
print(', '.join(map(str, a)))
