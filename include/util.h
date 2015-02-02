#pragma once

#include <cinttypes>
#include <string>

static const unsigned int MAX_VINT_SIZE = 9;

unsigned int vintEncode(unsigned char* buf, uint64_t val);
unsigned int vintDecode(uint64_t& val, const unsigned char* buf);

std::string formatTimestamp(uint32_t unix, uint32_t ns);


std::string bytesAsString(const unsigned char* idBeg, const unsigned char* idEnd);

std::string makeInodeID(uint32_t volIndex, uint64_t inum);
