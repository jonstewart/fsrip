#pragma once

#include <cinttypes>
#include <string>

unsigned int vintEncode(unsigned char* buf, uint64_t val);
unsigned int vintDecode(uint64_t& val, const unsigned char* buf);

std::string formatTimestamp(uint32_t unix, uint32_t ns);