/*
src/main.cpp

Created by Jon Stewart on 2010-01-04.
Copyright (c) 2010 Lightbox Technologies, Inc.
*/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>

#include "walkers.h"

#if defined(__WIN32__) || defined(_WIN32_) || defined(__WIN32) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
  #include <cstdio>
  #include <fcntl.h>
  void std_binary_io() {
    _setmode( _fileno( stdin  ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
    _setmode( _fileno( stderr ), _O_BINARY );
    std::cin.sync_with_stdio();
  }
#else
  void std_binary_io() { }
#endif 

namespace po = boost::program_options;

void printHelp(const po::options_description& desc) {
  std::cout << "fsrip, Copyright (c) 2010-2012, Lightbox Technologies, Inc." << std::endl;
  std::cout << "Built " << __DATE__ << std::endl;
  std::cout << "TSK version is " << tsk_version_get_str() << std::endl;
  std::cout << desc << std::endl;
}

std::shared_ptr<LbtTskAuto> createVisitor(const std::string& cmd, std::ostream& out, const std::vector<std::string>& segments) {
  if (cmd == "dumpimg") {
    return std::shared_ptr<LbtTskAuto>(new ImageDumper(out));
  }
  else if (cmd == "dumpfs") {
    return std::shared_ptr<LbtTskAuto>(new MetadataWriter(out));
  }
  else if (cmd == "count") {
    return std::shared_ptr<LbtTskAuto>(new FileCounter(out));
  }
  else if (cmd == "info") {
    return std::shared_ptr<LbtTskAuto>(new ImageInfo(out, segments));
  }
  else if (cmd == "dumpfiles") {
    return std::shared_ptr<LbtTskAuto>(new FileWriter(out));
  }
  else {
    return std::shared_ptr<LbtTskAuto>();
  }
}

int makeVolMode(const std::string& mode) {
  if (mode == "none") {
    return 0;
  }
  else if (mode == "unallocated") {
    return TSK_VS_PART_FLAG_UNALLOC;
  }
  else if (mode == "allocated") {
    return TSK_VS_PART_FLAG_ALLOC;
  }
  else if (mode == "metadata") {
    return TSK_VS_PART_FLAG_META;
  }
  else if (mode == "all") {
    return TSK_VS_PART_FLAG_UNALLOC | TSK_VS_PART_FLAG_ALLOC | TSK_VS_PART_FLAG_META;
  }
  else {
    return -1; // shouldn't happen
  }
}

int main(int argc, char *argv[]) {
  std::string ucMode,
              volMode;

  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(), "command to perform [info|dumpimg|dumpfs|dumpfiles|count]")
    ("unallocated", po::value< std::string >(&ucMode)->default_value("none"), "how to handle unallocated [none|fragment|block]")
    ("volume-entries", po::value< std::string >(&volMode)->default_value("none"), "output metadata entries for volumes [none|unallocated|allocated|metadata|all")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files");

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
    else if (vm.count("command") && vm.count("ev-files") && (walker = createVisitor(vm["command"].as<std::string>(), std::cout, imgSegs))) {
      std_binary_io();

      boost::scoped_array< const char* >  segments(new const char*[imgSegs.size()]);
      for (unsigned int i = 0; i < imgSegs.size(); ++i) {
        segments[i] = imgSegs[i].c_str();
      }
      if (0 == walker->openImageUtf8(imgSegs.size(), segments.get(), TSK_IMG_TYPE_DETECT, 0)) {
        walker->setVolFilterFlags((TSK_VS_PART_FLAG_ENUM)(TSK_VS_PART_FLAG_ALLOC | TSK_VS_PART_FLAG_UNALLOC | TSK_VS_PART_FLAG_META));
        walker->setFileFilterFlags((TSK_FS_DIR_WALK_FLAG_ENUM)(TSK_FS_DIR_WALK_FLAG_RECURSE | TSK_FS_DIR_WALK_FLAG_UNALLOC | TSK_FS_DIR_WALK_FLAG_ALLOC));
        walker->setVolMetadataMode(makeVolMode(volMode));
        if (ucMode == "fragment") {
          walker->setUnallocatedMode(LbtTskAuto::FRAGMENT);
        }
        else if (ucMode == "block") {
          walker->setUnallocatedMode(LbtTskAuto::BLOCK);
        }
        else {
          walker->setUnallocatedMode(LbtTskAuto::NONE);
        }
        if (0 == walker->start()) {
          walker->startUnallocated();
          walker->finishWalk();
          return 0;
        }
        else {
          std::cout.flush();
          std::cerr << "Had an error parsing filesystem" << std::endl;
          for (auto& err: walker->getErrorList()) {
            std::cerr << err.msg1 << " " << err.msg2 << std::endl;
          }
        }
      }
      else {
        std::cerr << "Had an error opening the evidence file" << std::endl;
        for (unsigned int i = 0; i < imgSegs.size(); ++i) {
          std::cerr << " ** seg[" << i << "] = " << imgSegs[i] << std::endl;
        }
        return 1;
      }
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
