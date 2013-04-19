/*
src/deacon/tsk.cpp

Created by Jon Stewart on 2010-01-06.
Copyright (c) 2010 Lightbox Technologies, Inc. All rights reserved.
*/

#include "tsk.h"

//#include <tsk3/fs/tsk_fs_i.h>

#include <sstream>

#include <cstdlib>

#include <algorithm>

Filesystem::Filesystem(TSK_FS_INFO* fs): Fs(fs) {}

Filesystem::~Filesystem() {
  tsk_fs_close(Fs);
}

uint64 Filesystem::numBlocks() const {
  return Fs->block_count;
}

unsigned int Filesystem::blockSize() const {
  return Fs->block_size;
}

unsigned int Filesystem::deviceBlockSize() const {
  return Fs->dev_bsize;
}

std::string Filesystem::blockName() const {
  return std::string(Fs->duname);
}

bool Filesystem::littleEndian() const {
  return Fs->endian == TSK_LIT_ENDIAN;
}

uint64 Filesystem::firstBlock() const {
  return Fs->first_block;
}

uint64 Filesystem::firstInum() const {
  return Fs->first_inum;
}

unsigned int Filesystem::flags() const {
  return Fs->flags;
}

std::vector< unsigned char > Filesystem::fsID() const {
  std::vector< unsigned char > ret;
  for (unsigned int i = 0; i < Fs->fs_id_used; ++i) {
    ret.push_back(Fs->fs_id[i]);
  }
  return ret;
}

std::string Filesystem::fsIDAsString() const {
  std::vector< unsigned char > id(fsID());
  std::stringstream buf;
  buf << std::hex;
  for (unsigned int i = 0; i < id.size(); ++i) {
    buf << std::hex << (unsigned int)id[i];
  }
  return buf.str();
}

uint64 Filesystem::fsType() const {
  return Fs->ftype;
}

std::string Filesystem::fsName() const {
  return std::string(tsk_fs_type_toname(Fs->ftype));
}

uint64 Filesystem::journalInum() const {
  return Fs->journ_inum;
}

uint64 Filesystem::numInums() const {
  return Fs->inum_count;
}

uint64 Filesystem::lastBlock() const {
  return Fs->last_block;
}

uint64 Filesystem::lastBlockAct() const {
  return Fs->last_block_act;
}

uint64 Filesystem::lastInum() const {
  return Fs->last_inum;
}

uint64 Filesystem::byteOffset() const {
  return Fs->offset;
}

uint64 Filesystem::rootInum() const {
  return Fs->root_inum;
}
//***********************************************************************

Volume::Volume(const TSK_VS_PART_INFO* vol): Vol(vol) {
  TSK_FS_INFO* fs(tsk_fs_open_vol(Vol, TSK_FS_TYPE_DETECT));
  if (fs) {
    Fs.reset(new Filesystem(fs));
  }
}

std::string Volume::desc() const {
  return std::string(Vol->desc);
}

unsigned int Volume::flags() const {
  return Vol->flags;
}

uint64 Volume::numBlocks() const {
  return Vol->len;
}

int64 Volume::slotNum() const {
  return Vol->slot_num;
}

uint64 Volume::startBlock() const {
  return Vol->start;
}

int64 Volume::tableNum() const {
  return Vol->table_num;
}

std::weak_ptr< Filesystem > Volume::filesystem() const {
  return std::weak_ptr<Filesystem>(Fs);
}
//***********************************************************************

VolumeSystem::VolumeSystem(TSK_VS_INFO* volInfo): VolInfo(volInfo) {
  for (unsigned int i = 0; i < VolInfo->part_count; ++i) {
    std::shared_ptr<Volume> p(new Volume(tsk_vs_part_get(VolInfo, i)));
    Volumes.push_back(p);
    WeakVols.push_back(std::weak_ptr<Volume>(p));
  }
}

VolumeSystem::~VolumeSystem() {
  tsk_vs_close(VolInfo);
}

unsigned int VolumeSystem::blockSize() const {
  return VolInfo->block_size;
}

uint64  VolumeSystem::offset() const {
  return VolInfo->offset;
}

unsigned int VolumeSystem::numVolumes() const {
  return VolInfo->part_count;
}

unsigned int VolumeSystem::type() const {
  return VolInfo->vstype;
}

std::string VolumeSystem::desc() const {
  return std::string(tsk_vs_type_todesc(VolInfo->vstype));
}

std::vector< std::weak_ptr< Volume > >::const_iterator VolumeSystem::volBegin() const {
  return WeakVols.begin();
}

std::vector< std::weak_ptr< Volume > >::const_iterator VolumeSystem::volEnd() const {
  return WeakVols.end();
}

std::weak_ptr< Volume > VolumeSystem::getVol(unsigned int i) const {
  return std::weak_ptr<Volume>(Volumes.at(i));
}
//***********************************************************************

Image::Image(TSK_IMG_INFO* img, const std::vector< std::string >& files, bool close):
  Img(img), Files(files), ShouldClose(close)
{
  // std::cerr << "opening volumeSystem, sector size = " << Img->sector_size << ", desc = " << desc() << std::endl;
  TSK_VS_INFO* vs = tsk_vs_open(img, 0, TSK_VS_TYPE_DETECT);
  if (vs) {
    VS.reset(new VolumeSystem(vs));
  }
  else {
    // std::cerr << "opening fileSystem" << std::endl;
    TSK_FS_INFO* fs = tsk_fs_open_img(img, 0, TSK_FS_TYPE_DETECT);
    if (fs) {
      Fs.reset(new Filesystem(fs));
    }
  }
}

Image::~Image() {
  if (ShouldClose) {
    tsk_img_close(Img);
  }
}

std::shared_ptr< Image > Image::open(const std::vector< std::string >& files) {
  std::shared_ptr< Image > ret;
  
  const TSK_TCHAR** evArray = new const TSK_TCHAR*[files.size()];
  for (unsigned int i = 0; i < files.size(); ++i) {
    evArray[i] = (const TSK_TCHAR*)files[i].c_str();
  }
  TSK_IMG_INFO* evInfo = tsk_img_open(files.size(), evArray, TSK_IMG_TYPE_DETECT, 0);
  if (evInfo) {
    ret.reset(new Image(evInfo, files, true));
  }
  delete [] evArray;
  
  return ret;
}

std::shared_ptr< Image > Image::wrap(TSK_IMG_INFO* img, const std::vector<std::string>& files, bool close) {
  return std::shared_ptr<Image>(new Image(img, files, close));
}

uint64  Image::size() const {
  return Img->size;
}

std::string  Image::desc() const {
  return std::string(tsk_img_type_todesc(Img->itype)); // utf8?
}

uint64  Image::sectorSize() const {
  return Img->sector_size;
}

std::weak_ptr< VolumeSystem > Image::volumeSystem() const {
  return std::weak_ptr<VolumeSystem>(VS);
}

std::weak_ptr< Filesystem > Image::filesystem() const {
  return std::weak_ptr<Filesystem>(Fs);
}

ssize_t Image::dump(std::ostream& o) const {
  ssize_t rlen;
  char buf[4096]; 
  TSK_OFF_T off = 0;

  while (off < Img->size) {
    rlen = tsk_img_read(Img, off, buf, sizeof(buf));
    if (rlen == -1) {
      return -1;
    }

    off += rlen;

    o.write(buf, rlen);
    if (!o.good()) {
      return -1;
    }
  }

  return Img->size;
}
