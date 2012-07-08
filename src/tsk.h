/*
src/deacon/tsk.h

Created by Jon Stewart on 2010-01-06.
Copyright (c) 2010 Lightbox Technologies, Inc. All rights reserved.
*/

#pragma once

#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <tsk3/libtsk.h>

#include <iostream>
#include <string>
#include <vector>

typedef unsigned long long uint64;
typedef long long int64;

using namespace std;

class Filesystem {
  friend class Volume;
  friend class Image;
public:
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
  uint64 journalInum() const;
  uint64 numInums() const;
  uint64 lastBlock() const;
  uint64 lastBlockAct() const;
  uint64 lastInum() const;
  uint64 byteOffset() const;
  uint64 rootInum() const;

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

  boost::weak_ptr< Filesystem > filesystem() const;

private:
  Volume(const TSK_VS_PART_INFO* vol);

  const TSK_VS_PART_INFO* Vol;
  boost::shared_ptr< Filesystem > Fs;
};

class VolumeSystem {
friend class Image;
  
public:
  typedef vector< boost::weak_ptr< Volume > >::const_iterator VolIter;
  
  ~VolumeSystem();

  unsigned int blockSize() const;
  uint64  offset() const;
  unsigned int numVolumes() const;
  unsigned int type() const;
  string desc() const;

  vector< boost::weak_ptr< Volume > >::const_iterator volBegin() const;
  vector< boost::weak_ptr< Volume > >::const_iterator volEnd() const;

  boost::weak_ptr< Volume > getVol(unsigned int i) const;
  
private:
  VolumeSystem(TSK_VS_INFO* volInfo);

  vector< boost::shared_ptr< Volume > > Volumes;
  vector< boost::weak_ptr< Volume > > WeakVols;

  TSK_VS_INFO* VolInfo;
};

class Image {
public:
  static boost::shared_ptr< Image > open(const vector< string >& files);
  static boost::shared_ptr< Image > wrap(TSK_IMG_INFO* img, const vector<string>& files, bool close);

  ~Image();

  uint64  size() const;
  string  desc() const;

  const vector< string >& files() const { return Files; }

  boost::weak_ptr< VolumeSystem > volumeSystem() const;
  boost::weak_ptr< Filesystem > filesystem() const;

  ssize_t dump(std::ostream& o) const;

private:
  Image(TSK_IMG_INFO* img, const vector< string >& files, bool close);

  TSK_IMG_INFO* Img;
  vector< string > Files;
  boost::shared_ptr< VolumeSystem > VS;
  boost::shared_ptr< Filesystem > Fs;
  bool ShouldClose;
};
