/*
src/main.cpp

Created by Jon Stewart on 2010-01-04.
Copyright (c) 2010 Lightbox Technologies, Inc.
*/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "tsk.h"

namespace po = boost::program_options;

using namespace std;

template<class T>
const T& j(const T& x) {
  return x;
}

template<>
const string& j<string>(const string& x) {
  static string S; // so evil
  S = "\"";
  S += x;
  S += "\"";
  return S;
}

template<class T>
class JsonPair {
public:
  JsonPair(const string& key, const T& val, bool first): Key(key), Value(val), First(first) {}

  const string& Key;
  const T& Value;
  bool First;
};

template<class T>
JsonPair<T> j(const string& key, const T& val, bool first = false) {
  return JsonPair<T>(key, val, first);
}

template<class T>
ostream& operator<<(ostream& out, const JsonPair<T>& pair) {
  if (!pair.First) {
    out << ",";
  }
  out << j(pair.Key) << ":";
  out << j(pair.Value);
  return out;
}

void outputFS(ostream& buf, shared_ptr<Filesystem> fs) {
  buf << "," << j(string("filesystem")) << ":{"
      << j("numBlocks", fs->numBlocks(), true)
      << j("blockSize", fs->blockSize())
      << j("deviceBlockSize", fs->deviceBlockSize())
      << j("blockName", fs->blockName())
      << j("littleEndian", fs->littleEndian())
      << j("firstBlock", fs->firstBlock())
      << j("firstInum", fs->firstInum())
      << j("flags", fs->flags())
      << j("fsID", fs->fsIDAsString())
      << j("type", fs->fsType())
      << j("typeName", fs->fsName())
      << j("isOrphanHunting", fs->isOrphanHunting())
      << j("journalInum", fs->journalInum())
      << j("numInums", fs->numInums())
      << j("lastBlock", fs->lastBlock())
      << j("lastBlockAct", fs->lastBlockAct())
      << j("lastInum", fs->lastInum())
      << j("byteOffset", fs->byteOffset())
      << j("rootInum", fs->rootInum())
      << "}";
}

string j(const weak_ptr< Volume >& vol) {
  stringstream buf;
  if (shared_ptr< Volume > p = vol.lock()) {
    buf << "{"
        << j("description", p->desc(), true)
        << j("flags", p->flags())
        << j("numBlocks", p->numBlocks())
        << j("slotNum", p->slotNum())
        << j("startBlock", p->startBlock())
        << j("tableNum", p->tableNum());
    if (shared_ptr<Filesystem> fs = p->filesystem().lock()) {
      outputFS(buf, fs);
    }
    buf << "}";
  }
  return buf.str();
}

template<class ItType>
void writeSequence(ostream& out, ItType begin, ItType end, const string& delimiter) {
  if (begin != end) {
    ItType it(begin);
    out << j(*it);
    for (++it; it != end; ++it) {
      out << delimiter << j(*it);
    }
  }
}

void writeAttr(ostream& out, const TSK_FS_ATTR* a) {
  out << "{"
      << j("flags", a->flags, true)
      << j("id", a->id)
      << j("name", a->name ? string(a->name): string(""))
      << j("size", a->size)
      << j("type", a->type)
      << j("rd_buf_size", a->rd.buf_size)
      << j("nrd_allocsize", a->nrd.allocsize)
      << j("nrd_compsize", a->nrd.compsize)
      << j("nrd_initsize", a->nrd.initsize)
      << j("nrd_skiplen", a->nrd.skiplen);
  if (a->flags & TSK_FS_ATTR_RES && a->rd.buf_size && a->rd.buf) {
    out << ", " << j(string("rd_buf")) << ":\"";
    ios::fmtflags oldFlags = out.flags();
    out << hex << setfill('0');
    size_t numBytes = min(a->rd.buf_size, (size_t)a->size);
    for (size_t i = 0; i < numBytes; ++i) {
      out << setw(2) << (unsigned int)a->rd.buf[i];
    }
    out.flags(oldFlags);
    out << "\"";
  }
  if (a->flags & TSK_FS_ATTR_NONRES && a->nrd.run) {
    out << ", \"nrd_runs\":[";
    for (TSK_FS_ATTR_RUN* curRun = a->nrd.run; curRun; curRun = curRun->next) {
      if (curRun != a->nrd.run) {
        out << ", ";
      }
      out << "{"
          << j("addr", curRun->addr, true)
          << j("flags", curRun->flags)
          << j("len", curRun->len)
          << j("offset", curRun->offset)
          << "}";
    }
    out << "]";
  }
  out << "}";
}

string bytesAsString(const unsigned char* idBeg, const unsigned char* idEnd) {
  stringstream buf;
  buf << hex;
  for (const unsigned char* cur = idBeg; cur < idEnd; ++cur) {
    buf << hex << (unsigned int)*cur;
  }
  return buf.str();
}

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

  virtual uint8_t start() {
    ssize_t rlen;
    char buf[4096];
    TSK_OFF_T off = 0;

    while (off < m_img_info->size) {
      rlen = tsk_img_read(m_img_info, off, buf, sizeof(buf));
      if (rlen == -1) {
        return -1;
      }
      off += rlen;
      Out.write(buf, rlen);
      if (!Out.good()) {
        return -1;
      }
    }
    return 0;
  }

private:
  ostream& Out;
};

class ImageInfo: public LbtTskAuto {
public:
  ImageInfo(ostream& out, const vector<string>& files): Out(out), Files(files) {}

  virtual uint8_t start() {
    shared_ptr<Image> img = Image::wrap(m_img_info, Files, false);

    Out << "{";

    Out << j(string("files")) << ":[";
    writeSequence(cout, img->files().begin(), img->files().end(), ", ");
    Out << "]"
        << j("description", img->desc())
        << j("size", img->size());

    if (shared_ptr<VolumeSystem> vs = img->volumeSystem().lock()) {
      Out << "," << j("volumeSystem") << ":{"
          << j("type", vs->type(), true)
          << j("description", vs->desc())
          << j("blockSize", vs->blockSize())
          << j("numVolumes", vs->numVolumes())
          << j("offset", vs->offset());

      if (vs->numVolumes()) {
        Out << "," << j(string("volumes")) << ":[";
        writeSequence(Out, vs->volBegin(), vs->volEnd(), ",");
        Out << "]";
      }
      else {
        cerr << "Image has volume system, but no volumes" << endl;
      }
      Out << "}";
    }
    else if (shared_ptr<Filesystem> fs = img->filesystem().lock()) {
      outputFS(Out, fs);
    }
    else {
      cerr << "Image had neither a volume system nor a filesystem" << endl;
    }
    Out << "}" << std::endl;
    return 0;
  }

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

MetadataWriter::MetadataWriter(ostream& out):
  FileCounter(out), Fs(0), CurDirIndex(0) {}

TSK_FILTER_ENUM MetadataWriter::filterFs(TSK_FS_INFO *fs) {
  stringstream buf;
  buf << j(string("fs")) << ":{"
      << j("byteOffset", fs->offset, true)
      << j("blockSize", fs->block_size)
      << j("fsID", bytesAsString(fs->fs_id, &fs->fs_id[fs->fs_id_used]))
      << "}";
  FsInfo = buf.str();
  Fs = fs;
  return TSK_FILTER_CONT;
}

TSK_RETVAL_ENUM MetadataWriter::processFile(TSK_FS_FILE* file, const char* path) {
  ++CurDirIndex;
  if (0 != CurDir.compare(path)) {
    CurDirIndex = 0;
    CurDir.assign(path);
  }
  // cerr << "beginning callback" << endl;
  try {
    if (file) {
      Out << "{" << FsInfo;
      if (path) {
        Out << j("path", string(path));
      }
      if (file->meta) {
        // need to come back for name2 and attrlist      
        TSK_FS_META* i = file->meta;
        Out << ", \"meta\":{"
            << j("addr", i->addr, true)
            << j("atime", i->atime)
            << j("content_len", i->content_len)
            << j("crtime", i->crtime)
            << j("ctime", i->ctime)
            << j("flags", i->flags)
            << j("gid", i->gid);
        if (i->link) {
          Out << j("link", string(i->link));
        }
        if (TSK_FS_TYPE_ISEXT(Fs->ftype)) {
          Out << j("dtime", i->time2.ext2.dtime);
        }
        else if (TSK_FS_TYPE_ISHFS(Fs->ftype)) {
          Out << j("bkup_time", i->time2.hfs.bkup_time);
        }
        Out << j("mode", i->mode)
            << j("mtime", i->mtime)
            << j("nlink", i->nlink)
            << j("seq", i->seq)
            << j("size", i->size)
            << j("type", i->type)
            << j("uid", i->uid)
            << "}";
      }
      if (file->name) {
        TSK_FS_NAME* n = file->name;
        Out << ", \"name\":{"
            << j("flags", n->flags, true)
            << j("meta_addr", n->meta_addr)
            << j("meta_seq", n->meta_seq)
            << j("name", (n->name && n->name_size ? string(n->name): Null))
            << j("shrt_name", (n->shrt_name && n->shrt_name_size ? string(n->shrt_name): Null))
            << j("type", n->type)
            << j("dirIndex", CurDirIndex)
            << "}";

      }
      int numAttrs = tsk_fs_file_attr_getsize(file);
      if (numAttrs > 0) {
        Out << ", \"attrs\":[";
        uint num = 0;
        for (int i = 0; i < numAttrs; ++i) {
          const TSK_FS_ATTR* a = tsk_fs_file_attr_get_idx(file, i);
          if (a) {
            if (num > 0) {
              Out << ", ";
            }
            writeAttr(Out, a);
            ++num;
          }
        }
        Out << "]";
      }
      Out << "}" << endl;
    }
  }
  catch (std::exception& e) {
    cerr << "Error on " << NumFiles << ": " << e.what() << endl;
  }
  // cerr << "finishing callback" << endl;
  FileCounter::processFile(file, path);
  return TSK_OK;
}

void printHelp(const po::options_description& desc) {
  std::cout << "fsrip, Copyright (c) 2010-2011, Lightbox Technologies, Inc." << std::endl;
  std::cout << "TSK version is " << tsk_version_get_str() << std::endl;
  std::cout << "Boost program options version is " << BOOST_PROGRAM_OPTIONS_VERSION << std::endl;
  std::cout << desc << std::endl;
}

shared_ptr<LbtTskAuto> createVisitor(const string& cmd, ostream& out, const vector<string>& segments) {
/*  if (cmd == "info") {
    return shared_ptr<TskAuto>(new ImgInfo(out));
  }*/
  if (cmd == "dumpimg") {
    return shared_ptr<LbtTskAuto>(new ImageDumper(out));
  }
  else if (cmd == "dumpfs") {
    return shared_ptr<LbtTskAuto>(new MetadataWriter(out));
  }
  else if (cmd == "count") {
    return shared_ptr<LbtTskAuto>(new FileCounter(out));
  }
  else if (cmd == "info") {
    return shared_ptr<LbtTskAuto>(new ImageInfo(out, segments));
  }
  else {
    return shared_ptr<LbtTskAuto>();
  }
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(), "command to perform [info|dumpimg|dumpfs|count]")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files");

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(posOpts).run(), vm);
    po::notify(vm);

    shared_ptr<LbtTskAuto> walker;

    std::vector< string > imgSegs;
    if (vm.count("ev-files")) {
      imgSegs = vm["ev-files"].as< vector< string > >();
    }
    if (vm.count("help")) {
      printHelp(desc);
    }
    else if (vm.count("command") && vm.count("ev-files") && (walker = createVisitor(vm["command"].as<string>(), cout, imgSegs))) {
      scoped_array< TSK_TCHAR* >  segments(new TSK_TCHAR*[imgSegs.size()]);
      for (unsigned int i = 0; i < imgSegs.size(); ++i) {
        segments[i] = (TSK_TCHAR*)imgSegs[i].c_str();
      }
      if (0 == walker->openImage(imgSegs.size(), segments.get(), TSK_IMG_TYPE_DETECT, 0)) {
        if (0 == walker->start()) {
          walker->finishWalk();
          return 0;
        }
        else {
          cerr << "Had an error parsing filesystem" << endl;
        }
      }
      else {
        cerr << "Had an error opening the evidence file" << endl;
        return 1;
      }
    }
    else {
      std::cerr << "did not understand arguments" << std::endl;
      return 1;
    }
  }
  catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << "\n\n";
    printHelp(desc);
    return 1;
  }
  return 0;
}
