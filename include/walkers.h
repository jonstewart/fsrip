#pragma once

#include "tsk.h"

#include <boost/icl/interval_map.hpp>

#include <map>

std::ostream& operator<<(std::ostream& out, const Image& img);

std::string makeFileID(const unsigned int level, const std::string& parentID, const unsigned int dirIndex);

struct Extent {
  std::string FileID,
              StreamID;
  std::string PartitionName;
  TSK_INUM_T  MetaAddr;
};

class DirInfo {
public:
  DirInfo(); // makes a dummy root

  std::string id() const;

  const std::string& path() const { return Path; }
  uint32_t           level() const { return Level; }
  uint32_t           count() const { return Count; }


  DirInfo     newChild(const std::string& path); // increments Count and returns a DirInfo
  std::string curChild() const;
  uint32_t    childLevel() const;
  void        incCount();

private:
  std::string Path;
  std::string BareID;
  uint32_t    Level;
  uint32_t    Count;
};

class LbtTskAuto: public TskAuto {
public:
  enum UNALLOCATED_HANDLING {
    NONE,
    FRAGMENT,
    BLOCK,
    SINGLE
  };

  virtual ~LbtTskAuto() {}

  virtual void setUnallocatedMode(const UNALLOCATED_HANDLING) {}
  virtual void setVolMetadataMode(const int) {} // should have bits set to TSK_VS_PART_FLAG_ENUM

  virtual uint8_t start();

  virtual TSK_FILTER_ENUM filterVol(const TSK_VS_PART_INFO*) { return TSK_FILTER_CONT; }

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE*, const char*) { return TSK_OK; }

  virtual void startUnallocated() {}

  virtual void finishWalk() {}

  std::shared_ptr<Image> getImage(const std::vector<std::string>& files) const;
};

class ImageDumper: public LbtTskAuto {
public:
  ImageDumper(std::ostream& out): Out(out) {}

  virtual uint8_t start();

private:
  std::ostream& Out;
};

class FileCounter: public LbtTskAuto {
public:
  FileCounter(std::ostream& out): NumFiles(0), Out(out) {}

  virtual ~FileCounter() {}

  virtual void setVolMetadataMode(const int volMode) { VolMode = volMode; }

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE*, const char*) {
    ++NumFiles;
    return TSK_OK;
  }

  virtual void finishWalk() {
    Out << NumFiles << std::endl;
  }

  unsigned int NumFiles;

protected:
  int           VolMode;
  std::ostream& Out;
};

class MetadataWriter: public FileCounter {
public:
  typedef std::pair<TSK_DADDR_T, TSK_DADDR_T> extent;

  MetadataWriter(std::ostream& out);

  virtual ~MetadataWriter() {}

  virtual void setUnallocatedMode(const UNALLOCATED_HANDLING mode) { UCMode = mode; }

  virtual uint8_t start();

  virtual TSK_FILTER_ENUM filterVol(const TSK_VS_PART_INFO* vs_part);
  virtual TSK_FILTER_ENUM filterFs(TSK_FS_INFO *fs_info);

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);

  virtual void startUnallocated();

  virtual void finishWalk();

protected:
  TSK_FS_INFO* Fs;

  std::string PartitionName;

  ssize_t     DataWritten;

  bool        InUnallocated;

  UNALLOCATED_HANDLING UCMode;

  std::map< std::string, boost::icl::interval_map<uint64, std::set<std::string>> > AllocatedRuns; // FS ID->UC Fragments
  decltype(AllocatedRuns.begin()) CurAllocatedItr;

  void setCurDir(const char* path);
  void setFsInfoStr(TSK_FS_INFO* fs);

  void writeFile(std::ostream& out, const TSK_FS_FILE* file);
  void writeNameRecord(std::ostream& out, const TSK_FS_NAME* n);
  void writeMetaRecord(std::ostream& out, const TSK_FS_FILE* file, const TSK_FS_INFO* fs);
  void writeAttr(std::ostream& out, const TSK_FS_ATTR* attr, const bool isAllocated);

  void markAllocated(const extent& allocated, const std::string& name);
  void flushUnallocated();

  virtual void processUnallocatedFile(TSK_FS_FILE* file);

  TSK_FS_FILE       DummyFile;
  TSK_FS_NAME       DummyName;
  TSK_FS_META       DummyMeta;
  TSK_FS_ATTRLIST   DummyAttrList;
  TSK_FS_ATTR       DummyAttr;
  TSK_FS_ATTR_RUN   DummyAttrRun;

  std::vector<DirInfo> Dirs;

private:
  std::string  FsInfo,
               FsID;
};

class FileWriter: public MetadataWriter {
public:
  FileWriter(std::ostream& out);

  virtual ~FileWriter() {}

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);

private:
  virtual void processUnallocatedFile(TSK_FS_FILE* file);

  void writeMetadata(const TSK_FS_FILE* file);

  std::vector<char> Buffer;
};
