/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "Application.h"

#include "CbmOnlineParWrite.h"

#include <FairLogger.h>

#include <TString.h>
#include <TSystem.h>

void Application::Run()
{
  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());

  LOG(info) << "Starting online parameter dump for setup " << fOpts.setup << "...";

  gSystem->mkdir(fOpts.outputDir.c_str(), kTRUE);
  gSystem->cd(fOpts.outputDir.c_str());

  CbmOnlineParWrite writer;
  CbmOnlineParWrite::Config writeConfig{
    .setupType   = fOpts.setup,
    .doAlignment = !fOpts.skipAlignment,
  };
  writer.Run(writeConfig);

  LOG(info) << "Online parameter dump finished";
}
