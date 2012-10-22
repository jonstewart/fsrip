#include "walkers.h"

#include "jsonhelp.h"

#include <sstream>
#include <iomanip>

ssize_t physicalSize(const TSK_FS_FILE* file) {
  // tsk_fs_file_read
  // toRead == tsk_fs_file_read(file, cur, &Buffer[0], toRead, TSK_FS_FILE_READ_FLAG_NONE))
  return 0;
}

std::string bytesAsString(const unsigned char* idBeg, const unsigned char* idEnd) {
  std::stringstream buf;
  buf << std::hex;
  for (const unsigned char* cur = idBeg; cur < idEnd; ++cur) {
    buf << std::hex << (unsigned int)*cur;
  }
  return buf.str();
}

void outputFS(std::ostream& buf, std::shared_ptr<Filesystem> fs) {
  buf << "," << j(std::string("filesystem")) << ":{"
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

std::string j(const std::weak_ptr< Volume >& vol) {
  std::stringstream buf;
  if (std::shared_ptr< Volume > p = vol.lock()) {
    buf << "{"
        << j("description", p->desc(), true)
        << j("flags", p->flags())
        << j("numBlocks", p->numBlocks())
        << j("slotNum", p->slotNum())
        << j("startBlock", p->startBlock())
        << j("tableNum", p->tableNum());
    if (std::shared_ptr<Filesystem> fs = p->filesystem().lock()) {
      outputFS(buf, fs);
    }
    buf << "}";
  }
  return buf.str();
}

template<class ItType>
void writeSequence(std::ostream& out, ItType begin, ItType end, const std::string& delimiter) {
  if (begin != end) {
    ItType it(begin);
    out << j(*it);
    for (++it; it != end; ++it) {
      out << delimiter << j(*it);
    }
  }
}

void writeAttr(std::ostream& out, const TSK_FS_ATTR* a) {
  out << "{"
      << j("flags", a->flags, true)
      << j("id", a->id)
      << j("name", a->name ? std::string(a->name): std::string(""))
      << j("size", a->size)
      << j("type", a->type)
      << j("rd_buf_size", a->rd.buf_size)
      << j("nrd_allocsize", a->nrd.allocsize)
      << j("nrd_compsize", a->nrd.compsize)
      << j("nrd_initsize", a->nrd.initsize)
      << j("nrd_skiplen", a->nrd.skiplen);
  if (a->flags & TSK_FS_ATTR_RES && a->rd.buf_size && a->rd.buf) {
    out << ", " << j(std::string("rd_buf")) << ":\"";
    std::ios::fmtflags oldFlags = out.flags();
    out << std::hex << std::setfill('0');
    size_t numBytes = std::min(a->rd.buf_size, (size_t)a->size);
    for (size_t i = 0; i < numBytes; ++i) {
      out << std::setw(2) << (unsigned int)a->rd.buf[i];
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

uint8_t ImageDumper::start() {
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

uint8_t ImageInfo::start() {
  std::shared_ptr<Image> img = Image::wrap(m_img_info, Files, false);

  Out << "{";

  Out << j(std::string("files")) << ":[";
  writeSequence(std::cout, img->files().begin(), img->files().end(), ", ");
  Out << "]"
      << j("description", img->desc())
      << j("size", img->size());

  if (std::shared_ptr<VolumeSystem> vs = img->volumeSystem().lock()) {
    Out << "," << j("volumeSystem") << ":{"
        << j("type", vs->type(), true)
        << j("description", vs->desc())
        << j("blockSize", vs->blockSize())
        << j("numVolumes", vs->numVolumes())
        << j("offset", vs->offset());

    if (vs->numVolumes()) {
      Out << "," << j(std::string("volumes")) << ":[";
      writeSequence(Out, vs->volBegin(), vs->volEnd(), ",");
      Out << "]";
    }
    else {
      std::cerr << "Image has volume system, but no volumes" << std::endl;
    }
    Out << "}";
  }
  else if (std::shared_ptr<Filesystem> fs = img->filesystem().lock()) {
    outputFS(Out, fs);
  }
  else {
    std::cerr << "Image had neither a volume system nor a filesystem" << std::endl;
  }
  Out << "}" << std::endl;
  return 0;
}

MetadataWriter::MetadataWriter(std::ostream& out):
  FileCounter(out), Fs(0), CurDirIndex(0) {}

TSK_FILTER_ENUM MetadataWriter::filterFs(TSK_FS_INFO *fs) {
  std::stringstream buf;
  buf << j(std::string("fs")) << ":{"
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
  // std::cerr << "beginning callback" << std::endl;
  try {
    if (file) {
      Out << "{" << FsInfo;
      if (path) {
        Out << j("path", std::string(path));
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
          Out << j("link", std::string(i->link));
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
            << j("name", (n->name && n->name_size ? std::string(n->name): Null))
            << j("shrt_name", (n->shrt_name && n->shrt_name_size ? std::string(n->shrt_name): Null))
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
      Out << "}" << std::endl;
    }
  }
  catch (std::exception& e) {
    std::cerr << "Error on " << NumFiles << ": " << e.what() << std::endl;
  }
  // std::cerr << "finishing callback" << std::endl;
  FileCounter::processFile(file, path);
  return TSK_OK;
}

FileWriter::FileWriter(std::ostream& out):
  MetadataWriter(out), Buffer(4096, 0) {}

TSK_RETVAL_ENUM FileWriter::processFile(TSK_FS_FILE* file, const char* path) {
  TSK_RETVAL_ENUM ret = MetadataWriter::processFile(file, path);
  if (TSK_OK == ret) {
    TSK_OFF_T blkSize = Fs->block_size;
    size_t size = file->meta ? (file->meta->flags == 5 ? file->meta->size: std::min(file->meta->size, blkSize)): 0,
           cur  = 0;
    Out.write((const char*)&size, sizeof(size));
    while (cur < size) {
      ssize_t toRead = std::min(size - cur, Buffer.size());
      if (toRead == tsk_fs_file_read(file, cur, &Buffer[0], toRead, TSK_FS_FILE_READ_FLAG_NONE)) {
        Out.write(&Buffer[0], toRead);
        cur += toRead;
      }
      else {
        throw std::runtime_error("Had a problem reading data out of a file");
      }
    }
  }
  return ret;
}
