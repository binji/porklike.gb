#!/usr/bin/env python
import argparse
import os
import sys

def main(args):
  parser = argparse.ArgumentParser()
  parser.add_argument('-o', '--out')
  parser.add_argument('-b', '--bank')
  parser.add_argument('file')
  options = parser.parse_args(args)
  name, _ = os.path.splitext(os.path.basename(options.file))

  with open(options.file, 'rb') as inf:
    data = inf.read()

  datalen = len(data)
  with open(options.out, 'w') as outf:
    outf.write('#include <stdint.h>\n')
    if options.bank:
      outf.write(f'#pragma bank {options.bank}\n')
    outf.write(f'const unsigned char {name}_bin[] = {{\n')
    while data:
      outf.write('  ')
      row, data = data[:16], data[16:]
      for b in row:
        outf.write(f'{b:3}, ')
      outf.write('\n')
    outf.write(f'}};\n')

  header = os.path.splitext(options.out)[0] + '.h'
  guard = os.path.splitext(os.path.basename(options.out))[0].upper() + '_H_'
  with open(header, 'w') as outf:
    outf.write(f'#ifndef {guard}\n')
    outf.write(f'#define {guard}\n')
    outf.write(f'#define {name.upper()}_LEN {datalen}\n')
    outf.write(f'extern const unsigned char {name}_bin[];\n')
    outf.write(f'#endif // {guard}\n')


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
