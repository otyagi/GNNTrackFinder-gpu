/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "ProgramOptions.h"

#include <boost/program_options.hpp>

#include <functional>
#include <iostream>

namespace po = boost::program_options;

ProgramOptions::ProgramOptions(int argc, char** argv)
{
  po::options_description required("Required options");
  // clang-format off
  required.add_options()
    ("filename,f", po::value(&sFullFilename)->value_name("<outputDir>")->required(),
      "Full path to TSA file")
  ;
  // clang-format on

  po::options_description optional{"Other options"};
  // clang-format off
  optional.add_options()
    ("timeslices,n", po::value(&uNbTimeslices),
      "number of timeslices")
    ("sys,s", po::value<std::string>()->notifier(std::bind(&ProgramOptions::ConvertSysId, this, std::placeholders::_1)),
      "SysId to select (component level), in hex! 0x00 to 0xFF!")
    // Hint: cannot use std::underlying_type_t<fles::Subsystem> directly for two reasons (-_-)
    // 1) this resolves to uint8_t and boost then want a character as program argument, not a number (which is typically
    //    multiple characters)
    // 2) boost program_options cannot digest hexadecimal numbers (0xYY) as input
    ("mspercomp,m", po::value(&nbMsPerComp),
      "number of microslices to dump per component")
    ("help,h", "Print help message")
  ;
  // clang-format on


  po::options_description cmdline_options;
  cmdline_options.add(required).add(optional);

  po::variables_map vm;
  po::command_line_parser parser{argc, argv};
  parser.options(cmdline_options);
  try {
    auto result = parser.run();
    po::store(result, vm);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Use '-h' to display all valid options." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if (vm.count("help") > 0) {
    std::cout << cmdline_options << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  try {
    po::notify(vm);
  }
  catch (const po::required_option& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Use '-h' to display all valid options." << std::endl;
    std::exit(EXIT_FAILURE);
  }
  catch (const po::invalid_option_value& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Use '-h' to display all valid options." << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void ProgramOptions::ConvertSysId(const std::string& sOption)
{
  std::stringstream interpreter;
  interpreter << std::hex << sOption;
  interpreter >> selSysId;

  if (0xFF < selSysId) {
    std::stringstream ss;
    // Not sure why the option name is not shown at same place as in default error message, probably wrong step in boost
    // program_options calls sequence or wrong error type.... gave up, not important
    ss << sOption << ": Value out of range (bigger than 0xFF) for --sys";
    throw po::invalid_option_value(ss.str());
  }
}
