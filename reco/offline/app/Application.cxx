/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file Application.cxx
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 09.06.2023
 **/

#include "Application.h"

#include "Config.h"

using std::string;

namespace cbm::reco::offline
{

  // -----   Constructor   ----------------------------------------------------
  Application::Application(ProgramOptions const& opt) : fOpt(opt) {}
  // --------------------------------------------------------------------------


  // -----   Run the reconstruction   -----------------------------------------
  void Application::Exec()
  {

    // --- Use program options
    fRun.SetOutput(fOpt.OutputFile().c_str());
    fRun.SetRawFile(fOpt.RawFile().c_str());
    fRun.SetParFile(fOpt.ParFile().c_str());
    fRun.SetGeoSetupTag(fOpt.SetupTag().c_str());
    if (fOpt.Overwrite()) fRun.AllowOverwrite();

    // --- Read configuration from YAML file
    cbm::reco::offline::Config config;
    config.LoadYaml(fOpt.ConfigFile());
    fRun.SetConfig(config);

    // --- Execute reconstruction run
    fRun.Exec();
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::reco::offline
