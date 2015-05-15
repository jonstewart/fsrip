#pragma once

#include <string>
#include <vector>

#include <boost/program_options.hpp>

class LbtTskAuto;


struct Options {
  std::string Command,
              UCMode,
              VolMode,
              OverviewFile,
              InodeMapFile,
              DiskMapFile;
  uint64_t    MaxUcBlockSize;
};

void std_binary_io();

int process(std::shared_ptr<LbtTskAuto> walker, const std::vector< std::string >&  imgSegs, const boost::program_options::variables_map& vm, const Options& opts);

std::shared_ptr<LbtTskAuto> createVisitor(const std::string& cmd, std::ostream& out, const std::vector<std::string>& segments);
