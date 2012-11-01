#include "walkers.h"

#include "jsonhelp.h"

#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>

std::string j(const boost::icl::discrete_interval<uint64>& i) {
  std::stringstream buf;
  buf << "(" << i.lower() << ", " << i.upper() << ")";
  return buf.str();
}

template<typename ItType>
void writeSequence(std::ostream& out, ItType begin, ItType end, const std::string& delimiter) {
  if (begin != end) {
    ItType it(begin);
    out << j(*it);
    for (++it; it != end; ++it) {
      out << delimiter << j(*it);
    }
  }
}

std::string bytesAsString(const unsigned char* idBeg, const unsigned char* idEnd) {
  std::stringstream buf;
  buf << std::hex;
  for (const unsigned char* cur = idBeg; cur < idEnd; ++cur) {
    buf << std::hex << (unsigned int)*cur;
  }
  return buf.str();
}
/*************************************************************************/

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
/*************************************************************************/

uint8_t ImageInfo::start() {
  // std::cerr << "starting to read image" << std::endl;

  std::shared_ptr<Image> img = Image::wrap(m_img_info, Files, false);

  Out << "{";

  Out << j(std::string("files")) << ":[";
  writeSequence(std::cout, img->files().begin(), img->files().end(), ", ");
  Out << "]"
      << j("description", img->desc())
      << j("size", img->size())
      << j("sectorSize", img->sectorSize());
  Out.flush();
  // std::cerr << "heyo" << std::endl;

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
/*************************************************************************/

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
/*************************************************************************/

TSK_WALK_RET_ENUM sumSizeWalkCallback(TSK_FS_FILE *,
                        TSK_OFF_T,
                        TSK_DADDR_T,
                        char *,
                        size_t a_len,
                        TSK_FS_BLOCK_FLAG_ENUM,
                        void *a_ptr)
{
  *(reinterpret_cast<ssize_t*>(a_ptr)) += a_len;
  return TSK_WALK_CONT;
}

ssize_t physicalSize(const TSK_FS_FILE* file) {
  // this uses tsk_walk with an option not to read data, in order to determine true size
  // includes slack, so should map well to EnCase's physical size
  ssize_t size = 0;
  uint8_t good = tsk_fs_file_walk(const_cast<TSK_FS_FILE*>(file),
                   TSK_FS_FILE_WALK_FLAG_ENUM(TSK_FS_FILE_WALK_FLAG_AONLY | TSK_FS_FILE_WALK_FLAG_SLACK),
                    &sumSizeWalkCallback,
                    &size);
  return good == 0 ? size: 0;
}

MetadataWriter::MetadataWriter(std::ostream& out):
  FileCounter(out), Fs(0), PhysicalSize(-1), CurDirIndex(0) {}

TSK_FILTER_ENUM MetadataWriter::filterFs(TSK_FS_INFO *fs) {
  flushUnallocated();

  std::stringstream buf;
  buf << j(std::string("fs")) << ":{"
      << j("byteOffset", fs->offset, true)
      << j("blockSize", fs->block_size)
      << j("fsID", bytesAsString(fs->fs_id, &fs->fs_id[fs->fs_id_used]))
      << "}";
  FsInfo = buf.str();
  Fs = fs;

  UnallocatedRuns += boost::icl::discrete_interval<uint64>::right_open(0, fs->block_count);
  // UnallocatedRuns.clear();
  // UnallocatedRuns.insert(extent(0, fs->block_count));

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
      PhysicalSize = physicalSize(file);
      writeFile(Out, file, PhysicalSize);
    }
  }
  catch (std::exception& e) {
    std::cerr << "Error on " << NumFiles << ": " << e.what() << std::endl;
  }
  // std::cerr << "finishing callback" << std::endl;
  FileCounter::processFile(file, path);
  return TSK_OK;
}

void MetadataWriter::finishWalk() {
  flushUnallocated();
}

void writeMetaRecord(std::ostream& out, const TSK_FS_META* i, const TSK_FS_INFO* fs) {
  out << "{"
      << j("addr", i->addr, true)
      << j("atime", i->atime)
      << j("content_len", i->content_len)
      << j("crtime", i->crtime)
      << j("ctime", i->ctime)
      << j("flags", i->flags)
      << j("gid", i->gid);
  if (i->link) {
    out << j("link", std::string(i->link));
  }
  if (TSK_FS_TYPE_ISEXT(fs->ftype)) {
    out << j("dtime", i->time2.ext2.dtime);
  }
  else if (TSK_FS_TYPE_ISHFS(fs->ftype)) {
    out << j("bkup_time", i->time2.hfs.bkup_time);
  }
  out << j("mode", i->mode)
      << j("mtime", i->mtime)
      << j("nlink", i->nlink)
      << j("seq", i->seq)
      << j("size", i->size)
      << j("type", i->type)
      << j("uid", i->uid)
      << "}";
}

void writeNameRecord(std::ostream& out, const TSK_FS_NAME* n, unsigned int dirIndex) {
  out << "{"
      << j("flags", n->flags, true)
      << j("meta_addr", n->meta_addr)
      << j("meta_seq", n->meta_seq)
      << j("name", (n->name && n->name_size ? std::string(n->name): ""))
      << j("shrt_name", (n->shrt_name && n->shrt_name_size ? std::string(n->shrt_name): ""))
      << j("type", n->type)
      << j("dirIndex", dirIndex)
      << "}";
}

void MetadataWriter::writeFile(std::ostream& out, const TSK_FS_FILE* file, uint64 physicalSize) {
  Out << "{" << FsInfo
      << j("path", CurDir)
      << j("physicalSize", physicalSize);

  if (file->meta) {
    // need to come back for name2 and attrlist      
    TSK_FS_META* i = file->meta;
    Out << ", \"meta\":";
    writeMetaRecord(out, i, file->fs_info);
  }
  if (file->name) {
    TSK_FS_NAME* n = file->name;
    Out << ", \"name\":";
    writeNameRecord(out, n, CurDirIndex);
  }

  Out << ", \"attrs\":[";
  if (file->meta->attr_state & TSK_FS_META_ATTR_STUDIED && file->meta->attr) {
    for (const TSK_FS_ATTR* a = file->meta->attr->head; a; a = a->next) {
      if (a != file->meta->attr->head) {
        Out << ", ";
      }
      writeAttr(Out, a);
    }
  }
  else {
    int numAttrs = tsk_fs_file_attr_getsize(const_cast<TSK_FS_FILE*>(file));
    if (numAttrs > 0) {
      uint num = 0;
      for (int i = 0; i < numAttrs; ++i) {
        const TSK_FS_ATTR* a = tsk_fs_file_attr_get_idx(const_cast<TSK_FS_FILE*>(file), i);
        if (a) {
          if (num > 0) {
            Out << ", ";
          }
          writeAttr(Out, a);
          ++num;
        }
      }
    }
  }
  Out << "]";
  Out << "}" << std::endl;
}

void MetadataWriter::writeAttr(std::ostream& out, const TSK_FS_ATTR* a) {
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

      markAllocated(extent(curRun->addr, curRun->addr + curRun->len));

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

void MetadataWriter::markAllocated(const extent& allocated) {
  UnallocatedRuns -= boost::icl::discrete_interval<uint64>::right_open(allocated.first, allocated.second);
}

void MetadataWriter::processUnallocatedFile(const TSK_FS_FILE* file, uint64 physicalSize) {
  writeFile(Out, file, physicalSize);
}

void MetadataWriter::flushUnallocated() {
  if (!Fs) {
    return;
  }
  CurDir = "$Unallocated/";
  CurDirIndex = 0;

  TSK_FS_NAME nameRec;
  TSK_FS_META metaRec;
  TSK_FS_FILE fileRec;

  fileRec.name = &nameRec;
  fileRec.meta = &metaRec;
  fileRec.fs_info = Fs;

  nameRec.flags = TSK_FS_NAME_FLAG_UNALLOC;
  nameRec.meta_addr = 0;
  nameRec.meta_seq = 0;
  nameRec.type = TSK_FS_NAME_TYPE_VIRT;

  TSK_FS_ATTR_RUN run;
  run.flags = TSK_FS_ATTR_RUN_FLAG_NONE;
  run.next = 0;
  run.offset = 0;

  TSK_FS_ATTR     attr;
  attr.flags = TSK_FS_ATTR_NONRES;
  attr.fs_file = &fileRec;
  attr.id = 0;
  attr.name = 0;
  attr.name_size = 0;
  attr.next = 0;
  attr.nrd.compsize = 0;
  attr.nrd.run = &run;
  attr.nrd.run_end = &run;
  attr.nrd.skiplen = 0;
  attr.rd.buf = 0;
  attr.rd.buf_size = 0;
  attr.rd.offset = 0;
  attr.type = TSK_FS_ATTR_TYPE_ENUM(128);

  TSK_FS_ATTRLIST attrList;
  attrList.head = &attr;

  metaRec.addr = 0;
  metaRec.atime = 0;
  metaRec.atime_nano = 0;
  metaRec.attr = &attrList;
  metaRec.attr_state = TSK_FS_META_ATTR_STUDIED;
  metaRec.content_len = 0; // ???
  metaRec.content_ptr = 0;
  metaRec.crtime = 0;
  metaRec.crtime_nano = 0;
  metaRec.ctime = 0;
  metaRec.ctime_nano = 0;
  metaRec.flags = TSK_FS_META_FLAG_UNALLOC;
  metaRec.gid = 0;
  metaRec.link = 0;
  metaRec.mode = TSK_FS_META_MODE_ENUM(0);
  metaRec.mtime = 0;
  metaRec.mtime_nano = 0;
  metaRec.name2 = 0;
  metaRec.nlink = 0;
  metaRec.seq = 0;
  metaRec.size = 0; // need to reset this.
  metaRec.time2.ext2.dtime = 0;
  metaRec.time2.ext2.dtime_nano = 0;
  metaRec.type = TSK_FS_META_TYPE_VIRT;
  metaRec.uid = 0;

  unsigned int fieldWidth = std::log10(Fs->block_count) + 1;
  std::string  name;

  for (auto unallocated: UnallocatedRuns) {
    run.addr = unallocated.lower();
    run.len = unallocated.upper() - run.addr;
    attr.size = run.len * Fs->block_size;
    std::stringstream buf;
    buf << std::setw(fieldWidth) << std::setfill('0')
      << run.addr << "-" << run.len;
    name = buf.str();
    nameRec.name = nameRec.shrt_name = const_cast<char*>(name.c_str());
    nameRec.name_size = nameRec.shrt_name_size = std::strlen(nameRec.name);

    processUnallocatedFile(&fileRec, attr.size);

    ++CurDirIndex;
  }

  // Out << "[";
  // writeSequence(Out, UnallocatedRuns.begin(), UnallocatedRuns.end(), ", ");
  // Out << "]\n";
}
/*************************************************************************/

FileWriter::FileWriter(std::ostream& out):
  MetadataWriter(out), Buffer(1024 * 1024, 0) {}

TSK_RETVAL_ENUM FileWriter::processFile(TSK_FS_FILE* file, const char* path) {
  TSK_RETVAL_ENUM ret = MetadataWriter::processFile(file, path);
  if (TSK_OK == ret) {
    size_t size = PhysicalSize == -1 ? 0: PhysicalSize,
           cur  = 0;
    Out.write((const char*)&size, sizeof(size));
    while (cur < size) {
      ssize_t toRead = std::min(size - cur, Buffer.size());
      if (toRead == tsk_fs_file_read(file, cur, &Buffer[0], toRead, TSK_FS_FILE_READ_FLAG_SLACK)) {
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

void FileWriter::processUnallocatedFile(const TSK_FS_FILE* file, uint64 physicalSize) {
//  writeFile(Out, file, physicalSize);
}
