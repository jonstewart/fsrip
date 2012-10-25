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

### Usage:

    fsrip <command> <evidence file segments>

Evidence file segments must be specified in the correct order. As long as the 
evidence file segments are in lexicographic order, the following Bash fu will 
work, using a wild card for the segment number:

  	fsrip count `ls path/to/image.E*`

### Commands:

- *info*    
> output one JSON record concerning the disk image, its volume system, and 
volumes and filesystems, but not directory entries.

- *count*
> output the total number of directory entries discovered in the disk image.

- *dumpfs*
> output one JSON record per line, detailing all metadata for a directory 
entry, for all directory entries discovered in the disk image.

- *dumpfiles*
> output a JSON record of file metadata with newline, followed by the size of
the file contents (8 bytes in binary), followed by the file contents, with
slack, and then repeated again for the next file until all files have been
output. extract.py and hasher.py are examples of python scripts which can read
this output.

- *dumpimg*
> output entire disk image to stdout

### Dependencies:

fsrip depends on the [Boost C++ library](http://www.boost.org) and the 
[Sleuthkit](http://www.sleuthkit.org), and uses [SCons](http://www.scons.org)
as a build tool. The build script will also build fsrip with [libewf]
(http://sourceforge.net/projects/libewf/) and [afflib]
(http://digitalcorpora.org/downloads/) support, though these are technically 
optional dependencies. libewf and afflib have their own dependencies, the most
notable being zlib and libcrypto.

You can then build fsrip by typing:

    scons

and you should see output like this:

    scons: Reading SConscript files ...
    System is Darwin-11.4.2-x86_64-i386-64bit, x86_64
    CC = clang, CXX = clang++
    Checking for C library boost_program_options... (cached) yes
    Checking for C library tsk3... (cached) yes
    Checking for C library afflib... (cached) yes
    Checking for C library libewf... (cached) yes
    scons: done reading SConscript files.
    scons: Building targets ...
    clang++ -o build/src/main.o -c -g -stdlib=libc++ -Wall -Wno-trigraphs -Wextra -O3 -std=c++11 -Wnon-virtual-dtor src/main.cpp
    clang++ -o build/src/tsk.o -c -g -stdlib=libc++ -Wall -Wno-trigraphs -Wextra -O3 -std=c++11 -Wnon-virtual-dtor src/tsk.cpp
    clang++ -o build/src/walkers.o -c -g -stdlib=libc++ -Wall -Wno-trigraphs -Wextra -O3 -std=c++11 -Wnon-virtual-dtor src/walkers.cpp
    clang++ -o build/src/fsrip -stdlib=libc++ build/src/main.o build/src/tsk.o build/src/walkers.o -ltsk3 -lboost_program_options -lafflib -lewf
    scons: done building targets.

and you can then run fsrip like so:

    ./build/src/fsrip --help
    
    fsrip, Copyright (c) 2010-2012, Lightbox Technologies, Inc.
    Built Oct 25 2012
    TSK version is 4.0.0
    Allowed Options:
      --help                produce help message
      --command arg         command to perform [info|dumpimg|dumpfs|dumpfiles|
                            count]
      --ev-files arg        evidence files
