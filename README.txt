fsrip

README

fsrip is a simple utility for extracting volume and filesystem information from disks and disk images in bulk, export. fsrip uses The Sleuthkit (http://www.sleuthkit.org/sleuthkit) to do the heavy lifting.

fsrip is released under version 2 of the Apache license. See LICENSE.txt for details.

fsrip depends on the Boost C++ library (http://www.boost.org) and the Sleuthkit (http://www.sleuthkit.org), and uses SCons (http://www.scons.org) as a build tool. The build script will also build fsrip with libewf and afflib support, though these are technically optional dependencies. libewf and afflib have their own dependencies, the most notable being zlib and libcrypto.
