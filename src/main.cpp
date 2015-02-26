/*
src/main.cpp

Created by Jon Stewart on 2010-01-04.
Copyright (c) 2010 Lightbox Technologies, Inc.
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
  if (cmd == "info") {
    return std::shared_ptr<LbtTskAuto>(new ImageInfo(out, segments));
  }
  else if (cmd == "dumpimg") {
    return std::shared_ptr<LbtTskAuto>(new ImageDumper(out));
  }
  else if (cmd == "dumpfs") {
    return std::shared_ptr<LbtTskAuto>(new MetadataWriter(out));
  }
  else if (cmd == "dumpfiles") {
    return std::shared_ptr<LbtTskAuto>(new FileWriter(out));
  }
  else {
    return std::shared_ptr<LbtTskAuto>();
  }
}

void outputDiskMap(const std::string& diskMapFile, std::shared_ptr<LbtTskAuto> w) {
  // std::cerr << "outputDiskMap" << std::endl;
  auto walker(std::dynamic_pointer_cast<MetadataWriter>(w));
  if (walker) {
    std::ofstream file(diskMapFile, std::ios::out | std::ios::trunc);

    auto map(walker->diskMap());
    for (auto fsMapInfo: map) {
      auto layout = std::get<3>(fsMapInfo.second);
      for (auto frag: layout) {
        file  << "{" << j("id", makeDiskMapID(frag.first.lower()), true)
              << ",\"t\": { \"i\": { "
              << j("b", frag.first.lower(), true)
              << j("l", frag.first.upper() - frag.first.lower())
              << ", \"f\":[";
        bool firstFile = true;
        for (auto f: frag.second) {
          if (!firstFile) {
            file << ", ";
          }
          file  << "{"
                << j("vol", fsMapInfo.first, true)
                << j("inum", static_cast<int64_t>(std::get<0>(f)))
                << j("attrId", std::get<1>(f))
                << j("s", std::get<2>(f))
                << j("drbeg", std::get<3>(f))
                << j("fo", std::get<4>(f))
                << "}";
          firstFile = false;
        }
        file << "]}}}\n";
      }
    }
    file.close();
  }
}

void outputInodeMap(const std::string& inodeMapFile, std::shared_ptr<LbtTskAuto> w) {
  auto walker(std::dynamic_pointer_cast<MetadataWriter>(w));
  if (walker) {
    std::ofstream file(inodeMapFile, std::ios::out | std::ios::trunc);

    const auto& reverseMap(walker->reverseMap());
    for (auto fsReverseMap: reverseMap) {
      uint32_t volIndex(fsReverseMap.first);

      for (auto inodeMap: fsReverseMap.second) {
        std::string s = makeInodeID(volIndex, inodeMap.first);

        file << "{ \"id\":\"" << s
             << "\", \"t\": { \"hardlinks\":[";

        bool first = true;
        for (auto fileID: inodeMap.second) {
          if (!first) {
            file << ", ";
          }
          file << "\"" << fileID << "\"";
          first = false;
        }
        file << "]}}\n";
      }
    }
    file.close();
  }
}

int main(int argc, char *argv[]) {
  std::string command,
              ucMode,
              volMode,
              inodeMapFile,
              diskMapFile;
  uint64_t    maxUcBlockSize;

  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(&command), "command to perform [info|dumpimg|dumpfs|dumpfiles]")
    ("overview-file", po::value< std::string >(), "output disk overview information")
    ("unallocated", po::value< std::string >(&ucMode)->default_value("none"), "how to handle unallocated [none|fragment|block]")
    ("max-unallocated-block-size", po::value< uint64_t >(&maxUcBlockSize)->default_value(std::numeric_limits<uint64_t>::max()), "Maximum size of an unallocated entry, in blocks")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files")
    ("inode-map-file", po::value<std::string>(&inodeMapFile)->default_value(""), "optional file to output containing directory entry to inode map")
    ("disk-map-file", po::value<std::string>(&diskMapFile)->default_value(""), "optional file to output containing disk data to inode map");

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
    else if (vm.count("command") && vm.count("ev-files") && (walker = createVisitor(command, std::cout, imgSegs))) {
      std_binary_io();

      boost::scoped_array< const char* >  segments(new const char*[imgSegs.size()]);
      for (unsigned int i = 0; i < imgSegs.size(); ++i) {
        segments[i] = imgSegs[i].c_str();
      }
      if (0 == walker->openImageUtf8(imgSegs.size(), segments.get(), TSK_IMG_TYPE_DETECT, 0)) {
        if (vm.count("overview-file")) {
          std::ofstream file(vm["overview-file"].as<std::string>().c_str(), std::ios::out);
          file << *(walker->getImage(imgSegs));
          file.close();
        }

        walker->setVolFilterFlags((TSK_VS_PART_FLAG_ENUM)(TSK_VS_PART_FLAG_ALLOC | TSK_VS_PART_FLAG_UNALLOC | TSK_VS_PART_FLAG_META));
        walker->setFileFilterFlags((TSK_FS_DIR_WALK_FLAG_ENUM)(TSK_FS_DIR_WALK_FLAG_RECURSE | TSK_FS_DIR_WALK_FLAG_UNALLOC | TSK_FS_DIR_WALK_FLAG_ALLOC));
        if (ucMode == "fragment") {
          walker->setUnallocatedMode(LbtTskAuto::FRAGMENT);
          walker->setMaxUnallocatedBlockSize(maxUcBlockSize);
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
          std::vector<std::future<void>> futs;
          if (vm.count("disk-map-file") && command == "dumpfs") {
            futs.emplace_back(std::async(outputDiskMap, diskMapFile, walker));
          }
          if (vm.count("inode-map-file") && command == "dumpfs") {
            futs.emplace_back(std::async(outputInodeMap, inodeMapFile, walker));
          }
          for (auto& fut: futs) {
            fut.get();
          }
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
