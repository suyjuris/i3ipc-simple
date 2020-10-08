#!/usr/bin/python3
# This updates README.md with the actual contents of the example files.

import os
import sys
import glob
import re

os.chdir(os.path.join(os.path.dirname(sys.argv[0]), '..'))

data = {}
flag_exit = False
for fname in glob.glob('examples/*.c'):
    with open(fname, 'r') as f: data[fname] = list(f)
    if not data[fname]:
        print('Warning: file %s is empty' % (fname,))
        flag_exit = True
    elif data[fname][0] != '/* ' + fname + ' */\n':
        print('Warning: first line of file %s does not match' % (fname,))
        flag_exit = True

if flag_exit: sys.exit(1)

f = open('README.md', 'r')
f_out = open('out_README.md', 'w')
blank = False
ignore = False
for line in f:
    line = line.rstrip()
    m = re.match(r'    /\* ([^ ]*) \*/', line)
    if blank and m:
        fname = m.group(1)
        if fname in data:
            for line in data[fname]: f_out.write('    ' + line)
            f_out.write('\n')
            ignore = True
        else:
            print('Warning: file %s not found' % (fname,))
    elif line and line[0] != ' ':
        ignore = False
    if not ignore:
        f_out.write(line + '\n')
    blank = not line
