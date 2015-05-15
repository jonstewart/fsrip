/*
src/main.cpp

Created by Jon Stewart on 2010-01-04.
Copyright (c) 2010-2015, Stroz Friedberg, LLC
*/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <fstream>
#include <future>
#include <limits>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>

#include "walkers.h"
#include "enums.h"
#include "util.h"
#include "jsonhelp.h"
#include "fsrip.h"

namespace po = boost::program_options;

void printHelp(const po::options_description& desc) {
  std::cout << "fsrip, Copyright (c) 2010-2015, Stroz Friedberg, LLC" << std::endl;
  std::cout << "Built " << __DATE__ << std::endl;
  std::cout << "TSK version is " << tsk_version_get_str() << std::endl;
  std::cout << desc << std::endl;
}

int main(int argc, char *argv[]) {
  Options opts;

  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(&opts.Command), "command to perform [info|dumpimg|dumpfs|dumpfiles]")
    ("overview-file", po::value< std::string >(&opts.OverviewFile), "output disk overview information")
    ("unallocated", po::value< std::string >(&opts.UCMode)->default_value("none"), "how to handle unallocated [none|fragment|block]")
    ("max-unallocated-block-size", po::value< uint64_t >(&opts.MaxUcBlockSize)->default_value(std::numeric_limits<uint64_t>::max()), "Maximum size of an unallocated entry, in blocks")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files")
    ("inode-map-file", po::value<std::string>(&opts.InodeMapFile)->default_value(""), "optional file to output containing directory entry to inode map")
    ("disk-map-file", po::value<std::string>(&opts.DiskMapFile)->default_value(""), "optional file to output containing disk data to inode map");

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(posOpts).run(), vm);
    po::notify(vm);

    std::shared_ptr<LbtTskAuto> walker;

    std::vector< std::string > imgSegs;
    if (vm.count("ev-files")) {
      imgSegs = vm["ev-files"].as< std::vector< std::string > >();
    }
    if (vm.count("help")) {
      printHelp(desc);
    }
    else if (vm.count("command") && vm.count("ev-files") && (walker = createVisitor(opts.Command, std::cout, imgSegs))) {
      std_binary_io();

      return process(walker, imgSegs, vm, opts);
    }
    else {
      std::cerr << "Error: did not understand arguments\n\n";
      printHelp(desc);
      return 1;
    }
  }
  catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << "\n\n";
    printHelp(desc);
    return 1;
  }
  return 0;
}
