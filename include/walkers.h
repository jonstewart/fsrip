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
  std::string lastChild() const;
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

class ImageInfo: public LbtTskAuto {
public:
  ImageInfo(std::ostream& out, const std::vector<std::string>& files): Out(out), Files(files) {}

  virtual uint8_t start();

private:
  std::ostream& Out;
  std::vector<std::string> Files;
};

class FileCounter: public LbtTskAuto {
public:
  FileCounter(std::ostream& out): NumFiles(0), Out(out) {}

  virtual ~FileCounter() {}

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE*, const char*) {
    ++NumFiles;
    return TSK_OK;
  }

  virtual void finishWalk() {
    Out << NumFiles << std::endl;
  }

  unsigned int NumFiles;

protected:
  std::ostream& Out;
};

class MetadataWriter: public FileCounter {
public:
  typedef std::pair<TSK_DADDR_T, TSK_DADDR_T> Extent;
  //                 inum      attrID    run start file offset
  typedef std::tuple<uint64_t, uint32_t, uint64_t> AttrRunInfo;
  typedef std::set<AttrRunInfo> AttrSet;
  typedef boost::icl::interval_map<uint64_t, AttrSet> FsMap;
  typedef std::tuple<uint32_t, uint64_t, uint64_t, FsMap> FsMapInfo;
  typedef std::map<std::string, FsMapInfo> DiskMap;

  MetadataWriter(std::ostream& out);

  virtual ~MetadataWriter() {}

  virtual void setUnallocatedMode(const UNALLOCATED_HANDLING mode) { UCMode = mode; }

  virtual uint8_t start();

  virtual TSK_FILTER_ENUM filterVol(const TSK_VS_PART_INFO* vs_part);
  virtual TSK_FILTER_ENUM filterFs(TSK_FS_INFO *fs_info);

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);

  virtual void startUnallocated();

  virtual void finishWalk();

  const DiskMap& diskMap() const { return AllocatedRuns; }

  uint64_t diskSize() const { return DiskSize; }
  uint32_t sectorSize() const { return SectorSize; }

  static bool makeUnallocatedDataRun(TSK_DADDR_T start, TSK_DADDR_T end, TSK_FS_ATTR_RUN& datarun);

protected:
  const TSK_VS_PART_INFO* Part;
  TSK_FS_INFO*      Fs;

  std::string PartitionName;

  uint64_t    NumUnallocated,
              DiskSize;
  ssize_t     DataWritten;
  uint32_t    SectorSize,
              NumVols;

  bool        InUnallocated;

  UNALLOCATED_HANDLING UCMode;

  DiskMap AllocatedRuns; // FS ID->interval->inodes
  std::map< std::string, unsigned int > NumRootEntries;
  decltype(AllocatedRuns.begin()) CurAllocatedItr;

  void setCurDir(const char* path);
  void setFsInfo(TSK_FS_INFO* fs, uint64_t startSector, uint64_t endSector);

  void writeFile(std::ostream& out, const TSK_FS_FILE* file);
  void writeNameRecord(std::ostream& out, const TSK_FS_NAME* n);
  void writeMetaRecord(std::ostream& out, const TSK_FS_FILE* file, const TSK_FS_INFO* fs);
  void writeAttr(std::ostream& out, TSK_INUM_T addr, const TSK_FS_ATTR* attr);

  void markDataRun(TSK_INUM_T addr, uint32_t attrID, const TSK_FS_ATTR_RUN& dataRun, uint64_t index);

  void prepUnallocatedFile(unsigned int fieldWidth, unsigned int blockSize, std::string& name,
                                         TSK_FS_ATTR_RUN& run, TSK_FS_ATTR& attr, TSK_FS_META& meta, TSK_FS_NAME& nameRec);
  void processUnallocatedFragment(TSK_DADDR_T start, TSK_DADDR_T end, unsigned int fieldWidth, std::string& name);
  void flushUnallocated();

  bool atFSRootLevel(const std::string& path) const;

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
