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
#include <boost/shared_ptr.hpp>

#include "walkers.h"

namespace po = boost::program_options;

using namespace std;

void printHelp(const po::options_description& desc) {
  std::cout << "fsrip, Copyright (c) 2010-2012, Lightbox Technologies, Inc." << std::endl;
  std::cout << "Built " << __DATE__ << std::endl;
  std::cout << "TSK version is " << tsk_version_get_str() << std::endl;
  std::cout << desc << std::endl;
}

shared_ptr<LbtTskAuto> createVisitor(const string& cmd, ostream& out, const vector<string>& segments) {
  if (cmd == "dumpimg") {
    return shared_ptr<LbtTskAuto>(new ImageDumper(out));
  }
  else if (cmd == "dumpfs") {
    return shared_ptr<LbtTskAuto>(new MetadataWriter(out));
  }
  else if (cmd == "count") {
    return shared_ptr<LbtTskAuto>(new FileCounter(out));
  }
  else if (cmd == "info") {
    return shared_ptr<LbtTskAuto>(new ImageInfo(out, segments));
  }
  else {
    return shared_ptr<LbtTskAuto>();
  }
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed Options");
  po::positional_options_description posOpts;
  posOpts.add("command", 1);
  posOpts.add("ev-files", -1);
  desc.add_options()
    ("help", "produce help message")
    ("command", po::value< std::string >(), "command to perform [info|dumpimg|dumpfs|count]")
    ("ev-files", po::value< std::vector< std::string > >(), "evidence files");

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(posOpts).run(), vm);
    po::notify(vm);

    shared_ptr<LbtTskAuto> walker;

    std::vector< string > imgSegs;
    if (vm.count("ev-files")) {
      imgSegs = vm["ev-files"].as< vector< string > >();
    }
    if (vm.count("help")) {
      printHelp(desc);
    }
    else if (vm.count("command") && vm.count("ev-files") && (walker = createVisitor(vm["command"].as<string>(), cout, imgSegs))) {
      scoped_array< TSK_TCHAR* >  segments(new TSK_TCHAR*[imgSegs.size()]);
      for (unsigned int i = 0; i < imgSegs.size(); ++i) {
        segments[i] = (TSK_TCHAR*)imgSegs[i].c_str();
      }
      if (0 == walker->openImage(imgSegs.size(), segments.get(), TSK_IMG_TYPE_DETECT, 0)) {
        if (0 == walker->start()) {
          walker->finishWalk();
          return 0;
        }
        else {
          cerr << "Had an error parsing filesystem" << endl;
        }
      }
      else {
        cerr << "Had an error opening the evidence file" << endl;
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
