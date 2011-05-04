/*
src/deacon/tsk.h

Created by Jon Stewart on 2010-01-06.
Copyright (c) 2010 Lightbox Technologies, Inc. All rights reserved.
*/

#pragma once

#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <tsk3/libtsk.h>

#include <string>
#include <vector>

typedef unsigned long long uint64;
typedef long long int64;

using namespace boost;
using namespace std;

class Filesystem {
  friend class Volume;
  friend class Image;
public:
  typedef function< TSK_WALK_RET_ENUM (TSK_FS_FILE*, const char*, unsigned int) > DirWalkFn;

  ~Filesystem();

  uint64 numBlocks() const;
  unsigned int blockSize() const;
  unsigned int deviceBlockSize() const;
  string blockName() const;
  bool littleEndian() const;
  uint64 firstBlock() const;
  uint64 firstInum() const;
  unsigned int flags() const;  
  vector< unsigned char > fsID() const;
  string fsIDAsString() const;
  uint64 fsType() const;
  string fsName() const;
  bool isOrphanHunting() const;
  uint64 journalInum() const;
  uint64 numInums() const;
  uint64 lastBlock() const;
  uint64 lastBlockAct() const;
  uint64 lastInum() const;
  uint64 byteOffset() const;
  uint64 rootInum() const;

  bool walk(DirWalkFn callback) const;

private:
  Filesystem(TSK_FS_INFO* fs);

  TSK_FS_INFO* Fs;
};

class Volume {
  friend class VolumeSystem;
public:
  string desc() const;
  unsigned int flags() const;
  uint64 numBlocks() const;
  int64 slotNum() const;
  uint64 startBlock() const;
  int64 tableNum() const;

  weak_ptr< Filesystem > filesystem() const;

private:
  Volume(const TSK_VS_PART_INFO* vol);

  const TSK_VS_PART_INFO* Vol;
  shared_ptr< Filesystem > Fs;
};

class VolumeSystem {
friend class Image;
  
public:
  typedef vector< weak_ptr< Volume > >::const_iterator VolIter;
  
  ~VolumeSystem();

  unsigned int blockSize() const;
  uint64  offset() const;
  unsigned int numVolumes() const;
  unsigned int type() const;
  string desc() const;

  vector< weak_ptr< Volume > >::const_iterator volBegin() const;
  vector< weak_ptr< Volume > >::const_iterator volEnd() const;

  weak_ptr< Volume > getVol(unsigned int i) const;
  
private:
  VolumeSystem(TSK_VS_INFO* volInfo);

  vector< shared_ptr< Volume > > Volumes;
  vector< weak_ptr< Volume > > WeakVols;

  TSK_VS_INFO* VolInfo;
};

class Image {
public:
  static shared_ptr< Image > open(const vector< string >& files);

  ~Image();

  uint64  size() const;
  string  desc() const;

  const vector< string >& files() const { return Files; }

  weak_ptr< VolumeSystem > volumeSystem() const;
  weak_ptr< Filesystem > filesystem() const;

private:
  Image(TSK_IMG_INFO* img, const vector< string >& files);

  TSK_IMG_INFO* Img;
  vector< string > Files;
  shared_ptr< VolumeSystem > VS;
  shared_ptr< Filesystem > Fs;
};
