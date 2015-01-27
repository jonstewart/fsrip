#!/opt/local/bin/python3.3

# {"id":"0000", 
#   "t":{ 
#     "fsmd":{ 
#       "fs":{"byteOffset":1048576,"blockSize":512,"fsID":"f7c7b628","partName":"part-0-0"},
#       "path":"", 
#       "name":{"flags":"Allocated","meta_addr":4,"meta_seq":0,"name":"Documents","shrt_name":"DOCUME~1","type":"Folder"},
#       "meta":{"addr":4,"accessed":"2012-08-07T04:00:00Z","content_len":8,"created":"2012-08-07T22:13:53.84Z","metadata":"1970-01-01T00:00:00Z","flags":"Allocated, Used","gid":0,"mode":511,"modified":"2012-08-07T22:13:52Z","nlink":1,"seq":0,"size":8192,"type":"Folder","uid":0, 
#        "attrs":[
#           {"flags":"In Use, Non-resident","id":0,"name":"","size":8192,"type":1,"rd_buf_size":0,"nrd_allocsize":8192,"nrd_compsize":0,"nrd_initsize":8192,"nrd_skiplen":0,
#             "nrd_runs":[
#               {"addr":3880,"flags":0,"len":8,"offset":0}, 
#               {"addr":618808,"flags":0,"len":8,"offset":8}
#             ]
#           }
#         ]
#       }
#     } 
#   } 
# }


import sys
import json

input = sys.stdin

for l in input:
  line = l.strip()
  metadata = json.loads(line)
  if 'id' not in metadata:
    print("** no id on %s" % (line), file=sys.stderr)

  id = metadata['id']

  if 't' not in metadata:
    print("** no t on %s" % (line), file=sys.stderr)

  t = metadata['t']

  if 'fsmd' not in t:
    print("** no fsmd in t on %s" % (line), file=sys.stderr)

  fsmd = t['fsmd']

  if 'path' not in fsmd:
    print("** no path in fsmd on %s" % (line), file=sys.stderr)

  if 'name' not in fsmd or 'name' not in fsmd['name']:
    print("** no name in fsmd on %s" % (line), file=sys.stderr)

  print("%s\t%s" % (id, fsmd['path'] + fsmd['name']['name']))
