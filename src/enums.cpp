#include "enums.h"

#include <vector>
#include <sstream>

#include <tsk/libtsk.h>

std::string metaType(unsigned int type) {
  static const std::vector<std::string> types({
    "Undefined",
    "File",
    "Folder",
    "Named Pipe",
    "Character Device",
    "Block Device",
    "Symbolic Link",
    "Shadow Inode",
    "Domain Socket",
    "Whiteout Inode",
    "Virtual"
  });
  return type < types.size() ? types[type]: types[0];
}

std::string nameType(unsigned int type) {
  static const std::vector<std::string> types({
    "Undefined",
    "Named Pipe",
    "Character Device",
    "Folder",
    "Block Device",
    "File",
    "Symbolic Link",
    "Domain Socket",
    "Shadow Inode",
    "Whiteout Inode",
    "Virtual"
  });
  return type < types.size() ? types[type]: types[0];
}

std::string metaFlags(unsigned int flags) {
  std::string ret;
  if (flags) {
    bool has = false;
    std::stringstream buf;
    if (flags & TSK_FS_META_FLAG_ALLOC) {
      buf << "Allocated";
      has = true;
    }
    if (flags & TSK_FS_META_FLAG_UNALLOC) {
      if (has) {
        buf << ", ";
      }
      buf << "Deleted";
      has = true;
    }
    if (flags & TSK_FS_META_FLAG_USED) {
      if (has) {
        buf << ", ";
      }
      buf << "Used";
      has = true;
    }
    if (flags & TSK_FS_META_FLAG_UNUSED) {
      if (has) {
        buf << ", ";
      }
      buf << "Unused";
      has = true;
    }
    if (flags & TSK_FS_META_FLAG_COMP) {
      if (has) {
        buf << ", ";
      }
      buf << "Compressed";
      has = true;
    }
    if (flags & TSK_FS_META_FLAG_ORPHAN) {
      if (has) {
        buf << ", ";
      }
      buf << "Orphan";
      has = true;
    }
    ret = buf.str();
  }
  return ret;
}

std::string nameFlags(unsigned int flags) {
  std::string ret;
  if (flags) {
    bool has = false;
    std::stringstream buf;
    if (flags & TSK_FS_NAME_FLAG_ALLOC) {
      buf << "Allocated";
      has = true;
    }
    if (flags & TSK_FS_NAME_FLAG_UNALLOC) {
      if (has) {
        buf << ", ";
      }
      buf << "Deleted";
      has = true;
    }
    ret = buf.str();
  }
  return ret;
}

std::string attrFlags(unsigned int flags) {
  std::string ret;
  if (flags) {
    bool has = false;
    std::stringstream buf;
    if (flags & TSK_FS_ATTR_INUSE) {
      buf << "In Use";
      has = true;
    }
    if (flags & TSK_FS_ATTR_NONRES) {
      if (has) {
        buf << ", ";
      }
      buf << "Non-resident";
      has = true;
    }
    if (flags & TSK_FS_ATTR_RES) {
      if (has) {
        buf << ", ";
      }
      buf << "Resident";
      has = true;
    }
    if (flags & TSK_FS_ATTR_ENC) {
      if (has) {
        buf << ", ";
      }
      buf << "Encrypted";
      has = true;
    }
    if (flags & TSK_FS_ATTR_COMP) {
      if (has) {
        buf << ", ";
      }
      buf << "Compressed";
      has = true;
    }
    if (flags & TSK_FS_ATTR_SPARSE) {
      if (has) {
        buf << ", ";
      }
      buf << "Sparse";
      has = true;
    }
    if (flags & TSK_FS_ATTR_RECOVERY) {
      if (has) {
        buf << ", ";
      }
      buf << "Recovered";
      has = true;
    }
    ret = buf.str();
  }
  return ret;
}
