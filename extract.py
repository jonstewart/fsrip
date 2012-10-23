#!/bin/python

import sys
import json
import re
import struct
import os

input = sys.stdin

def getHashSize(physicalSize, metadata):
  size = physicalSize
  if ('meta' in metadata):
    size = min(size, metadata['meta']['size'])
  return size

def getPath(metadata):
  return metadata['path'] + metadata['name']['name'], metadata['name']['name']

def exportFile(path, data):
  dir = os.path.dirname(path)
  if (False == os.path.exists(dir)):
    os.makedirs(dir)
  with open(path, 'w') as f:
    f.write(data)

search = re.compile(sys.argv[1])

line = input.readline()
filesRead = 0
while line:
  try:
    metadata = json.loads(line)
    path, name = getPath(metadata)
    sizeData = input.read(8)
    size = struct.unpack('<Q', sizeData)[0]
    data = input.read(size)

    size = getHashSize(size, metadata)
    if (None != search.search(path) and 5 == metadata['name']['type'] and not (name == '.' or name == '..')):
      exportFile(path, data[:size])
    line = input.readline()
  except struct.error as e:
    print("%s with len(sizeData) == %s on %s" % (e, str(len(sizeData)), path))
    raise
