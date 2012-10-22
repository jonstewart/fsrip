#pragma once

#include "tsk.h"

class LbtTskAuto: public TskAuto {
public:
  virtual ~LbtTskAuto() {}

  virtual uint8_t start() {
    return findFilesInImg();
  }

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE*, const char*) { return TSK_OK; }

  virtual void finishWalk() {}
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
  MetadataWriter(std::ostream& out);

  virtual ~MetadataWriter() {}

  virtual TSK_FILTER_ENUM filterFs(TSK_FS_INFO *fs_info);

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);
  virtual void finishWalk() {}

protected:
  TSK_FS_INFO* Fs;

private:
  std::string  FsInfo,
          Null,
          CurDir;

  unsigned int CurDirIndex;
};

class FileWriter: public MetadataWriter {
public:
  FileWriter(std::ostream& out);

  virtual ~FileWriter() {}

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);

private:
  std::vector<char> Buffer;
};
