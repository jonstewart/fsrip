#!/usr/bin/python3 -b

import collections
import re
import sys

def flatten(l):
  while l:
    while l and isinstance(l[0], list):
      l[0:1] = l[0]
    if l:
      yield l.pop(0)

v = collections.defaultdict(list)

for line in sys.stdin:
  # expect var='val'
  (var, val) = line.split('=', 1)
  # strip leading and trailing quotes
  val = val[1:-2]

  # split val into literals and refs to other vars
  while val:
    m = re.match(r"(.*?)\$(?:\((\w+)\)|{(\w+)}|(\w+))(.*)", val)
    if m:
      if m.group(1):
        v[var].append(m.group(1))
      v[var].append(v[m.group(2) or m.group(3) or m.group(4)])
      val = m.group(5)
    else:
      v[var].append(val)
      break 

# dump it
for (var, val) in sorted(v.items()):
  print('CONFIG_{}="{}"'.format(var, ''.join(flatten(val))))

