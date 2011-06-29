/*
src/deacon/tsk.cpp

Created by Jon Stewart on 2010-01-06.
Copyright (c) 2010 Lightbox Technologies, Inc. All rights reserved.
*/

#include "tsk.h"

#include <tsk3/fs/tsk_fs_i.h>

#include <sstream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

#include <iostream>
#include <algorithm>

namespace {
class CallbackHelper {
public:
  CallbackHelper(Filesystem::DirWalkFn fn): Fn(fn) {}

  Filesystem::DirWalkFn Fn;
};

static TSK_WALK_RET_ENUM dirWalkCbHelper(TSK_FS_FILE *file, const char* path, unsigned int dirEntry, void* ptr) {
  return reinterpret_cast<CallbackHelper*>(ptr)->Fn(file, path, dirEntry);
}
}
//***********************************************************************

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

string Filesystem::blockName() const {
  return string(Fs->duname);
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

vector< unsigned char > Filesystem::fsID() const {
  vector< unsigned char > ret;
  for (unsigned int i = 0; i < Fs->fs_id_used; ++i) {
    ret.push_back(Fs->fs_id[i]);
  }
  return ret;
}

string Filesystem::fsIDAsString() const {
  vector< unsigned char > id(fsID());
  stringstream buf;
  buf << hex;
  for (unsigned int i = 0; i < id.size(); ++i) {
    buf << hex << (unsigned int)id[i];
  }
  return buf.str();
}

uint64 Filesystem::fsType() const {
  return Fs->ftype;
}

string Filesystem::fsName() const {
  return string(tsk_fs_type_toname(Fs->ftype));
}

bool Filesystem::isOrphanHunting() const {
  return Fs->isOrphanHunting == 1;
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

bool notPresent(vector< uint64 >& stack, uint64 value) {
  return stack.rend() == find(stack.rbegin(), stack.rend(), value);
}

static const string DOT(".");

int fsRecurse(TSK_FS_INFO* fs, uint64 inum, Filesystem::DirWalkFn callback, string path, vector< uint64 >& stack, TSK_FS_FILE* file, bool lookForOrphans) {
  // cerr << "opening directory " << inum << endl;
  shared_ptr<TSK_FS_DIR> dir(tsk_fs_dir_open_meta(fs, inum), tsk_fs_dir_close);
  if (dir) {
    file = tsk_fs_file_alloc(fs);
    stack.push_back(inum);
    // cerr << "got directory for inum " << inum << ", file is " << (dir->fs_file ? "non-null": "null") << endl;
    if (dir->names && dir->names_used) {
      // cerr << "directory " << inum << " has " << dir->names_used << " children, " << dir->names_alloc << " slots allocated" << endl;
      for (unsigned int i = 0; i < dir->names_used; ++i) {
        TSK_FS_NAME* name = &dir->names[i];
        if (name) {
          // cerr << "child " << i << ", " << name->name << ", opening address " << name->meta_addr << (name->meta_seq ? " and nonzero meta_seq": "") << endl;
          file->name = 0;
          file->meta = 0;
          if ((name->meta_addr
                  || (name->flags & TSK_FS_NAME_FLAG_ALLOC)) && tsk_fs_file_open_meta(fs, file, name->meta_addr)) {
            // if (file->name) {
            //   cerr << "file->name was nonnull!!!" << endl;
            // }
          }
          else {
            // cerr << "no metadata struct" << endl;
            file->meta = 0;
          }
          file->name = name;
          TSK_WALK_RET_ENUM retCode = callback(file, path.c_str(), i);
          
          if (lookForOrphans && file->meta && (file->meta->flags & TSK_FS_META_FLAG_UNALLOC)) {
            if (tsk_list_add(&fs->list_inum_named, file->meta->addr)) {
              // cerr << "had an error adding to the orphan inum list" << endl;
              tsk_list_free(fs->list_inum_named);
              fs->list_inum_named = 0;
              lookForOrphans = false;
            }
          }
          
          if (TSK_WALK_CONT != retCode) {
            // cerr << "Callback said stop for inum " << name->meta_addr << endl;
            return 1;
          }
          if ((TSK_FS_NAME_TYPE_DIR == name->type || TSK_FS_NAME_TYPE_UNDEF == name->type)
            && file->meta
            && file->meta->type == TSK_FS_META_TYPE_DIR
            && ((file->name->flags & TSK_FS_NAME_FLAG_ALLOC) || ((file->name->flags & TSK_FS_NAME_FLAG_UNALLOC) && (file->meta->flags & TSK_FS_META_FLAG_UNALLOC)))
            // && file->name->meta_addr != TSK_FS_ORPHANDIR_INUM(fs)
            && notPresent(stack, name->meta_addr)) {
            if (0 != fsRecurse(fs, name->meta_addr, callback, path + name->name + "/", stack, file, file->name->meta_addr == TSK_FS_ORPHANDIR_INUM(fs) ? false: lookForOrphans)) {
              return 1;
            }
          }
          file->name = 0; // set to null so that tsk_fs_file_close doesn't try to destroy the name structure
          if (file->meta) {
            tsk_fs_meta_close(file->meta);
            file->meta = 0;
          }
        }
      }
    }
    else {
      // cerr << "directory had no children" << endl;
    }
    tsk_fs_file_close(file);
    return 0;
  }
  else {
    // cerr << "Could not open up directory for inum " << inum << endl;
    return 0;    
  }
}

bool Filesystem::walk(DirWalkFn callback) const {
//  CallbackHelper trampoline(callback);
  vector< uint64 > stack;
  tsk_list_free(Fs->list_inum_named);
  Fs->list_inum_named = 0;
  shared_ptr<TSK_FS_FILE> file(tsk_fs_file_alloc(Fs), tsk_fs_file_close);
  if (0 == fsRecurse(Fs, Fs->root_inum, callback, string(""), stack, 0/*file.get()*/, true)) {
    return true;
  }
  else
    return false;
/*  pid_t childPid = fork();
  if (childPid == 0) {
    if (0 == fsRecurse(Fs, Fs->root_inum, callback, string(""))
      tsk_fs_dir_walk(Fs, Fs->root_inum,
      (TSK_FS_DIR_WALK_FLAG_ENUM)(TSK_FS_DIR_WALK_FLAG_ALLOC | TSK_FS_DIR_WALK_FLAG_UNALLOC | TSK_FS_DIR_WALK_FLAG_RECURSE),
      dirWalkCbHelper, &trampoline))
    {
      exit(0);
    }
    else {
      exit(1);
    }
  }
  else {
    int exitCode;
    wait(&exitCode);
    return 0 == exitCode;
  }*/
}
//***********************************************************************

Volume::Volume(const TSK_VS_PART_INFO* vol): Vol(vol) {
  TSK_FS_INFO* fs(tsk_fs_open_vol(Vol, TSK_FS_TYPE_DETECT));
  if (fs) {
    Fs.reset(new Filesystem(fs));
  }
}

string Volume::desc() const {
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

weak_ptr< Filesystem > Volume::filesystem() const {
  return weak_ptr<Filesystem>(Fs);
}
//***********************************************************************

VolumeSystem::VolumeSystem(TSK_VS_INFO* volInfo): VolInfo(volInfo) {
  for (unsigned int i = 0; i < VolInfo->part_count; ++i) {
    shared_ptr<Volume> p(new Volume(tsk_vs_part_get(VolInfo, i)));
    Volumes.push_back(p);
    WeakVols.push_back(weak_ptr<Volume>(p));
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

string VolumeSystem::desc() const {
  return std::string(tsk_vs_type_todesc(VolInfo->vstype));
}

vector< weak_ptr< Volume > >::const_iterator VolumeSystem::volBegin() const {
  return WeakVols.begin();
}

vector< weak_ptr< Volume > >::const_iterator VolumeSystem::volEnd() const {
  return WeakVols.end();
}

weak_ptr< Volume > VolumeSystem::getVol(unsigned int i) const {
  return weak_ptr<Volume>(Volumes.at(i));
}
//***********************************************************************

Image::Image(TSK_IMG_INFO* img, const vector< string >& files):
  Img(img), Files(files)
{
  TSK_VS_INFO* vs = tsk_vs_open(img, 0, TSK_VS_TYPE_DETECT);
  if (vs) {
    VS.reset(new VolumeSystem(vs));
  }
  TSK_FS_INFO* fs = tsk_fs_open_img(img, 0, TSK_FS_TYPE_DETECT);
  if (fs) {
    Fs.reset(new Filesystem(fs));
  }
}

Image::~Image() {
  tsk_img_close(Img);
}

shared_ptr< Image > Image::open(const vector< string >& files) {
  shared_ptr< Image > ret;
  
  const char** evArray = new const char*[files.size()];
  for (unsigned int i = 0; i < files.size(); ++i) {
    evArray[i] = files[i].c_str();
  }
  TSK_IMG_INFO* evInfo = tsk_img_open(files.size(), evArray, TSK_IMG_TYPE_DETECT, 0);
  if (evInfo) {
    ret.reset(new Image(evInfo, files));
  }
  delete [] evArray;
  
  return ret;
}

uint64  Image::size() const {
  return Img->size;
}

string  Image::desc() const {
  return std::string(tsk_img_type_todesc(Img->itype)); // utf8?
}

weak_ptr< VolumeSystem > Image::volumeSystem() const {
  return weak_ptr<VolumeSystem>(VS);
}

weak_ptr< Filesystem > Image::filesystem() const {
  return weak_ptr<Filesystem>(Fs);
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
