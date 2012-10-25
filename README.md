fsrip
=====

Copyright 2010-2012, Lightbox Technologies, Inc

About:
------

fsrip is a simple utility for extracting volume and filesystem information 
from disks and disk images in bulk. fsrip uses The Sleuthkit 
(http://www.sleuthkit.org/sleuthkit) to do the heavy lifting.

fsrip is released under version 2 of the Apache license. See LICENSE.txt for 
details.

### Dependencies:

fsrip depends on the Boost C++ library (http://www.boost.org) and the 
Sleuthkit (http://www.sleuthkit.org), and uses SCons (http://www.scons.org) as 
a build tool. The build script will also build fsrip with libewf and afflib 
support, though these are technically optional dependencies. libewf and afflib 
have their own dependencies, the most notable being zlib and libcrypto.

### Usage:

    fsrip <command> <evidence file segments>

Evidence file segments must be specified in the correct order. As long as the 
evidence file segments are in lexicographic order, the following Bash fu will 
work, using a wild card for the segment number:

  	fsrip count `ls path/to/image.E*`

### Commands:

  info    - output one JSON record concerning the disk image, its volume 
            system, and volumes and filesystems, but not directory entries.
  
  count   - output the total number of directory entries discovered in the 
            disk image.
  
  dumpfs  - output one JSON record per line, detailing all metadata for a 
            directory entry, for all directory entries discovered in the disk 
            image.

  dumpfiles - output a JSON of file metadata (with newline) followed by 
            binary of the file contents, then JSON record for next file, and
      			so on.

  dumpimg - output entire disk image to stdout
