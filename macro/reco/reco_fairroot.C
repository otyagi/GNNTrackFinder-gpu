/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


// --- Includes needed for IDE code analyser
#if !defined(__CLING__)
#include "CbmReco.h"

#include <TStopwatch.h>
#include <TSystem.h>

#include <iostream>

#include "algo/evbuild/Config.h"
#endif

#include <FairLogger.h>


using std::string;

/** @brief Macro for CBM reconstruction from FLES timeslices or digi level
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since  12 March 2022
 ** @param tsaFile    Name of input file (.tsa or .root)
 ** @param outFile    Name of output file
 ** @param numTs      Number of timeslices to process. If not specified, all available will be used.
 ** @param port       Port of http server. If 0, server will not be activated.
 **
 ** Reconstruction from timeslice level with FairRoot, making use of the steering class CbmReco.
 ** Currently included stages:
 ** - Unpacking (STS only) (if from timeslice level)
 ** - Event trigger based on STS digis (CbmTaskDigiTrigger)
 ** - Event building (CbmTaskBuildEvents) (STS only)
 **
 ** If the input file name is left empty, a default file from the repository will be used.
 **/

void reco_fairroot(TString tsaFile, TString configFile, TString outFile,
                   int32_t numTs = std::numeric_limits<int32_t>::max(), uint16_t port = 0)
{


  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------


  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());
  // ------------------------------------------------------------------------


  // -----   Environment   --------------------------------------------------
  TString myName = "reco_fairoot";  // this macro's name for screen output
  // ------------------------------------------------------------------------


  // -----   Run reconstruction   -------------------------------------------
  TStopwatch timer;
  timer.Start();
  CbmReco::Config config;
  CbmReco run(config, tsaFile.Data(), outFile.Data(), configFile.Data(), numTs, port);
  run.Run();
  timer.Stop();
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << myName << ": Macro finished successfully." << std::endl;
  std::cout << myName << ": CPU time = " << timer.CpuTime() << " s, real time = " << timer.RealTime() << " s."
            << std::endl;
  // ------------------------------------------------------------------------

}  // End of main macro function
