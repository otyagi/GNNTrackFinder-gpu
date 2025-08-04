/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "ProgramOptions.h"

#include <Logger.h>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

using std::string;

namespace cbm::services::histserv_tester
{

  // -----   Parse command line   ---------------------------------------------
  void ProgramOptions::ParseOptions(int argc, char* argv[])
  {

    // --- Define generic options
    po::options_description generic("  Generic options");
    auto generic_add = generic.add_options();
    generic_add("help,h", "display this help and exit");

    // --- Define configuration options: Mandatory
    po::options_description config_req("  Configuration (required)");
    auto config_req_add = config_req.add_options();
    config_req_add("output,o", po::value<string>(&fsChanHistosIn)->required()->value_name("<protocol://xxxxxx>"),
                   "name or host:port or whatever is needed for output channel (histos/canvases config and data), "
                   "cf http://api.zeromq.org/2-1:zmq-connect");

    // --- Define configuration options: Optional
    po::options_description config("  Configuration (optional)");
    auto config_add = config.add_options();
    config_add("runtime,r", po::value<int64_t>(&fRunTime)->default_value(90),
               "duration of test run in seconds ~ x4 entries in test histogram ");
    config_add("pubint,p", po::value<int64_t>(&fPubInterval)->default_value(5),
               "publication interval in seconds (accumulate statistics between message emissions)");

    // --- Allowed options
    po::options_description cmdline_options("Allowed options");
    cmdline_options.add(generic).add(config_req).add(config);

    // --- Parse command line
    po::variables_map vars;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vars);

    // --- Help: print help information and exit program
    if (vars.count("help") != 0u) {
      std::cout << cmdline_options << std::endl;
      exit(EXIT_SUCCESS);
    }

    // --- Run notify after processing the help to avoid it being blocked by missing arguments
    po::notify(vars);
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::services::histserv_tester
