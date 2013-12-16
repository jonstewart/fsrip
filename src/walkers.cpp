#include "walkers.h"

#include "jsonhelp.h"

#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>

#include <iostream>

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
  FileCounter(out), Fs(0), PhysicalSize(-1), DataWritten(0), InUnallocated(false), UCMode(NONE), CurDirIndex(0)
{
  DummyFile.name = &DummyName;
  DummyFile.meta = &DummyMeta;
  DummyFile.fs_info = Fs;

  DummyName.flags = TSK_FS_NAME_FLAG_UNALLOC;
  DummyName.meta_addr = 0;
  DummyName.meta_seq = 0;
  DummyName.type = TSK_FS_NAME_TYPE_VIRT;

  DummyAttrRun.flags = TSK_FS_ATTR_RUN_FLAG_NONE;
  DummyAttrRun.next = 0;
  DummyAttrRun.offset = 0;

  DummyAttr.flags = (TSK_FS_ATTR_FLAG_ENUM)(TSK_FS_ATTR_NONRES | TSK_FS_ATTR_INUSE);
  DummyAttr.fs_file = &DummyFile;
  DummyAttr.id = 0;
  DummyAttr.name = 0;
  DummyAttr.name_size = 0;
  DummyAttr.next = 0;
  DummyAttr.nrd.compsize = 0;
  DummyAttr.nrd.run = &DummyAttrRun;
  DummyAttr.nrd.run_end = &DummyAttrRun;
  DummyAttr.nrd.skiplen = 0;
  DummyAttr.rd.buf = 0;
  DummyAttr.rd.buf_size = 0;
  DummyAttr.rd.offset = 0;
  DummyAttr.type = TSK_FS_ATTR_TYPE_ENUM(128);

  DummyAttrList.head = &DummyAttr;

  DummyMeta.addr = 0;
  DummyMeta.atime = 0;
  DummyMeta.atime_nano = 0;
  DummyMeta.attr = &DummyAttrList;
  DummyMeta.attr_state = TSK_FS_META_ATTR_STUDIED;
  DummyMeta.content_len = 0; // ???
  DummyMeta.content_ptr = 0;
  DummyMeta.crtime = 0;
  DummyMeta.crtime_nano = 0;
  DummyMeta.ctime = 0;
  DummyMeta.ctime_nano = 0;
  DummyMeta.flags = TSK_FS_META_FLAG_UNALLOC;
  DummyMeta.gid = 0;
  DummyMeta.link = 0;
  DummyMeta.mode = TSK_FS_META_MODE_ENUM(0);
  DummyMeta.mtime = 0;
  DummyMeta.mtime_nano = 0;
  DummyMeta.name2 = 0;
  DummyMeta.nlink = 0;
  DummyMeta.seq = 0;
  DummyMeta.size = 0; // need to reset this.
  DummyMeta.time2.ext2.dtime = 0;
  DummyMeta.time2.ext2.dtime_nano = 0;
  DummyMeta.type = TSK_FS_META_TYPE_VIRT;
  DummyMeta.uid = 0;  
}

TSK_FILTER_ENUM MetadataWriter::filterVol(const TSK_VS_PART_INFO* vs_part) {
  if ((VolMode | vs_part->flags) && ((VolMode & TSK_VS_PART_FLAG_META) || (0 == (vs_part->flags & TSK_VS_PART_FLAG_META)))) {
    CurDir = "";
    CurDirIndex = vs_part->table_num;

    TSK_FS_INFO fs; // we'll make image & volume system look like an fs, sort of
    const TSK_VS_INFO* vs = vs_part->vs;
    fs.block_count = (vs->img_info->size / vs->block_size);
    fs.block_post_size = fs.block_pre_size = 0;
    fs.block_size = fs.dev_bsize = vs->block_size;
    fs.duname = "sector";
    fs.endian = vs->endian;
    fs.first_block = 0;
    fs.first_inum = 0;
    fs.flags = TSK_FS_INFO_FLAG_NONE;
    fs.fs_id_used = 0;
    fs.ftype = TSK_FS_TYPE_UNSUPP;
    fs.img_info = vs->img_info;
    fs.inum_count = 1;
    fs.journ_inum = 0;
    fs.last_block = fs.last_block_act = fs.block_count - 1;
    fs.last_inum = 0;
    fs.list_inum_named = 0;
    fs.offset = 0;
    fs.orphan_dir = 0;
    fs.root_inum = 0;
    setFsInfoStr(&fs);

    DummyFile.fs_info = &fs;

    DummyAttrRun.addr = vs_part->start;
    DummyAttrRun.len = vs_part->len;
    DummyMeta.size = DummyAttr.size = DummyAttr.nrd.allocsize = DummyAttrRun.len * fs.block_size;
    std::stringstream buf;
    buf << "partition-" << vs_part->addr;

    std::string name(buf.str());
    // std::cerr << "name = " << name << std::endl;
    DummyName.name = DummyName.shrt_name = const_cast<char*>(name.c_str());
    DummyName.name_size = DummyName.shrt_name_size = std::strlen(DummyName.name);

//    std::cerr << "processing " << CurDir << name << std::endl;
    processUnallocatedFile(&DummyFile, DummyAttr.size);
//    std::cerr << "done processing" << std::endl;
  }
  return TSK_FILTER_CONT;
}

TSK_FILTER_ENUM MetadataWriter::filterFs(TSK_FS_INFO *fs) {
  setFsInfoStr(fs);
  std::string fsID(bytesAsString(fs->fs_id, &fs->fs_id[fs->fs_id_used]));
  std::stringstream buf;
  buf << j(std::string("fs")) << ":{"
      << j("byteOffset", fs->offset, true)
      << j("blockSize", fs->block_size)
      << j("fsID", fsID)
      << "}";
  FsInfo = buf.str();
  Fs = fs;

  if (InUnallocated) {
    flushUnallocated();
    return TSK_FILTER_SKIP;
  }
  else {
    UnallocatedRuns[fsID] += boost::icl::discrete_interval<uint64>::right_open(0, fs->block_count);
    CurUnallocatedItr = UnallocatedRuns.find(fsID);
    return TSK_FILTER_CONT;
  }
}

void MetadataWriter::startUnallocated() {
  if (NONE != UCMode) {
    InUnallocated = true;
    start();
  }
}

void MetadataWriter::setFsInfoStr(TSK_FS_INFO* fs) {
  std::string fsID(bytesAsString(fs->fs_id, &fs->fs_id[fs->fs_id_used]));
  std::stringstream buf;
  buf << j(std::string("fs")) << ":{"
      << j("byteOffset", fs->offset, true)
      << j("blockSize", fs->block_size)
      << j("fsID", fsID)
      << "}";
  FsInfo = buf.str();
}

void MetadataWriter::setCurDir(const char* path) {
  ++CurDirIndex;
  if (0 != CurDir.compare(path)) {
    CurDirIndex = 0;
    CurDir.assign(path);
  }
}

TSK_RETVAL_ENUM MetadataWriter::processFile(TSK_FS_FILE* file, const char* path) {
  setCurDir(path);
  // std::cerr << "beginning callback" << std::endl;
  try {
    if (file) {
      PhysicalSize = physicalSize(file);
      std::stringstream buf;
      writeFile(buf, file, PhysicalSize);
      std::string output(buf.str());
      Out << output << '\n';
      DataWritten += output.size();
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

void writeDummyNameord(std::ostream& out, const TSK_FS_NAME* n, unsigned int dirIndex) {
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
  out << "{" << FsInfo
      << j("path", CurDir)
      << j("physicalSize", physicalSize);

  TSK_FS_META* m = file->meta;
  if (m && (m->flags & TSK_FS_META_FLAG_USED)) {
    out << ", \"meta\":";
    writeMetaRecord(out, m, file->fs_info);
  }
  if (file->name) {
    TSK_FS_NAME* n = file->name;
    out << ", \"name\":";
    writeDummyNameord(out, n, CurDirIndex);
  }

  out << ", \"attrs\":[";
  if (file->meta && (file->meta->attr_state & TSK_FS_META_ATTR_STUDIED) && file->meta->attr) {
    const TSK_FS_ATTR* lastAttr = 0;
    for (const TSK_FS_ATTR* a = file->meta->attr->head; a; a = a->next) {
      if (a->flags & TSK_FS_ATTR_INUSE) {
        if (lastAttr != 0) {
          out << ", ";
        }
        writeAttr(out, a, file->meta->flags & TSK_FS_META_FLAG_ALLOC);
        lastAttr = a;
      }
    }
  }
  else {
    int numAttrs = tsk_fs_file_attr_getsize(const_cast<TSK_FS_FILE*>(file));
    if (numAttrs > 0) {
      unsigned int num = 0;
      for (int i = 0; i < numAttrs; ++i) {
        const TSK_FS_ATTR* a = tsk_fs_file_attr_get_idx(const_cast<TSK_FS_FILE*>(file), i);
        if (a) {
          if (num > 0) {
            out << ", ";
          }
          writeAttr(out, a, file->meta->flags & TSK_FS_META_FLAG_ALLOC);
          ++num;
        }
      }
    }
  }
  out << "]";
  out << "}";
}

void MetadataWriter::writeAttr(std::ostream& out, const TSK_FS_ATTR* a, const bool isAllocated) {
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
      if (isAllocated) {
        markAllocated(extent(curRun->addr, curRun->addr + curRun->len));
      }

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
  if (allocated.first < allocated.second && allocated.second <= Fs->block_count) {
    CurUnallocatedItr->second -= boost::icl::discrete_interval<uint64>::right_open(allocated.first, allocated.second);    
  }
}

void MetadataWriter::processUnallocatedFile(TSK_FS_FILE* file, uint64 physicalSize) {
  std::stringstream buf;
  writeFile(buf, file, physicalSize);
  const std::string output(buf.str());
  Out << output << '\n';
  DataWritten += output.size();
  FileCounter::processFile(file, CurDir.c_str());
}

void MetadataWriter::flushUnallocated() {
  if (!Fs || UCMode == NONE) {
    return;
  }
  CurDir = "$Unallocated/";
  CurDirIndex = 0;

  DummyFile.fs_info = Fs;

  const unsigned int fieldWidth = std::log10(Fs->block_count) + 1;
  std::string  name;

  const std::string fsID(bytesAsString(Fs->fs_id, &Fs->fs_id[Fs->fs_id_used]));

//  std::cerr << "processing unallocated" << std::endl;
  // uint num = UnallocatedRuns.size();
  // for (auto unallocated: UnallocatedRuns) {
  //   ++num;
  // }
//  std::cerr << num << " unallocated fragments" << std::endl;
  const auto& unallocated = UnallocatedRuns[fsID];
  for (decltype(unallocated.begin()) it(unallocated.begin()); it != unallocated.end(); ++it) {
    auto unallocated = *it;
    // std::cerr << "got here" << std::endl;
    // std::cerr << "run [" << unallocated.lower() << ", " << unallocated.upper() << ")" << std::endl;

    if (FRAGMENT == UCMode) {
      DummyAttrRun.addr = unallocated.lower();
      DummyAttrRun.len = unallocated.upper() - DummyAttrRun.addr;
      DummyMeta.size = DummyAttr.size = DummyAttr.nrd.allocsize = DummyAttrRun.len * Fs->block_size;
      std::stringstream buf;
      buf << std::setw(fieldWidth) << std::setfill('0')
        << DummyAttrRun.addr << "-" << DummyAttrRun.len;
      name = buf.str();
      // std::cerr << "name = " << name << std::endl;
      DummyName.name = DummyName.shrt_name = const_cast<char*>(name.c_str());
      DummyName.name_size = DummyName.shrt_name_size = std::strlen(DummyName.name);

  //    std::cerr << "processing " << CurDir << name << std::endl;
      processUnallocatedFile(&DummyFile, DummyAttr.size);
  //    std::cerr << "done processing" << std::endl;

      ++CurDirIndex;
    }
    else if (BLOCK == UCMode) {
      DummyAttrRun.len = 1;
      DummyMeta.size = DummyAttr.size = DummyAttr.nrd.allocsize = DummyAttrRun.len * Fs->block_size;
      for (auto cur = unallocated.lower(); cur < unallocated.upper(); ++cur) {
        DummyAttrRun.addr = cur;
        std::stringstream buf;
        buf << std::setw(fieldWidth) << std::setfill('0') << DummyAttrRun.addr;
        name = buf.str();
        // std::cerr << "name = " << name << std::endl;
        DummyName.name = DummyName.shrt_name = const_cast<char*>(name.c_str());
        DummyName.name_size = DummyName.shrt_name_size = std::strlen(DummyName.name);

    //    std::cerr << "processing " << CurDir << name << std::endl;
        processUnallocatedFile(&DummyFile, DummyAttr.size);
    //    std::cerr << "done processing" << std::endl;

        ++CurDirIndex;
      }
    }
  }
//  std::cerr << "done processing unallocated" << std::endl;

  // Out << "[";
  // writeSequence(Out, UnallocatedRuns.begin(), UnallocatedRuns.end(), ", ");
  // Out << "]\n";
}
/*************************************************************************/

FileWriter::FileWriter(std::ostream& out):
  MetadataWriter(out), Buffer(1024 * 1024, 0) {}

void FileWriter::writeMetadata(const TSK_FS_FILE* file, uint64 physicalSize) {
  std::stringstream buf;
  writeFile(buf, file, physicalSize);
  const std::string output(buf.str());

  uint64 size = output.size();
  Out.write((const char*)&size, sizeof(size));
  Out.write(output.data(), size);

  DataWritten += sizeof(size);
  DataWritten += output.size();
}

TSK_RETVAL_ENUM FileWriter::processFile(TSK_FS_FILE* file, const char* path) {
  std::string fp(path);
  fp += std::string(file->name->name);
  //std::cerr << "processing " << fp << std::endl;
  setCurDir(path);
  PhysicalSize = physicalSize(file);
  writeMetadata(file, PhysicalSize);

  uint64 size = PhysicalSize == -1 ? 0: PhysicalSize,
         cur  = 0;
  // std::cerr << "writing " << fp << ", size = " << size << std::endl;
  Out.write((const char*)&size, sizeof(size));
  DataWritten += sizeof(size);

  while (cur < size) {
    ssize_t toRead = std::min(size - cur, (uint64)Buffer.size());
    if (toRead == tsk_fs_file_read(file, cur, &Buffer[0], toRead, TSK_FS_FILE_READ_FLAG_SLACK)) {
      Out.write(&Buffer[0], toRead);
      cur += toRead;
    }
    else {
      throw std::runtime_error("Had a problem reading data out of a file");
    }
  }
  DataWritten += cur;
  // std::cerr << "Done with " << fp << ", DataWritten = " << DataWritten << std::endl;
  //std::cerr << "done processing " << fp << std::endl;
  return TSK_OK;
}

void FileWriter::processUnallocatedFile(TSK_FS_FILE* file, uint64 physicalSize) {
  std::string fp(CurDir);
  fp += std::string(file->name->name);
  // std::cerr << "processing " << fp << std::endl;

  writeMetadata(file, physicalSize);

  Out.write((const char*)&physicalSize, sizeof(physicalSize));
  DataWritten += sizeof(physicalSize);

  uint64 bytesToWrite = 0;
  auto bytes = Fs->block_size;

  for (auto run = file->meta->attr->head->nrd.run; run; run = run->next) {
    auto blockOffset = run->addr;
    auto endBlock = run->addr + run->len;
    while (blockOffset < endBlock) {
//      auto toRead = std::min(endBlock - blockOffset, maxBlocks);
      if (bytes == tsk_fs_read_block(Fs, blockOffset, &Buffer[0], bytes)) {
        Out.write(&Buffer[0], bytes);
        ++blockOffset;
        bytesToWrite += bytes;
        // std::cerr << "wrote block " << blockOffset << std::endl;
      }
    }
  }
  DataWritten += bytesToWrite;
  if (bytesToWrite != physicalSize) {
    std::stringstream buf;
    buf << "Did not write out expected amount of unallocated data for " << file->name->name << 
      ". Physical size: " << physicalSize << ", Bytes Written: " << bytesToWrite;
    throw std::runtime_error(buf.str());
  }
  // std::cerr << "done processing " << fp << std::endl;
}
