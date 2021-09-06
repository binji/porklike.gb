#!/usr/bin/env python
import argparse
import os
import sys

def main(args):
  parser = argparse.ArgumentParser()
  parser.add_argument('-t', '--tiles', action='store_true')
  parser.add_argument('-o', '--out')
  parser.add_argument('file')
  options = parser.parse_args(args)
  name, _ = os.path.splitext(os.path.basename(options.file))

  with open(options.file, 'rb') as inf:
    data = inf.read()

  with open(options.out, 'w') as outf:
    outf.write('#include <stdint.h>\n')
    outf.write(f'#define {name.upper()}_LEN {len(data)}\n')
    if options.tiles:
      outf.write(f'#define {name.upper()}_TILES {len(data)//16}\n')
    outf.write(f'const unsigned char {name}_bin[] = {{\n')
    while data:
      outf.write('  ')
      row, data = data[:16], data[16:]
      for b in row:
        outf.write(f'{b:3}, ')
      outf.write('\n')
    outf.write(f'}};\n')


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
