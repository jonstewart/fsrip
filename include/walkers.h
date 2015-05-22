/*
Copyright (c) 2010-2015, Stroz Friedberg, LLC
*/

#pragma once

#include "tsk.h"

#include <boost/icl/interval_map.hpp>

#include <map>

std::ostream& operator<<(std::ostream& out, const Image& img);

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
  virtual void setMaxUnallocatedBlockSize(const uint64_t) {}

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

struct AttrRunInfo {
  bool operator<(const AttrRunInfo& o) const {
    if (Inum < o.Inum) {
      return true;
    }
    else if (Inum == o.Inum) {
      if (AttrID < o.AttrID) {
        return true;
      }
      else if (AttrID == o.AttrID) {
        if (!Slack && o.Slack) {
          return true;
        }
        else if (Slack == o.Slack) {
          if (DRBeg < o.DRBeg) {
            return true;
          }
          else if (DRBeg == o.DRBeg) {
            return Offset < o.Offset;
          }
        }
      }
    }
    return false;
  }

  bool operator==(const AttrRunInfo& o) const {
    return Inum == o.Inum && AttrID == o.AttrID && Slack == o.Slack && DRBeg == o.DRBeg && Offset == o.Offset;
  }

  uint64_t Inum;
  uint32_t AttrID;
  bool     Slack;
  uint64_t DRBeg,
           Offset;
};

typedef std::set<AttrRunInfo> AttrSet;
typedef boost::icl::interval_map<uint64_t, AttrSet> FsMap;

struct FsMapInfo {
  uint32_t BlockSize;
  uint64_t StartSector,
           EndSector;
  FsMap    Runs;
};

struct Run {
  uint64_t FileOffset,
           Start,
           End;
};

struct AttrInfo {

  AttrInfo(uint32_t id): ID(id), Resident(false), Size(0), SlackSize(0) {}

  uint32_t ID;

  bool        Resident;
  std::string ResidentData;

  uint64_t Size,
           SlackSize;

  std::vector<Run> Runs,
                   SlackRuns;

  void addRun(bool slack, const Run& run) {
    if (slack) {
      SlackRuns.push_back(run);
    }
    else {
      Runs.push_back(run);
    }
  }
};

struct InodeInfo {
  std::vector<std::string> DirentIDs;

  bool Deleted;

  std::vector<AttrInfo> Attrs;

  AttrInfo& getOrInsertAttr(uint32_t id) {
    auto itr = std::find_if(Attrs.begin(), Attrs.end(), [id](const AttrInfo& ai) { return ai.ID == id; });
    if (itr == Attrs.end()) {
      Attrs.push_back(AttrInfo(id));
      return Attrs.back();
    }
    else {
      return *itr;
    }
  }
};

class MetadataWriter: public FileCounter {
public:
  typedef std::pair<TSK_DADDR_T, TSK_DADDR_T> Extent;
  typedef std::map<uint32_t, FsMapInfo> DiskMap; // FS index as key

  // FS index -> inode -> [IDs]
  typedef std::map<uint32_t, std::map<uint64_t, InodeInfo>> ReverseInodeMapType;

  MetadataWriter(std::ostream& out);

  virtual ~MetadataWriter() {}

  virtual void setUnallocatedMode(const UNALLOCATED_HANDLING mode) { UCMode = mode; }
  virtual void setMaxUnallocatedBlockSize(const uint64_t maxBlocks) { MaxUnallocatedBlockSize = maxBlocks; }

  virtual uint8_t start();

  virtual TSK_FILTER_ENUM filterVol(const TSK_VS_PART_INFO* vs_part);
  virtual TSK_FILTER_ENUM filterFs(TSK_FS_INFO *fs_info);

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);

  virtual void startUnallocated();

  virtual void finishWalk();

  const DiskMap& diskMap() const { return AllocatedRuns; }
  const ReverseInodeMapType& reverseMap() const { return ReverseMap; }

  uint64_t diskSize() const { return DiskSize; }
  uint32_t sectorSize() const { return SectorSize; }

  static bool makeUnallocatedDataRun(TSK_DADDR_T start, TSK_DADDR_T end, TSK_FS_ATTR_RUN& datarun);

protected:
  const TSK_VS_PART_INFO* Part;
  TSK_FS_INFO*      Fs;

  std::string VolName;

  uint64_t    NumUnallocated,
              DiskSize,
              PartBeg, // adjusted to fit in disk
              PartEnd, // adjusted to fit in disk
              FSBeg, // adjusted to fit in partition & disk
              FSEnd, // adjusted to fit in partition & disk - 0 <= PartBeg <= FSBeg <= FSEnd <= PartEnd <= DiskSize
              MaxUnallocatedBlockSize;
  ssize_t     DataWritten;
  uint32_t    SectorSize,
              NumVols;

  bool        InUnallocated;

  UNALLOCATED_HANDLING UCMode;

  DiskMap AllocatedRuns; // FS index->interval->inodes
  std::map<uint32_t, unsigned int> NumRootEntries; // FS index->count
  decltype(AllocatedRuns.begin()) CurAllocatedItr;

  ReverseInodeMapType ReverseMap;

  void setCurDir(const char* path);
  void setFsInfo(TSK_FS_INFO* fs, uint64_t startSector, uint64_t endSector);
  void resetPartitionRange();
  void setPartitionRange(uint64_t begin, uint64_t end);

  void writeFile(std::ostream& out, const TSK_FS_FILE* file);
  void writeNameRecord(std::ostream& out, const TSK_FS_NAME* n);
  void writeMetaRecord(std::ostream& out, const TSK_FS_FILE* file, const TSK_FS_INFO* fs, InodeInfo& inode);
  void writeAttr(std::ostream& out, InodeInfo& inode, TSK_INUM_T addr, const TSK_FS_ATTR* attr);

  void markDataRun(uint64_t beg, uint64_t end, uint64_t offset, TSK_INUM_T addr, uint32_t attrID, bool slack);

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
