/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "ProgramOptions.h"

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

ProgramOptions::ProgramOptions(int argc, char** argv)
{
  po::options_description required("Required options");
  // clang-format off
  required.add_options()
    ("setup,s", po::value(&setup)->value_name("<setup>")->required(),
      "Setup: mCBM2022, mCBM2024")
    ("outdir,o", po::value(&outputDir)->value_name("<outputDir>")->required(),
      "Output directory for the parameter files")
  ;
  // clang-format on

  po::options_description optional{"Other options"};
  // clang-format off
  optional.add_options()
    ("no-alignment", po::bool_switch(&skipAlignment),
      "Do not do alignment")
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
}
