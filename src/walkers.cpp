#include "walkers.h"

#include "jsonhelp.h"

#include <sstream>
#include <iomanip>

using namespace std;

string bytesAsString(const unsigned char* idBeg, const unsigned char* idEnd) {
  stringstream buf;
  buf << hex;
  for (const unsigned char* cur = idBeg; cur < idEnd; ++cur) {
    buf << hex << (unsigned int)*cur;
  }
  return buf.str();
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

uint8_t ImageInfo::start() {
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
