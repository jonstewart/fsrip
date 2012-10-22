#!/bin/python

import sys
import json
import hashlib

input = sys.stdin


line = input.readline()
filesRead = 0
while line:
  metadata = json.loads(line)
  size = metadata['meta']['size']
  data = input.read(size)
  hasher = hashlib.md5()
  hasher.update(data)
  print("%s\t%s\t%s\t%s" % (metadata['path'], metadata['name']['name'], str(size), hasher.hexdigest()))
  filesRead += 1
  line = input.readline()
print("read %s files" % (filesRead))
