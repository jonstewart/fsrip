#include "walkers.h"

#include "jsonhelp.h"
#include "util.h"
#include "enums.h"

#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>
#include <algorithm>

#include <iostream>

std::string j(const boost::icl::discrete_interval<uint64_t>& i) {
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

/*************************************************************************/

std::string appendVarint(const std::string& base, const unsigned int val) {
  unsigned char encoded[9];
  auto bytes = vintEncode(encoded, val);
  std::string ret(base);
  ret += bytesAsString(encoded, encoded + bytes);
  return ret;
}

DirInfo::DirInfo():
  Path(""), BareID(""), Level(0), Count(0) {}

DirInfo DirInfo::newChild(const std::string &path) {
  uint32_t childLvl = childLevel();
  DirInfo  ret;
  ret.Path = path;
  ret.BareID = appendVarint(BareID, Count - 1);
//  ret.BareID = appendVarint(BareID, Count);
  ret.Level = childLvl;
  return ret;
}

std::string DirInfo::id() const {
  std::string ret(appendVarint("", RecordTypes::FILE));
  ret = appendVarint(ret, Level);
  ret += BareID;
  return ret;
}

std::string DirInfo::lastChild() const {
  std::string ret(appendVarint("", RecordTypes::FILE));
  ret = appendVarint(ret, childLevel());
  ret += BareID;
  if (Count > 0) {
    ret = appendVarint(ret, Count - 1);
  }
  return ret;
}

uint32_t DirInfo::childLevel() const {
  // if ID is null, then we are the dummy root
  // root doesn't have a level; its children are at level 0
  return BareID.empty() ? 0: Level + 1;
}

void DirInfo::incCount() {
  ++Count;
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

uint8_t LbtTskAuto::start() {
  return findFilesInImg();
}

std::shared_ptr<Image> LbtTskAuto::getImage(const std::vector<std::string>& files) const {
  return Image::wrap(m_img_info, files, false);
}
/*************************************************************************/

std::ostream& operator<<(std::ostream& out, const Image& img) {
  out << "{";

  out << j(std::string("files")) << ":[";
  writeSequence(out, img.files().begin(), img.files().end(), ", ");
  out << "]"
      << j("description", img.desc())
      << j("size", img.size())
      << j("sectorSize", img.sectorSize());
  out.flush();
  // std::cerr << "heyo" << std::endl;

  if (std::shared_ptr<VolumeSystem> vs = img.volumeSystem().lock()) {
    out << "," << j("volumeSystem") << ":{"
        << j("type", vs->type(), true)
        << j("description", vs->desc())
        << j("blockSize", vs->blockSize())
        << j("numVolumes", vs->numVolumes())
        << j("offset", vs->offset());

    if (vs->numVolumes()) {
      out << "," << j(std::string("volumes")) << ":[";
      writeSequence(out, vs->volBegin(), vs->volEnd(), ",");
      out << "]";
    }
    else {
      std::cerr << "Image has volume system, but no volumes" << std::endl;
    }
    out << "}";
  }
  else if (std::shared_ptr<Filesystem> fs = img.filesystem().lock()) {
    outputFS(out, fs);
  }
  else {
    std::cerr << "Image had neither a volume system nor a filesystem" << std::endl;
  }
  out << "}" << std::endl;
  return out;
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
  FileCounter(out), Fs(0), NumUnallocated(0), DiskSize(0), MaxUnallocatedBlockSize(std::numeric_limits<uint64_t>::max()),
  DataWritten(0), SectorSize(0), NumVols(0), InUnallocated(false), UCMode(NONE)
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
  DummyMeta.flags = (TSK_FS_META_FLAG_ENUM)(TSK_FS_META_FLAG_UNALLOC | TSK_FS_META_FLAG_USED);
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
  Dirs.emplace_back(DirInfo());
}

uint8_t MetadataWriter::start() {
  DiskSize = m_img_info->size;
  SectorSize = m_img_info->sector_size;
  NumVols = 0;
  // set PartBeg and PartEnd in case there isn't a partition scheme
  resetPartitionRange();
  return LbtTskAuto::start();
}

std::string getPartName(const TSK_VS_PART_INFO* vs_part) {
  std::stringstream buf;
  if (vs_part->flags & TSK_VS_PART_FLAG_ALLOC) {
    buf << "part-" << (int)vs_part->table_num << "-" << (int)vs_part->slot_num;
  }
  else if (vs_part->flags & TSK_VS_PART_FLAG_META) {
    buf << "meta-part@" << vs_part->start;
  }
  else {
    buf << "unused@" << vs_part->start;
  }
  return buf.str();
}

TSK_FILTER_ENUM MetadataWriter::filterVol(const TSK_VS_PART_INFO* vs_part) {
  VolName.clear();
  Part = vs_part;

  std::string partName(getPartName(vs_part));
  std::stringstream buf;
  buf << vs_part->addr;
  std::string partID(buf.str());

  Dirs.resize(1);
  ++NumVols;
  if (!InUnallocated) {
    TSK_FS_INFO fs; // we'll make image & volume system look like an fs, sort of
                    // fs.partName will be empty, since we're not _in_ a partition
    const TSK_VS_INFO* vs = vs_part->vs;
    fs.block_count = (vs->img_info->size / vs->img_info->sector_size);
    fs.block_post_size = fs.block_pre_size = 0;
    fs.block_size = fs.dev_bsize = vs->img_info->sector_size;
    fs.duname = "sector";
    fs.endian = vs->endian;
    fs.first_block = 0;
    fs.first_inum = 1;
    fs.flags = TSK_FS_INFO_FLAG_NONE;
    fs.fs_id_used = 0;
    fs.ftype = TSK_FS_TYPE_UNSUPP;
    fs.img_info = vs->img_info;
    fs.inum_count = vs->part_count;
    fs.journ_inum = 0;
    fs.last_block = fs.last_block_act = fs.block_count - 1;
    fs.last_inum = vs->part_count;
    fs.list_inum_named = 0;
    fs.offset = 0;
    fs.orphan_dir = 0;
    fs.root_inum = 1;
    uint32_t numVols = NumVols;
    NumVols = 0; // because NumVols is used as the FS index
    resetPartitionRange(); // since we're talking about the dummy root FS
    setFsInfo(&fs, fs.first_block, fs.first_block + fs.block_count);
    NumVols = numVols;

    DummyFile.fs_info = &fs;

    DummyAttrRun.addr = vs_part->start;
    DummyAttrRun.len = vs_part->len;
    DummyAttrRun.offset = 0;
    DummyMeta.size = DummyAttr.size = DummyAttr.nrd.initsize = DummyAttr.nrd.allocsize = DummyAttrRun.len * fs.block_size;
    
    DummyName.meta_addr = DummyMeta.addr = NumVols;

    // std::cerr << "name = " << name << std::endl;
    DummyName.name = DummyName.shrt_name = const_cast<char*>(partName.c_str());
    DummyName.name_size = DummyName.shrt_name_size = partName.size();

//    std::cerr << "processing " << CurDir << name << std::endl;
    processFile(&DummyFile, "");
//    std::cerr << "done processing" << std::endl;
  }
  setPartitionRange(std::min(vs_part->start * SectorSize, DiskSize),
                    std::min((vs_part->start * vs_part->len) * SectorSize, DiskSize));
  VolName = partName;
  Dirs.emplace_back(Dirs.back().newChild(VolName + "/"));
  return TSK_FILTER_CONT;
}

TSK_FILTER_ENUM MetadataWriter::filterFs(TSK_FS_INFO *fs) {
  using namespace boost::icl;
  setFsInfo(fs, Part ? Part->start: 0, Part ? Part->start + Part->len: m_img_info->size / m_img_info->sector_size);

  if (InUnallocated) {
    for (unsigned i = 0; i < NumRootEntries[NumVols]; ++i) {
      Dirs.back().incCount();
    }
    flushUnallocated();
    return TSK_FILTER_SKIP;
  }
  else {
    return TSK_FILTER_CONT;
  }
}

void MetadataWriter::startUnallocated() {
  if (NONE != UCMode) {
    InUnallocated = true;
    start();
  }
}

void MetadataWriter::setFsInfo(TSK_FS_INFO* fs, uint64_t startSector, uint64_t endSector) {
  FsID = bytesAsString(fs->fs_id, &fs->fs_id[fs->fs_id_used]);
  std::stringstream buf;
  buf << j(std::string("fs")) << ":{"
      << j("byteOffset", fs->offset, true)
      << j("blockSize", fs->block_size)
      << j("fsID", FsID)
      << j("volName", VolName)
      << j("volIndex", NumVols)
      << "}";
  FsInfo = buf.str();
  Fs = fs; // does not take ownership
  FSBeg = PartBeg;
  FSEnd = PartEnd;
  CurAllocatedItr = AllocatedRuns.find(NumVols);
  if (AllocatedRuns.end() == CurAllocatedItr) {
    CurAllocatedItr = AllocatedRuns.insert(std::make_pair(NumVols,
                        std::make_tuple(fs->block_size, startSector, endSector, FsMap{}))).first;
  }
}

bool MetadataWriter::atFSRootLevel(const std::string& path) const {
  return path.size() == VolName.size() + 1 && path.back() == '/';
}

void MetadataWriter::setCurDir(const char* path) {
  std::string p;
  if (!VolName.empty()) {
    p += VolName;
    p += "/";
  }
  p += path;

  auto rItr = std::find_if(Dirs.rbegin(), Dirs.rend(), [&p](const DirInfo& d){ return d.path() == p; });
  if (rItr == Dirs.rend()) {
    // Couldn't find the dir on the stack, so push it on
    // However, since TSK uses depth-first traversal, we'll have seen the entry for the directory immediately prior,
    // so we _MUST NOT_ increment the count on Dirs.back(), because then we'd be double-counting.
    // std::cerr << "new directory " << p << std::endl;
    Dirs.emplace_back(Dirs.back().newChild(p));
  }
  else {
    // found it; pop off any children and inc the count
    // std::cerr << "old directory " << p << std::endl;
    Dirs.erase(rItr.base(), Dirs.end());
  }
  Dirs.back().incCount();
  if (atFSRootLevel(p)) {
    NumRootEntries[NumVols] = Dirs.back().count();
  }
  // std::cerr << "setCurDir(" << path << ") seen, id = " << Dirs.back().id() << ", Count = " << Dirs.back().count()
  //   << ", parentID = " << (++Dirs.rbegin() != Dirs.rend() ? (++Dirs.rbegin())->id(): "") << std::endl;
}

void MetadataWriter::resetPartitionRange() {
  setPartitionRange(0, DiskSize);
}

void MetadataWriter::setPartitionRange(uint64_t begin, uint64_t end) {
  PartBeg = begin;
  PartEnd = end;
}


TSK_RETVAL_ENUM MetadataWriter::processFile(TSK_FS_FILE* file, const char* path) {
  // std::cerr << "processFile on " << path << file->name->name << std::endl;
  setCurDir(path);
  // std::cerr << "beginning callback" << std::endl;
  try {
    if (file) {
      std::stringstream buf;
      writeFile(buf, file);
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

void MetadataWriter::writeMetaRecord(std::ostream& out, const TSK_FS_FILE* file, const TSK_FS_INFO* fs) {
  const TSK_FS_META* i = file->meta;
  out << "{"
      << j<int64_t>("addr", static_cast<int64_t>(i->addr), true)
      << j("accessed", formatTimestamp(i->atime, i->atime_nano))
      << j("content_len", i->content_len)
      << j("created", formatTimestamp(i->crtime, i->crtime_nano))
      << j("metadata", formatTimestamp(i->ctime, i->ctime_nano))
      << j("flags", metaFlags(i->flags))
      << j("gid", i->gid);
  if (i->link) {
    out << j("link", std::string(i->link));
  }
  if (TSK_FS_TYPE_ISEXT(fs->ftype)) {
    out << j("dtime", formatTimestamp(i->time2.ext2.dtime, i->time2.ext2.dtime_nano));
  }
  else if (TSK_FS_TYPE_ISHFS(fs->ftype)) {
    out << j("bkup_time", formatTimestamp(i->time2.hfs.bkup_time, i->time2.hfs.bkup_time_nano));
  }
  out << j("mode", i->mode)
      << j("modified", formatTimestamp(i->mtime, i->mtime_nano))
      << j("nlink", i->nlink)
      << j("seq", i->seq)
      << j("size", i->size)
      << j("type", metaType(i->type))
      << j("uid", i->uid);

  out << ", \"attrs\":[";
  if ((i->attr_state & TSK_FS_META_ATTR_STUDIED) && i->attr) {
    const TSK_FS_ATTR* lastAttr = 0;
    for (const TSK_FS_ATTR* a = i->attr->head; a; a = a->next) {
      if (a->flags & TSK_FS_ATTR_INUSE) {
        if (lastAttr != 0) {
          out << ", ";
        }
        writeAttr(out, i->addr, i, a);
        lastAttr = a;
      }
    }
  }
  else {
    int numAttrs = tsk_fs_file_attr_getsize(const_cast<TSK_FS_FILE*>(file));
    if (numAttrs > 0) {
      unsigned int num = 0;
      for (int j = 0; j < numAttrs; ++j) {
        const TSK_FS_ATTR* a = tsk_fs_file_attr_get_idx(const_cast<TSK_FS_FILE*>(file), j);
        if (a) {
          if (num > 0) {
            out << ", ";
          }
          writeAttr(out, i->addr, i, a);
          ++num;
        }
      }
    }
  }
  out << "]";
  out << "}";
}

void MetadataWriter::writeNameRecord(std::ostream& out, const TSK_FS_NAME* n) {
  out << "{"
      << j("flags", nameFlags(n->flags), true)
      << j<int64_t>("meta_addr", static_cast<int64_t>(n->meta_addr))
      << j("meta_seq", n->meta_seq)
      << j("name", (n->name && n->name_size ? std::string(n->name): ""))
      << j("shrt_name", (n->shrt_name && n->shrt_name_size ? std::string(n->shrt_name): ""))
      << j("type", nameType(n->type))
      << "}";
}

bool typeMatch(const TSK_FS_NAME_TYPE_ENUM n, const TSK_FS_META_TYPE_ENUM m) {
  // because why have one enum for the type when you can have two different lists
  // of the same elements?
  return (n == TSK_FS_NAME_TYPE_FIFO && m == TSK_FS_META_TYPE_FIFO) ||
         (n == TSK_FS_NAME_TYPE_CHR && m == TSK_FS_META_TYPE_CHR) ||
         (n == TSK_FS_NAME_TYPE_DIR && m == TSK_FS_META_TYPE_DIR) ||
         (n == TSK_FS_NAME_TYPE_BLK && m == TSK_FS_META_TYPE_BLK) ||
         (n == TSK_FS_NAME_TYPE_REG && m == TSK_FS_META_TYPE_REG) ||
         (n == TSK_FS_NAME_TYPE_LNK && m == TSK_FS_META_TYPE_LNK) ||
         (n == TSK_FS_NAME_TYPE_SOCK && m == TSK_FS_META_TYPE_SOCK) ||
         (n == TSK_FS_NAME_TYPE_SHAD && m == TSK_FS_META_TYPE_SHAD) ||
         (n == TSK_FS_NAME_TYPE_WHT && m == TSK_FS_META_TYPE_WHT) ||
         (n == TSK_FS_NAME_TYPE_VIRT && m == TSK_FS_META_TYPE_VIRT) ||
         (n == TSK_FS_NAME_TYPE_UNDEF); // no meta type for this, so give it the pedantic benefit of the doubt
}

void MetadataWriter::writeFile(std::ostream& out, const TSK_FS_FILE* file) {
  DirInfo     fileDirEnt(Dirs.back().newChild(""));
  std::string id(fileDirEnt.id());

  out << "{" << j("id", id, true)
      << j("parent", Dirs.back().id())
      << j("children", fileDirEnt.lastChild())
      << ", \"t\":{ \"fsmd\":{ ";

  out << FsInfo
      << j("path", Dirs.back().path());

  TSK_FS_NAME* n = nullptr;
  if (file->name) {
    n = file->name;
    out << ", \"name\":";
    writeNameRecord(out, n);
  }
  TSK_FS_META* m = file->meta;
  if (m && // gotta have a pointer
     (m->flags & TSK_FS_META_FLAG_USED) && // gotta be legit
     (!n || n->flags & TSK_FS_NAME_FLAG_ALLOC || typeMatch(n->type, m->type))) // no sense in outputting meta if file's deleted and name and meta types don't match
  {
    out << ", \"meta\":";
    writeMetaRecord(out, file, file->fs_info);

    ReverseMap[NumVols][file->meta->addr].emplace_back(id);

    out << "}, \"__link\":\"" << makeInodeID(NumVols, file->meta->addr) << "\"";
  }
  else {
    out << "}";
  }

  out << " } }";
}

void MetadataWriter::writeAttr(std::ostream& out, TSK_INUM_T addr, const TSK_FS_META* m, const TSK_FS_ATTR* a) {
  out << "{"
      << j("flags", attrFlags(a->flags), true)
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

  if (a->flags & TSK_FS_ATTR_NONRES) {
    out << ", \"nrd_runs\":[";
    uint64_t fo = 0; // file offset
    uint64_t slackFo = 0;
    uint64_t skipBytes = a->nrd.skiplen; // up from 32 bits to 64 for convenience
    const uint64_t mainSize  = (m->flags & TSK_FS_META_FLAG_COMP) ? a->nrd.allocsize: a->nrd.initsize;
    // if (addr == 3240) {
    //   std::cerr << "mainSize = " << mainSize << "\n";
    // }
    bool first = true;
    for (TSK_FS_ATTR_RUN* curRun = a->nrd.run; curRun; curRun = curRun->next) {
      if (TSK_FS_ATTR_RUN_FLAG_FILLER == curRun->flags) {
        // TO-DO: check on the exact semantics of this flag
        continue;
      }
      // normal case - make absolute offsets
      uint64_t beg = (curRun->addr * Fs->block_size) + Fs->offset,
               runEnd = beg + (curRun->len * Fs->block_size),
               end = runEnd;
      bool     trueSlack = false;
      // if (addr == 3240) {
      //   std::cerr << "beg = " << beg << ", end = " << end << ", len = " << (end - beg) << ", fo = " << fo << ", slackFo = " << slackFo << "\n";
      // }

      // if skipping, advance beg and decrement skipBytes accordingly
      if (skipBytes > 0) { // still towards beginning where skiplen is > 0
        uint64_t toSkip = std::min(end - beg, skipBytes);
        beg += toSkip;
        skipBytes -= toSkip;
      }
      if (beg < end) { // past skipping, we're onto data
        uint64_t bytesRemaining = mainSize - fo; // how much data left in file stream?
        if (beg + bytesRemaining < end) {
          end = beg + bytesRemaining; // end is now beginning of true slack
          trueSlack = true;
          // if (3240 == addr) {
          //   std::cerr << "bytesRemaining = " << bytesRemaining << ", end now =" << end << "\n";
          // }
        }
        if (beg < end) { // if false, we're fully into true slack, nothing of file left
          if (TSK_FS_ATTR_RUN_FLAG_NONE == curRun->flags) {
            // just normal data; sparse blocks will be made available as unallocated
            markDataRun(beg, end, fo, addr, a->id, false);
          }
          fo += (end - beg); // advances fo even if data run is sparse, which is critical
        }
        if (trueSlack) {
          // mark slack at end of allocated space
          if (TSK_FS_ATTR_RUN_FLAG_NONE == curRun->flags) { // but only if not sparse (yes, could have sparse slack)
            markDataRun(end, runEnd, slackFo, addr, a->id, true);
          }
          slackFo += (runEnd - end);
        }
      }
      // output data run as json
      if (!first) {
        out << ", ";
      }
      out << "{"
          << j("addr", curRun->addr, true)
          << j("flags", curRun->flags)
          << j("len", curRun->len)
          << j("offset", curRun->offset)
          << "}";
      first = false;
    }
    out << "]" << j("slack_size", slackFo);
  }
  out << "}";
}

void MetadataWriter::markDataRun(uint64_t beg, uint64_t end, uint64_t offset, TSK_INUM_T addr, uint32_t attrID, bool slack) {
  beg = std::max(beg, FSBeg); // just in case
  end = std::min(end, FSEnd);
  if (beg < end) {
    std::get<3>(CurAllocatedItr->second) += std::make_pair(
      boost::icl::discrete_interval<uint64_t>::right_open(beg, end),
      AttrSet{{AttrRunInfo{addr, attrID, slack, beg, offset}}}
    );
  }
}

bool MetadataWriter::makeUnallocatedDataRun(TSK_DADDR_T start, TSK_DADDR_T end, TSK_FS_ATTR_RUN& datarun) {
  if (start < end) {
    datarun.addr = start;
    datarun.len = end - start;
    datarun.offset = 0;
    return true;
  }
  return false;
}

void MetadataWriter::prepUnallocatedFile(unsigned int fieldWidth, unsigned int blockSize, std::string& name,
                                         TSK_FS_ATTR_RUN& run, TSK_FS_ATTR& attr, TSK_FS_META& meta, TSK_FS_NAME& nameRec)
{
  std::stringstream buf;
  buf << std::setw(fieldWidth) << std::setfill('0')
    << run.addr << "-" << run.len;
  name = buf.str();
  // std::cerr << "name = " << name << std::endl;
  nameRec.name = nameRec.shrt_name = const_cast<char*>(name.c_str());
  nameRec.name_size = nameRec.shrt_name_size = name.size();

  meta.size = attr.size = attr.nrd.initsize = attr.nrd.allocsize = run.len * blockSize;

  nameRec.meta_addr = meta.addr = std::numeric_limits<uint64_t>::max() - run.addr;
}

void MetadataWriter::processUnallocatedFragment(TSK_DADDR_T start, TSK_DADDR_T end, unsigned int fieldWidth, std::string& name) {
  std::string path("$Unallocated/");
  if (makeUnallocatedDataRun(start, end, DummyAttrRun)) {
    if (BLOCK == UCMode) {
      for (TSK_DADDR_T cur(start); cur != end; ++cur) {
        makeUnallocatedDataRun(cur, cur + 1, DummyAttrRun);
        prepUnallocatedFile(fieldWidth, Fs->block_size, name, DummyAttrRun, DummyAttr, DummyMeta, DummyName);
        processFile(&DummyFile, path.c_str());
      }
    }
    else {
      const uint64_t maxBlocks = MaxUnallocatedBlockSize == std::numeric_limits<uint64_t>::max() ? end - start: MaxUnallocatedBlockSize;
      for (TSK_DADDR_T cur = start; cur < end; cur += maxBlocks) {
        makeUnallocatedDataRun(cur, std::min(cur + maxBlocks, end), DummyAttrRun);
        prepUnallocatedFile(fieldWidth, Fs->block_size, name, DummyAttrRun, DummyAttr, DummyMeta, DummyName);
        processFile(&DummyFile, path.c_str());
      }
      // makeUnallocatedDataRun(start, end, DummyAttrRun);
      // prepUnallocatedFile(fieldWidth, Fs->block_size, name, DummyAttrRun, DummyAttr, DummyMeta, DummyName);
      // processFile(&DummyFile, path.c_str());
    }
  }
}

void MetadataWriter::flushUnallocated() {
  if (!Fs || UCMode == NONE) {
    return;
  }
  DummyFile.fs_info = Fs;

  // throw out an entry for the folder, and then reset fields for being files
  std::string name = "$Unallocated";
  DummyName.meta_addr = std::numeric_limits<uint64_t>::max();
  DummyName.meta_seq = 0;
  DummyName.name = DummyName.shrt_name = const_cast<char*>(name.c_str());
  DummyName.name_size = DummyName.shrt_name_size = name.size();
  DummyName.par_addr = Fs->root_inum;
  DummyName.par_seq = 0;
  DummyName.type = TSK_FS_NAME_TYPE_DIR;
  DummyName.flags = TSK_FS_NAME_FLAG_ALLOC;
  DummyMeta.flags = TSK_FS_META_FLAG_UNUSED; // will cause meta to be omitted
  processFile(&DummyFile, "");
  DummyName.type = TSK_FS_NAME_TYPE_VIRT;
  DummyName.par_addr = DummyName.meta_addr;
  DummyMeta.flags = (TSK_FS_META_FLAG_ENUM)(TSK_FS_META_FLAG_USED | TSK_FS_META_FLAG_UNALLOC);
  DummyMeta.type = TSK_FS_META_TYPE_VIRT;

  const unsigned int fieldWidth = std::log10(Fs->block_count) + 1;

  TSK_DADDR_T start = (Fs->first_block * Fs->block_size) + Fs->offset;

//  std::cerr << "processing unallocated" << std::endl;
  const auto& partition = std::get<3>(AllocatedRuns[NumVols]);
  // iterate over the allocated extents
  // start is the end of the last allocated extent, end is the beginning of the next,
  // so we need to do one more round after the loop completes
  for (auto nextFrag(partition.begin()); nextFrag != partition.end(); ++nextFrag) {
    processUnallocatedFragment((start - Fs->offset) / Fs->block_size, (nextFrag->first.lower() - Fs->offset) / Fs->block_size, fieldWidth, name);
    start = nextFrag->first.upper();
  }
  processUnallocatedFragment((start - Fs->offset) / Fs->block_size, Fs->last_block, fieldWidth, name);
//  std::cerr << "done processing unallocated" << std::endl;

  // Out << "[";
  // writeSequence(Out, UnallocatedRuns.begin(), UnallocatedRuns.end(), ", ");
  // Out << "]\n";
}
/*************************************************************************/

FileWriter::FileWriter(std::ostream& out):
  MetadataWriter(out), Buffer(1024 * 1024, 0) {}

void FileWriter::writeMetadata(const TSK_FS_FILE*) {
  // std::stringstream buf;
  // writeFile(buf, file, physicalSize);
  // const std::string output(buf.str());

  // uint64 size = output.size();
  // Out.write((const char*)&size, sizeof(size));
  // Out.write(output.data(), size);

  // DataWritten += sizeof(size);
  // DataWritten += output.size();
}

TSK_RETVAL_ENUM FileWriter::processFile(TSK_FS_FILE*, const char*) {
  // std::string fp(path);
  // fp += std::string(file->name->name);
  // //std::cerr << "processing " << fp << std::endl;
  // setCurDir(path);
  // // PhysicalSize = physicalSize(file);
  // writeMetadata(file);

  // uint64 size = PhysicalSize == -1 ? 0: PhysicalSize,
  //        cur  = 0;
  // // std::cerr << "writing " << fp << ", size = " << size << std::endl;
  // Out.write((const char*)&size, sizeof(size));
  // DataWritten += sizeof(size);

  // while (cur < size) {
  //   ssize_t toRead = std::min(size - cur, (uint64)Buffer.size());
  //   if (toRead == tsk_fs_file_read(file, cur, &Buffer[0], toRead, TSK_FS_FILE_READ_FLAG_SLACK)) {
  //     Out.write(&Buffer[0], toRead);
  //     cur += toRead;
  //   }
  //   else {
  //     throw std::runtime_error("Had a problem reading data out of a file");
  //   }
  // }
  // DataWritten += cur;
  // std::cerr << "Done with " << fp << ", DataWritten = " << DataWritten << std::endl;
  //std::cerr << "done processing " << fp << std::endl;
  return TSK_OK;
}

void FileWriter::processUnallocatedFile(TSK_FS_FILE* ) {
//   std::string fp(DirCounts.back().first);
//   fp += std::string(file->name->name);
//   // std::cerr << "processing " << fp << std::endl;

//   writeMetadata(file);

//   Out.write((const char*)&physicalSize, sizeof(physicalSize));
//   DataWritten += sizeof(physicalSize);

//   uint64 bytesToWrite = 0;
//   auto bytes = Fs->block_size;

//   for (auto run = file->meta->attr->head->nrd.run; run; run = run->next) {
//     auto blockOffset = run->addr;
//     auto endBlock = run->addr + run->len;
//     while (blockOffset < endBlock) {
// //      auto toRead = std::min(endBlock - blockOffset, maxBlocks);
//       if (bytes == tsk_fs_read_block(Fs, blockOffset, &Buffer[0], bytes)) {
//         Out.write(&Buffer[0], bytes);
//         ++blockOffset;
//         bytesToWrite += bytes;
//         // std::cerr << "wrote block " << blockOffset << std::endl;
//       }
//     }
//   }
//   DataWritten += bytesToWrite;
//   if (bytesToWrite != physicalSize) {
//     std::stringstream buf;
//     buf << "Did not write out expected amount of unallocated data for " << file->name->name << 
//       ". Physical size: " << physicalSize << ", Bytes Written: " << bytesToWrite;
//     throw std::runtime_error(buf.str());
//   }
//   // std::cerr << "done processing " << fp << std::endl;
}
