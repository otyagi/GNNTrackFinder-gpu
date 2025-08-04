/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file ProgramOptions.cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 27.10.2023
 **
 ** Code taken from reco/app/cbmreco_fairrun/ProgramOptions.cxx (J. de Cuveland)
 **/

#include "ProgramOptions.h"

#include <Logger.h>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

using std::string;
using std::vector;

namespace cbm::atconverter
{

  // -----   Parse command line   ---------------------------------------------
  void ProgramOptions::ParseOptions(int argc, char* argv[])
  {

    // --- Define generic options
    po::options_description generic("Generic options");
    auto generic_add = generic.add_options();
    generic_add("help,h", "display this help and exit");

    // --- Define configuration options
    string defconfig = std::getenv("VMCWORKDIR");
    defconfig.append("/");
    defconfig.append(DEFAULT_CONFIG);
    po::options_description config("Configuration");
    auto config_add = config.add_options();
    config_add("output,o", po::value<string>(&fOutput)->value_name("<file name>"),
               "name of the output ROOT file with analysistree data");
    config_add("transport,t", po::value<vector<string>>(&fTra)->value_name("<file names>")->multitoken(),
               "name of transport input sources (ROOT files)");
    config_add("digitization,d", po::value<string>(&fRaw)->value_name("<file name>"),
               "name of the raw ROOT file containing digi data");
    config_add("parameter,p", po::value<string>(&fPar)->value_name("<file name>"),
               "name of a parameter ROOT file (FairRuntimeDb format)");
    config_add("reconstruction,r", po::value<string>(&fReco)->value_name("<file nams>"),
               "name of a reconstruction ROOT file");
    config_add("config,c", po::value<string>(&fConfig)->value_name("<file name>")->default_value(defconfig),
               "name of a YAML file describing the configuration of reconstruction");
    config_add("setup,s", po::value<string>(&fSetup)->value_name("<tag>")->default_value(DEFAULT_SETUP),
               "geometry setup tag");
    config_add("overwrite,w", po::bool_switch(&fOverwrite)->default_value(false),
               "allow to overwite an existing output file");

    // --- Allowed options
    po::options_description cmdline_options("Allowed options");
    cmdline_options.add(generic).add(config);

    // --- Parse command line
    po::variables_map vars;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vars);
    po::notify(vars);

    // --- Help: print help information and exit program
    if (vars.count("help") != 0u) {
      std::cout << cmdline_options << std::endl;
      exit(EXIT_SUCCESS);
    }

    // --- Catch mandatory parameters not being specified
    if (vars.count("output") == 0u) { throw std::runtime_error("no output file name specified"); }
    if (vars.count("transport") == 0u) { throw std::runtime_error("no transport file name specified"); }
    if (vars.count("digitization") == 0u) { throw std::runtime_error("no digitization (raw) file name specified"); }
    if (vars.count("parameter") == 0u) { throw std::runtime_error("no parameter file name specified"); }
    if (vars.count("reconstruction") == 0u) { throw std::runtime_error("no reconstruction file name specified"); }
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::atconverter
