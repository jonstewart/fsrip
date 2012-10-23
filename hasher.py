#!/bin/python

import sys
import json
import hashlib
import struct

def getHashSize(physicalSize, metadata):
  size = physicalSize
  if ('meta' in metadata):
    size = min(size, metadata['meta']['size'])
  return size

input = sys.stdin

line = input.readline()
filesRead = 0
while line:
  metadata = json.loads(line)

  sizeData = input.read(8)
  size = struct.unpack('<Q', sizeData)[0]
  data = input.read(size)

  size = getHashSize(size, metadata)
  hasher = hashlib.md5()
  hasher.update(data[:size])

  print("%s\t%s\t%s\t%s" % (metadata['path'], metadata['name']['name'], str(size), hasher.hexdigest()))
  filesRead += 1
  line = input.readline()
print("read %s files" % (filesRead))
