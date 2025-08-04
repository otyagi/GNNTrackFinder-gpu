/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Application.cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 09.01.2024
 **/

#include "Application.h"

#include "Config.h"

using std::string;

namespace cbm::sim::digitization
{

  // -----   Constructor   ----------------------------------------------------
  Application::Application(ProgramOptions const& opt) : fOpt(opt) {}
  // --------------------------------------------------------------------------


  // -----   Run the reconstruction   -----------------------------------------
  void Application::Exec()
  {

    // --- Use program options
    fRun.SetOutput(fOpt.OutputFile().c_str());
    fRun.SetTraFiles(fOpt.TraFiles());
    fRun.SetParFile(fOpt.ParFile().c_str());
    fRun.SetGeoSetupTag(fOpt.SetupTag().c_str());
    if (fOpt.Overwrite()) fRun.AllowOverwrite();

    // --- Read configuration from YAML file
    cbm::sim::digitization::Config config;
    config.LoadYaml(fOpt.ConfigFile());
    fRun.SetConfig(config);

    // --- Execute reconstruction run
    fRun.Exec();
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::sim::digitization
