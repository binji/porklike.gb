import copy

notenames = [
  'c-0','c#-0','d-0','d#-0','e-0','f-0','f#-0','g-0','g#-0','a-0','a#-0','b-0',
  'c-1','c#-1','d-1','d#-1','e-1','f-1','f#-1','g-1','g#-1','a-1','a#-1','b-1',
  'c-2','c#-2','d-2','d#-2','e-2','f-2','f#-2','g-2','g#-2','a-2','a#-2','b-2',
  'c-3','c#-3','d-3','d#-3','e-3','f-3','f#-3','g-3','g#-3','a-3','a#-3','b-3',
  'c-4','c#-4','d-4','d#-4','e-4','f-4','f#-4','g-4','g#-4','a-4','a#-4','b-4',
  'c-5','c#-5','d-5','d#-5',
]

notefreq = [
    65.27,   68.97,   73.35,   77.72,   82.09,   87.14,   92.19,   97.90,  103.63,  109.68,  116.42,  122.81,
   130.55,  138.28,  146.69,  155.45,  164.53,  174.29,  184.71,  195.82,  207.59,  219.71,  232.83,  245.95,
   261.43,  276.90,  293.38,  310.89,  329.39,  348.91,  369.77,  391.97,  415.18,  439.75,  466.00,  491.91,
   522.86,  553.81,  586.79,  621.76,  658.79,  697.81,  739.87,  783.94,  830.72,  879.85,  931.99,  983.82,
  1045.72, 1107.61, 1173.56, 1243.56, 1317.58, 1395.62, 1479.74, 1567.88, 1661.43, 1759.67, 1863.97, 1967.59,
  2091.41, 2215.22, 2347.12, 2487.07,
]

def duty_freq(note):
  return round(2048 - 131072 / notefreq[note])

def wave_freq(note):
  return round(2048 - 65536 / notefreq[note])


def chomp(s):
  if s[-1] == '\n':
    return s[:-1]
  return s


def readsound():
  pats = []
  with open('sound.txt', 'r') as f:
    for line in f:
      line = chomp(line)

      editor = int(line[0:2], 16)
      speed = int(line[2:4], 16)
      loops = int(line[4:6], 16)
      loope = int(line[6:8], 16)
      line = line[8:]

      notes = []
      while line:
        pitch = int(line[0:2], 16)
        wave = int(line[2], 16)
        volume = int(line[3], 16)
        effect = int(line[4], 16)
        notes.append((pitch, wave, volume, effect))
        line = line[5:]

      pats.append({
        'editor':editor,
        'speed':speed,
        'loops':loops,
        'loope':loope,
        'notes':notes,
      })
  return pats


def printsounds(pats):
  print('patterns:')
  for i, pat in enumerate(pats):
    print(f"{i:2}: spd:{pat['speed']:2}", end=' ')

    for o in range(4):
      for note in pat['notes'][o * 8:o * 8 + 8]:
        if note[2] == 0:
          print(f".......", end='|')
        else:
          print(f"{notenames[note[0]]:4}{note[1]}{note[2]}{note[3]}", end='|')
      print()
      print(' '*11, end='')
    print()


def readmusic():
  def getpat(n):
    pat = int(n, 16)
    if pat >= 64:
      return None
    return pat

  seqs = []
  with open('music.txt', 'r') as f:
    for line in f:
      line = chomp(line)
      byte = int(line[1])
      loops = (byte & 1) != 0
      loope = (byte & 2) != 0
      end = (byte & 4) != 0
      pat1 = getpat(line[3:5])
      pat2 = getpat(line[5:7])
      pat3 = getpat(line[7:9])
      pat4 = getpat(line[9:11])
      seqs.append({
        'loops': loops,
        'loope': loope,
        'end': end,
        'pats': [pat1, pat2, pat3, pat4]
      })
  return seqs


def printmusic(seqs):
  print('music:')
  for i, seq in enumerate(seqs):
    loops = '<' if seq['loops'] else ' '
    loope = '>' if seq['loope'] else ' '
    end = 'x' if seq['end'] else ' '
    print(f"{i:2}: {loops}{loope}{end}", end=' ')

    for pat in seq['pats']:
      print(f"{pat if pat else '..'} ", end='')

    print()


def getused(col):
  insused = []
  for pat in sounds:
    ins = set()
    for note in pat['notes']:
      if note[2] != 0:
        ins.add(note[col])
    insused.append(ins)
  return insused


def printused(usedlist):
  print('used:')
  for i, used in enumerate(usedlist):
    if used:
      print(f"{i:2}: {sorted(list(used))}")


sounds = readsound()
music = readmusic()

printsounds(sounds)
printmusic(music)

# get instruments used in each pattern
insused = getused(1)
effectused = getused(3)

# get music patterns
musicpat = set()
for seq in music:
  for pat in seq['pats']:
    if pat is not None and insused[pat]:
      musicpat.add(pat)

# calc which patterns are sound effects (and not unused)
soundpat = set()
for i, used in enumerate(insused):
  if used and i not in musicpat:
    soundpat.add(i)

# print instruments for each pattern
# printused(insused)
# printused(effectused)

if False:
# figure out which instruments are used for each tick of the music
  musicins = []
  for seq in music:
    seqins = []
    for tick in range(32):
      tickins = []
      for pat in seq['pats']:
        if pat is not None:
          note = sounds[pat]['notes'][tick]
          if note[2] != 0:
            tickins.append(note[1])
      seqins.append(tickins)
    musicins.append(seqins)

  print('music tick instruments used:')
  for i, seq in enumerate(musicins):
    print(f"{i:2}: ", end='')
    for tick in seq:
      print(f"{''.join(map(str, sorted(tick))):3},", end='')
    print()

DUTY_12_5 = 0
DUTY_25 = 1
DUTY_50 = 2
DUTY_75 = 3

GB_DUTY1 = 0
GB_DUTY2 = 1
GB_WAVE = 2
GB_NOISE = 3

INS_TRIANGLE = 0
INS_TILTED_SAW = 1
INS_SAW = 2
INS_ORGAN = 5
INS_PHASER = 7

FX_VIBRATO = 2
FX_DROP = 3
FX_FADEIN = 4
FX_FADEOUT = 5
FX_ARPFAST = 6

# Used combinations
# (0, 0),  triangle
# (0, 2),  triangle vibrato
# (0, 4),  triangle fadein
# (0, 5),  triangle fadeout
# (0, 6),  triangle arpfast
# (1, 0),  tiltsaw
# (1, 2),  tiltsaw vibrato
# (1, 3),  tiltsaw drop
# (1, 4),  tiltsaw fadein
# (1, 5),  tiltsaw fadeout
# (2, 0),  saw
# (2, 5),  saw fadeout
# (5, 0),  organ
# (5, 2),  organ vibrato
# (5, 4),  organ fadein
# (5, 5),  organ
# (7, 0),  phaser
# (7, 2),  phaser vibrato
# (7, 4),  phaser fadein
# (7, 5),  phaser fadeout
# (7, 6)   phaser arpfast

ins_to_duty = {
  INS_TRIANGLE: DUTY_25,
  INS_TILTED_SAW: DUTY_12_5,
  INS_SAW: DUTY_12_5,
  INS_ORGAN: DUTY_50,
  INS_PHASER: DUTY_75,
}

NR10 = 0x10  # sweep    0sssdnnn (s=sweep time, d=down, n=count)
NR11 = 0x11  # duty     ddllllll (d=duty, l=length)
NR12 = 0x12  # vol env  vvvvunnn (v=vol, u=up, n=count)
NR13 = 0x13  # freq lo  ffffffff
NR14 = 0x14  # freq hi  tl000fff (t=trigger, l=use length, f=freq)
NR21 = 0x16  # duty     ddllllll (d=duty, l=length)
NR22 = 0x17  # vol env  vvvvunnn (v=vol, u=up, n=count)
NR23 = 0x18  # freq lo  ffffffff
NR24 = 0x19  # freq hi  tl000fff (t=trigger, l=use length, f=freq)
NR30 = 0x1a  # enable   e0000000 (e=enable)
NR31 = 0x1b  # length   llllllll
NR32 = 0x1c  # vol      0vv00000 (v=vol, 0=mute, 1=100%, 2=50%, 3=25%)
NR33 = 0x1d  # freq lo  ffffffff
NR34 = 0x1e  # freq hi  tl000fff (t=trigger, l=use length, f=freq)
NR41 = 0x20  # length   00llllll
NR42 = 0x21  # vol env  vvvvunnn (v=vol, u=up, n=count)
NR43 = 0x22  # freq     ffffwrrr (f=freq, w=width, r=ratio)
NR44 = 0x23  # trigger  tl000000 (t=trigger, l=use length)
WAVERAM = 0x30

# *2 for now, but maybe want to tweak
pulse_vol = {
    1: 2,
    2: 4,
    3: 6,
    4: 8,
    5: 10,
    6: 12,
    7: 14,
}

wave_vol = {
    1: 1,
    2: 1,
    3: 2,
    4: 2,
    5: 2,
    6: 3,
    7: 3,
}

def get_NRX1(duty):
  return duty << 6

def get_NRX2(vol, up, count):
  return (vol << 4) | (int(up) << 3) | count

def get_NRX3(freq):
  return freq & 255

def get_NRX4(trigger, freq):
  return (int(trigger) << 7) | (freq >> 8)

def get_NR30(enable):
  return int(enable) << 7

def get_NR32(vol):
  return [0, 2, 3, 1][vol] << 5  # ordered as 0%, 100%, 50%, 25%


if False:
  def get_note_for_gb(note, gbch):
    if gbch in [GB_DUTY1, GB_DUTY2]:
      freq = duty_freq(note[0])
      vol = pulse_vol[note[2]]

      # TODO drop fx
      nrx0 = 0
      nrx1 = get_NRX1(ins_to_duty[note[1]])
      if note[3] == FX_FADEIN:
        nrx2 = get_NRX2(1, True, 3)  # TODO: figure out env count
      elif note[3] == FX_FADEOUT:
        nrx2 = get_NRX2(vol, False, 3)  # TODO: figure out env count
      else:
        nrx2 = get_NRX2(vol, False, 0)

      nrx3 = get_NRX3(freq)
      nrx4 = get_NRX4(True, freq)

      if gbch == GB_DUTY1:
        return (nrx0, nrx1, nrx2, nrx3, nrx4)
      else:
        return (nrx1, nrx2, nrx3, nrx4)

    elif gbch == GB_WAVE:
      freq = wave_freq(note[0])
      vol = wave_vol[note[2]]

      nr30 = get_NR30(True)
      nr31 = 0
      nr32 = get_NR32(vol)
      nr33 = get_NRX3(freq)
      nr34 = get_NRX4(True, freq)
      return (nr30, nr31, nr32, nr33, nr34)

    else:  # GB_NOISE
      # TODO
      return (0, 0, 0, 0)

  for i, freq in enumerate(notefreq):
    freq = wave_freq(i)
    print(i, f'{notenames[i]:4}', freq, freq, hex(freq & 0xff), hex(freq >> 8))


  def get_writes_for_music_seq(assign, seq):
    notes = set()
    for ipat, pat in enumerate(seq['pats']):
      if pat is not None:
        for note in sounds[pat]['notes']:
          if note[2] != 0:
            gbch = assign[ipat]
            note = get_note_for_gb(note, gbch)
            notes.add((gbch, note))

    return notes

  assign = [GB_WAVE, GB_DUTY1, GB_DUTY2, GB_NOISE]

  writes = get_writes_for_music_seq(assign, music[0])


names = [
  'c0','cs0','d0','ds0','e0','f0','fs0','g0','gs0','a0','as0','b0',
  'c1','cs1','d1','ds1','e1','f1','fs1','g1','gs1','a1','as1','b1',
  'c2','cs2','d2','ds2','e2','f2','fs2','g2','gs2','a2','as2','b2',
  'c3','cs3','d3','ds3','e3','f3','fs3','g3','gs3','a3','as3','b3',
  'c4','cs4','d4','ds4','e4','f4','fs4','g4','gs4','a4','as4','b4',
  'c5','cs5','d5','ds5',
]

# CHANNEL3 stuff ##############################################################

ch3_pats = [34, 38, 42, 43, 50, 51, 52, 53]
ch3_vol = [25, 25, 50, 50, 50, 100, 100, 100]
fade_time = {25: 7, 50: 14, 100: 20}

print('-'*80)
used = set()
for pat in ch3_pats:
  speed = sounds[pat]['speed']
  last_freq = None

  print(f'seq{pat}::')
  i = 0
  notes = sounds[pat]['notes']
  while i < 32:
    note = notes[i]
    time = 0
    # note = [freq, ins, vol, effect]
    if note[2] != 0:
      vol = ch3_vol[note[2]]
      if note[0] != last_freq:
        print(f'  NOTE ch3_{names[note[0]]}')
        last_freq = note[0]
        used.add(note[0])

      if note[3] in [4, 5]:  # fade out (TODO fadein)
        time += fade_time[vol]
        # Find how many rests after this note
        delay = speed - time
        i += 1
        while i < 32 and notes[i][2] == 0:
          delay += speed
          i += 1

        comment = ''
        if note[3] == 4:
          comment = '  ; TODO: fadein'

        if delay == 0:
          print(f'  FADE{vol}_NODELAY{comment}')
        else:
          print(f'  FADE{vol}, {delay}{comment}')
      elif note[3] == 0:   # no effect
        # Find how many rests after this note
        delay = speed - time
        i += 1
        while i < 32 and notes[i][2] == 0:
          delay += speed
          i += 1

        print(f'  WAIT {delay}')
      else:
        print(f'effect {note[3]}')
        raise NotImplemented()
    else:
      print(f'  WAIT {speed}')
  print(f'  RETN')

print()
for note in used:
  freq = wave_freq(note)
  lo = freq & 255
  hi = (freq >> 8) | 0x80
  print(f'CH3_NOTE ch3_{names[note]:3} 0x{lo:02x} 0x{hi:02x}')

# CHANNEL1 stuff ##############################################################

ch_pats = [
    [37, 41, 48, 49, 35, 39, 44, 45],
    [26, 27, 28, 29, 36, 40, 46, 47, 30, 31, 32, 33, 54, 55, 56, 57],
]

def ch12_name(ch, note):
  vol = note[2]
  effect = {
    0: 'hold',
    2: 'fade',
    4: 'fade',
    5: 'fade',
    6: 'fade',
  }[note[3]]
  name = names[note[0]]
  return f'ch{ch + 1}_{name}_{vol}_{effect}'

def env_name(note):
  vol = note[2]
  effect = {
    0: 'HOLD',
    2: 'FADE',
    4: 'FADE',
    5: 'FADE',
    6: 'FADE',
  }[note[3]]
  return f'ENV_{vol}{effect}'

print()
used = set()
for ch in range(2):
  for pat in ch_pats[ch]:
    speed = sounds[pat]['speed']
    last_freq = None

    print(f'seq{pat}::')
    i = 0
    notes = sounds[pat]['notes']
    while i < 32:
      note = notes[i]
      time = 0
      # note = [freq, ins, vol, effect]
      if note[2] != 0:
        # (TODO vibrato, fadein, arpeggio)
        if note[3] in [0, 2, 4, 5, 6]:  # hold, fade out
          # HACK remap effect number
          effect = {
            0: 0,
            2: 5,
            4: 5,
            5: 5,
            6: 5,
          }[note[3]]

          used.add(((note[0], 0, note[2], effect), ch))

          # Find how many rests after this note
          delay = speed - time
          note_delay = delay
          i += 1
          while i < 32 and notes[i][2] == 0:
            delay += speed
            i += 1

          comment = ''
          if note[3] == 2:
            comment = '  ; TODO: vibrato'
          elif note[3] == 4:
            comment = '  ; TODO: fadein'
          elif note[3] == 6:
            comment = '  ; TODO: arpreggio'

          name = ch12_name(ch, note)

          assert delay != 0
          if note[3] == 0 and i == 32:
            # If holding, and this is the last note of the section, cut the
            # note
            print(f'  NTWT {name}, {note_delay} {comment}')
            delay -= note_delay
            if delay == 0:
              print(f'  NOTE ch{ch + 1}_mute')
            else:
              print(f'  NTWT ch{ch + 1}_mute, {delay}')

          else:
            print(f'  NTWT {name}, {delay} {comment}')
        else:
          print(f'effect {note[3]}')
          raise NotImplemented()
      else:
        # Find how many rests after this note
        delay = speed
        i += 1
        while i < 32 and notes[i][2] == 0:
          delay += speed
          i += 1
        print(f'  WAIT {delay}')
    print(f'  RETN')
  print()

seen_note = set()
for note, ch in sorted(used):
  if note[0] not in seen_note:
    seen_note.add(note[0])
    generic = f'ch12_{names[note[0]]}'
    freq = duty_freq(note[0])
    lo = freq & 255
    hi = (freq >> 8) | 0x80
    print(f'CH12_NOTE    {generic:8} 0x{lo:02x} 0x{hi:02x}')

  name = ch12_name(ch, note)
  env = env_name(note)
  generic = f'ch12_{names[note[0]]}'
  print(f'CH{ch + 1}_NOTE_ENV {name:14} {env} {generic}')
