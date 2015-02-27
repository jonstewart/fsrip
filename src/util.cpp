#include "util.h"

#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

#include "enums.h"
#include "jsonhelp.h"

template<unsigned int N>
inline void writeBigEndian(const uint64_t val, unsigned char* buf) {
  constexpr unsigned int maskShift = (N - 1) * 8;
  constexpr uint64_t mask = (uint64_t)0xFF << maskShift;
  *buf = (val & mask) >> maskShift;
  writeBigEndian<N-1>(val, buf + 1);
}

template<>
inline void writeBigEndian<1>(const uint64_t val, unsigned char* buf) {
  *buf = val & 0xff;
}

template<unsigned int N>
inline void readBigEndian(const unsigned char* buf, uint64_t& val) {
  constexpr unsigned int shift = (N - 1) * 8;
  val |= (uint64_t)*buf << shift;
  readBigEndian<N-1>(buf + 1, val);
}

template<>
inline void readBigEndian<1>(const unsigned char* buf, uint64_t& val) {
  val |= *buf;
}

unsigned int vintEncode(unsigned char* buf, uint64_t val) {
  if (val < 241) {
    buf[0] = val;
    return 1;
  }
  else if (val < 2288) {
    const unsigned int diff = val - 240;
    buf[0] = (diff / 256) + 241;
    buf[1] = diff % 256;
    return 2;
  }
  else if (val < 67824) {
    buf[0] = 249;
    const unsigned int diff = val - 2288;
    buf[1] = diff / 256;
    buf[2] = diff % 256;
    return 3;
  }
  else if (val < (1ull << 24)) {
    buf[0] = 250;
    writeBigEndian<3>(val, &buf[1]);
    return 4;
  }
  else if (val < (1ull << 32)) {
    buf[0] = 251;
    writeBigEndian<4>(val, &buf[1]);
    return 5;
  }
  else if (val < (1ull << 40)) {
    buf[0] = 252;
    writeBigEndian<5>(val, &buf[1]);
    return 6;
  }
  else if (val < (1ull << 48)) {
    buf[0] = 253;
    writeBigEndian<6>(val, &buf[1]);
    return 7;
  }
  else if (val < (1ull << 56)) {
    buf[0] = 254;
    writeBigEndian<7>(val, &buf[1]);
    return 8;
  }
  else {
    buf[0] = 255;
    writeBigEndian<8>(val, &buf[1]);
    return 9;
  }
}

unsigned int vintDecode(uint64_t& val, const unsigned char* buf) {
  if (buf == nullptr) {
    return 0;
  }
  val = 0;
  int diff = buf[0] - 241;
  switch (diff) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7: // 241-248
      val = 256*diff + buf[1] + 240;
      return 2;
      break;
    case 8: // 249
      val = 2288 + 256*buf[1] + buf[2];
      return 3;
      break;
    case 9: // 250
      readBigEndian<3>(&buf[1], val);
      return 4;
      break;
    case 10:
      readBigEndian<4>(&buf[1], val);
      return 5;
      break;
    case 11:
      readBigEndian<5>(&buf[1], val);
      return 6;
      break;
    case 12:
      readBigEndian<6>(&buf[1], val);
      return 7;
      break;
    case 13:
      readBigEndian<7>(&buf[1], val);
      return 8;
      break;
    case 14:
      readBigEndian<8>(&buf[1], val);
      return 9;
      break;
    default:
      val = buf[0];
      return 1;
  }
}

std::string appendVarint(const std::string& base, const unsigned int val) {
  unsigned char encoded[9];
  auto bytes = vintEncode(encoded, val);
  std::string ret(base);
  ret += bytesAsString(encoded, encoded + bytes);
  return ret;
}

std::string makeChildID(const unsigned char* parentID, unsigned int len, unsigned int childIndex) {
  std::string ret;
  if (len > 1) {
    ret += "00";
    uint64_t level;
    unsigned int levelLength = vintDecode(level, &parentID[1]);
    if (1 + levelLength > len) {
      return "";
    }
    ++level;
    ret = appendVarint(ret, level);
    ret += bytesAsString(&parentID[1 + levelLength], &parentID[len]);
    ret += appendVarint(ret, childIndex);
  }  
  return ret;
}

std::string formatTimestamp(uint32_t unix, uint32_t ns) {
  std::string ret;
  time_t ts = unix;
  tm* tmPtr = gmtime(&ts);
  char tbuf[100];
  size_t len = strftime(tbuf, 100, "%FT%T", tmPtr);
  if (len) {
    ret.append(tbuf);
    if (ns) {
      std::stringstream buf;
      buf.setf(std::ios::fixed, std::ios::floatfield);
      buf.precision(8);
      buf << double(ns)/1000000000;
      std::string frac = buf.str();
      frac.erase(0, 1); // leading 0
      std::string::iterator zeroItr(frac.end());
      --zeroItr;
      while (zeroItr != frac.begin() && *zeroItr == '0') {
        --zeroItr;
      }
      ++zeroItr;
      frac.erase(zeroItr, frac.end()); // kill trailing zeroes
      ret.append(frac);
    }
    ret.append("Z");
  }
  return ret;
}

std::string bytesAsString(const unsigned char* idBeg, const unsigned char* idEnd) {
  std::stringstream buf;
  unsigned short val;
  for (const unsigned char* cur = idBeg; cur < idEnd; ++cur) {
    val = *cur;
    buf.width(2);
    buf.fill('0');
    buf << std::hex;
    buf << val;
  }
  return buf.str();
}

std::string makeInodeID(uint32_t volIndex, uint64_t inum) {
  std::stringstream buf;
  buf.width(2);
  buf.fill('0');
  buf << std::hex << RecordTypes::INODE;
  buf.width(2 * sizeof(volIndex));
  buf.fill('0');
  buf << std::hex << volIndex;
  buf.width(2 * sizeof(inum));
  buf.fill('0');
  buf << std::hex << inum;
  return buf.str();
}

std::string makeDiskMapID(uint64_t offset) {
  std::stringstream buf;
  buf.width(2);
  buf.fill('0');
  buf << std::hex << RecordTypes::DISK_MAP;
  buf.width(2 * sizeof(offset));
  buf.fill('0');
  buf << offset;
  return buf.str();
}

std::string j(const std::string& x) {
  std::string s("\"");
  s += x;
  s += "\"";
  return s;
}

// template<>
// std::string j<char>(const char* x) {
//   return j(std::string(x));
// }
