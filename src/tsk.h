/*
src/deacon/tsk.h

Created by Jon Stewart on 2010-01-06.
Copyright (c) 2010 Lightbox Technologies, Inc. All rights reserved.
*/

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include <boost/function.hpp>

#include <tsk3/libtsk.h>

typedef unsigned long long uint64;
typedef long long int64;

class Filesystem {
  friend class Volume;
  friend class Image;
public:
  ~Filesystem();

  uint64 numBlocks() const;
  unsigned int blockSize() const;
  unsigned int deviceBlockSize() const;
  std::string blockName() const;
  bool littleEndian() const;
  uint64 firstBlock() const;
  uint64 firstInum() const;
  unsigned int flags() const;  
  std::vector< unsigned char > fsID() const;
  std::string fsIDAsString() const;
  uint64 fsType() const;
  std::string fsName() const;
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
  std::string desc() const;
  unsigned int flags() const;
  uint64 numBlocks() const;
  int64 slotNum() const;
  uint64 startBlock() const;
  int64 tableNum() const;

  std::weak_ptr< Filesystem > filesystem() const;

private:
  Volume(const TSK_VS_PART_INFO* vol);

  const TSK_VS_PART_INFO* Vol;
  std::shared_ptr< Filesystem > Fs;
};

class VolumeSystem {
friend class Image;
  
public:
  typedef std::vector< std::weak_ptr< Volume > >::const_iterator VolIter;
  
  ~VolumeSystem();

  unsigned int blockSize() const;
  uint64  offset() const;
  unsigned int numVolumes() const;
  unsigned int type() const;
  std::string desc() const;

  std::vector< std::weak_ptr< Volume > >::const_iterator volBegin() const;
  std::vector< std::weak_ptr< Volume > >::const_iterator volEnd() const;

  std::weak_ptr< Volume > getVol(unsigned int i) const;
  
private:
  VolumeSystem(TSK_VS_INFO* volInfo);

  std::vector< std::shared_ptr< Volume > > Volumes;
  std::vector< std::weak_ptr< Volume > > WeakVols;

  TSK_VS_INFO* VolInfo;
};

class Image {
public:
  static std::shared_ptr< Image > open(const std::vector< std::string >& files);
  static std::shared_ptr< Image > wrap(TSK_IMG_INFO* img, const std::vector<std::string>& files, bool close);

  ~Image();

  uint64  size() const;
  std::string  desc() const;

  const std::vector< std::string >& files() const { return Files; }

  std::weak_ptr< VolumeSystem > volumeSystem() const;
  std::weak_ptr< Filesystem > filesystem() const;

  ssize_t dump(std::ostream& o) const;

private:
  Image(TSK_IMG_INFO* img, const std::vector< std::string >& files, bool close);

  TSK_IMG_INFO* Img;
  std::vector< std::string > Files;
  std::shared_ptr< VolumeSystem > VS;
  std::shared_ptr< Filesystem > Fs;
  bool ShouldClose;
};
