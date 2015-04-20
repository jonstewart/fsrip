/*
Copyright (c) 2010-2015, Stroz Friedberg, LLC
*/

#pragma once

#include <string>

std::string metaType(unsigned int type);

std::string nameType(unsigned int type);

std::string metaFlags(unsigned int flags);

std::string nameFlags(unsigned int flags);

std::string attrFlags(unsigned int flags);

namespace RecordTypes {
  enum RecordTypes {
    FILE  = 0,
    INODE = 1,
    DISK_MAP = 2
  };
}
