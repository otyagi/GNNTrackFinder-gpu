/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "ProgramOptions.h"

#include <Logger.h>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

using std::string;

namespace cbm::services::histserv
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
    config_req_add("input,i", po::value<string>(&fsChanHistosIn)->required()->value_name("<protocol://xxxxxx>"),
                   "name or host:port or whatever is needed for input channel (histos/canvases config and data), "
                   "cf http://api.zeromq.org/2-1:zmq-bind");

    // --- Define configuration options: Optional
    po::options_description config("  Configuration (optional or with default)");
    auto config_add = config.add_options();
    config_add("highwater,hwm", po::value<int32_t>(&fiHistosInZmqHwm)->default_value(1),
               "ZMQ input channel high-water mark in messages: 0 = no buffering, val = nb updates kept in buffer \n"
               "Tune to match nb of update emitters but also to avoid extrem memory usage!");
    config_add("timeout,zto", po::value<int32_t>(&fiHistosInZmqRcvToMs)->default_value(10),
               "ZMQ input channel timeout in ms: -1 = block (term. handling!), 0 = instant (CPU load!), val = nb ms");
    config_add("port,p", po::value<uint32_t>(&fuHttpServerPort)->default_value(8080),
               "port on which the http ROOT server (JSroot) will be available (mind default value!)");
    config_add("output,o", po::value<string>(&fsHistoFileName)->value_name("<file name>"),
               "name of the output ROOT file with histograms backup");
    config_add("overwrite,w", po::bool_switch(&fOverwrite)->default_value(false),
               "allow to overwite an existing output file");
    config_add("hideguicmds", po::bool_switch(&fHideGuiCommands)->default_value(false),
               "allow to hides (disable) the GUI commands for Reset/Save/Close");
#ifdef BOOST_IOS_HAS_ZSTD
    config_add("compressed,c", po::bool_switch(&fCompressedInput)->default_value(false),
               "enables ZSTD decompression of the input stream (compression needed in source!)");
#endif

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

}  // namespace cbm::services::histserv
