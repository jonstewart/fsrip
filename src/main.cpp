/*
src/deacon/main.cpp

Created by Jon Stewart on 2010-01-04.
Copyright (c) 2010 Lightbox Technologies, Inc. All rights reserved.
*/

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "tsk.h"

namespace po = boost::program_options;

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

class FsWalker {
public:
  FsWalker(ostream& out, shared_ptr<Filesystem> fs);
  TSK_WALK_RET_ENUM printFile(TSK_FS_FILE* file, const char* path, unsigned int dirIndex);
  TSK_WALK_RET_ENUM countFile(TSK_FS_FILE*, const char*, unsigned int);

  unsigned int NumFiles;
  
private:
  ostream& Out;
  shared_ptr<Filesystem> Fs;
  string  FsInfo,
          Null;
};

FsWalker::FsWalker(ostream& out, shared_ptr<Filesystem> fs):
  NumFiles(0), Out(out), Fs(fs)
{
  stringstream buf;
  buf << j(string("fs")) << ":{"
      << j("byteOffset", fs->byteOffset(), true)
      << j("blockSize", fs->blockSize())
      << j("fsID", fs->fsIDAsString())
      << "}";
  FsInfo = buf.str();
}

TSK_WALK_RET_ENUM FsWalker::printFile(TSK_FS_FILE* file, const char* path, unsigned int dirIndex) {
  ++NumFiles;
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
        if (TSK_FS_TYPE_ISEXT(Fs->fsType())) {
          Out << j("dtime", i->time2.ext2.dtime);
        }
        else if (TSK_FS_TYPE_ISHFS(Fs->fsType())) {
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
            << j("dirIndex", dirIndex)
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
  return TSK_WALK_CONT;
}

TSK_WALK_RET_ENUM FsWalker::countFile(TSK_FS_FILE*, const char*, unsigned int) {
  ++NumFiles; 
  return TSK_WALK_CONT;  
}

bool dumpFs(const shared_ptr<Filesystem>& fs) {
  FsWalker dumper(cout, fs);
  bool ret = fs->walk(bind(&FsWalker::printFile, ref(dumper), _1, _2, _3));
  if (!ret) {
    cerr << "Encountered an error while walking the filesystem" << endl;
  }
  return ret;
}

bool walkFs(const shared_ptr<Filesystem>& fs) {
  FsWalker walker(cout, fs);
  bool ret = fs->walk(bind(&FsWalker::countFile, ref(walker), _1, _2, _3));
  if (!ret) {
    cerr << "Encountered an error while walking the filesystem" << endl;
  }
  cout << "Walked " << walker.NumFiles << " directory entries" << std::endl;
  return ret;
}

template<typename VisitFn>
bool visitFilesystems(const shared_ptr< Image >& img, VisitFn fn) {
  bool good = true;
  if (shared_ptr<VolumeSystem> vs = img->volumeSystem().lock()) {
    if (vs->numVolumes()) {
      for (VolumeSystem::VolIter it(vs->volBegin()); it != vs->volEnd(); ++it) {
        if (shared_ptr<Volume> vol = it->lock()) {
          if (shared_ptr<Filesystem> fs = vol->filesystem().lock()) {
            if (!fn(fs)) {
              good = false;
            }
          }
        }
      }
    }
    else {
      good = false;
    }
  }
  else if (shared_ptr<Filesystem> fs = img->filesystem().lock()) {
    good = fn(fs);
  }
  else {
    good = false;
  }
  return good;
}

bool printDeviceInfo(const shared_ptr< Image >& img) {
  std::cout << "{";

  std::cout << j(string("files")) << ":[";
  writeSequence(cout, img->files().begin(), img->files().end(), ", ");
  std::cout << "]"
            << j("description", img->desc())
            << j("size", img->size());

  if (shared_ptr<VolumeSystem> vs = img->volumeSystem().lock()) {
    std::cout << "," << j("volumeSystem") << ":{"
              << j("type", vs->type(), true)
              << j("description", vs->desc())
              << j("blockSize", vs->blockSize())
              << j("numVolumes", vs->numVolumes())
              << j("offset", vs->offset());

    if (vs->numVolumes()) {
      std::cout << "," << j(string("volumes")) << ":[";
      writeSequence(std::cout, vs->volBegin(), vs->volEnd(), ",");
      std::cout << "]";
    }
    else {
      cerr << "Image has volume system, but no volumes" << endl;
    }
    std::cout << "}";
  }
  else if (shared_ptr<Filesystem> fs = img->filesystem().lock()) {
    outputFS(std::cout, fs);
  }
  else {
    cerr << "Image had neither a volume system nor a filesystem" << endl;
  }
  std::cout << "}" << std::endl;
  return true;
}

void printHelp(const po::options_description& desc) {
  std::cout << "deacon, Copyright (c) 2010, Lightbox Technologies, Inc." << std::endl;
  std::cout << "TSK version is " << tsk_version_get_str() << std::endl;
  std::cout << "Boost program options version is " << BOOST_PROGRAM_OPTIONS_VERSION << std::endl;
  std::cout << desc << std::endl;
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(), "command to perform [info|dumpfs]")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files");

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(posOpts).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
      printHelp(desc);
    }
    else if (vm.count("command") && vm.count("ev-files")) {
      std::string command(vm["command"].as< std::string >());
      shared_ptr< Image > img(Image::open(vm["ev-files"].as< vector< string > >()));
      if (img) {
        if (command == "info") {
          return printDeviceInfo(img) ? 0: 1;
        }
        else if (command == "dumpfs") {
          return visitFilesystems(img, dumpFs) ? 0: 1;
        }
        else if (command == "walkfs") {
          return visitFilesystems(img, walkFs) ? 0: 1;
        }
        else {
          std::cerr << "Command '" << command << "' unrecognized\n\n";
          printHelp(desc);
          return 1;
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
