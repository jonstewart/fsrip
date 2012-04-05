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
  ImageDumper(ostream& out): Out(out) {}

  virtual uint8_t start();

private:
  ostream& Out;
};

class ImageInfo: public LbtTskAuto {
public:
  ImageInfo(ostream& out, const vector<string>& files): Out(out), Files(files) {}

  virtual uint8_t start();

private:
  ostream& Out;
  vector<string> Files;
};

class FileCounter: public LbtTskAuto {
public:
  FileCounter(ostream& out): NumFiles(0), Out(out) {}

  virtual ~FileCounter() {}

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE*, const char*) {
    ++NumFiles;
    return TSK_OK;
  }

  virtual void finishWalk() {
    Out << NumFiles << endl;
  }

  unsigned int NumFiles;

protected:
  ostream& Out;
};

class MetadataWriter: public FileCounter {
public:
  MetadataWriter(ostream& out);

  virtual ~MetadataWriter() {}

  virtual TSK_FILTER_ENUM filterFs(TSK_FS_INFO *fs_info);

  virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE *fs_file, const char *path);
  virtual void finishWalk() {}

private:
  string  FsInfo,
          Null,
          CurDir;

  TSK_FS_INFO* Fs;

  unsigned int CurDirIndex;
};
