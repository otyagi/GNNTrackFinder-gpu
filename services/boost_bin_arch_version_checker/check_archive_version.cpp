/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include <boost/archive/binary_iarchive.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{

  if (2 != argc) {
    std::cout << "This small programm needs a path to a boost archive as input argument!" << std::endl;
    return -1;
  }

  std::cout << "trying to check BOOST_ARCHIVE_VERSION of " << argv[1] << std::endl;

  std::ifstream ifs(argv[1], std::ios::binary);
  if (!ifs) {
    std::stringstream msg;
    msg << "file \"" << argv[1] << "\" was not found";
    throw std::runtime_error(msg.str());
  }

  boost::archive::binary_iarchive ia(ifs);
  unsigned v = ia.get_library_version();

  std::cout << "get_library_version() for " << argv[1] << "\n => " << v << std::endl;
  if (v < 17) {
    std::cout << "Any version of Fairsoft supported by Cbmroot should work" << std::endl;
  }
  else {
    switch (v) {
      case 17: {
        std::cout << "Minimal Fairsoft version: jun19 and its patch versions" << std::endl;
        break;
      }
      case 18: {
        std::cout << "Minimal Fairsoft version: apr21 and its patch versions" << std::endl;
        break;
      }
      case 19: {
        std::cout << "Minimal Fairsoft version: apr22 and its patch versions" << std::endl;
        break;
      }
      case 20:
      default: {
        std::cout << "No Fairsoft version supporting this file to date (10/04/2024)" << std::endl;
        break;
      }
    }
  }

  return v;
}
