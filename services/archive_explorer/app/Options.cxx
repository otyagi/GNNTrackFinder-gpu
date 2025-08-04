/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "Options.h"

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

using namespace cbm::explore;

Options::Options(int argc, char** argv)
{
  po::options_description options("Options");

  // clang-format off
  options.add_options()
    ("input,i", po::value(&fInput)->value_name("<input>")->required(),
      "input archive")
    ("input2,j", po::value(&fInput2)->value_name("<input2>"),
      "second input archive, used for comparison (optional)")
    ("port,p", po::value(&fPort)->value_name("<port>")->default_value(8080),
      "port to listen on")
    ("sensor", po::value(&fSensor)->value_name("<sensor>"),
      "sensor to filter on")
    ("help,h",
      "produce help message")
  ;
  // clang-format on

  po::variables_map vm;
  po::command_line_parser parser {argc, argv};
  parser.options(options);
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
    std::cout << options << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  fFilterSensor = vm.count("sensor") > 0;

  try {
    po::notify(vm);
  }
  catch (const po::required_option& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Use '-h' to display all valid options." << std::endl;
    std::exit(EXIT_FAILURE);
  }
}
