/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Jan de Cuveland, Volker Friese, Pierre-Alain Loizeau, Pascal Raisig [committer], Dominik Smith, Adrian A. Weber  */

/** @file run_unpack_tsa_algo.C
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since May 2021
 **/


// --- Includes needed for IDE
#include <RtypesCore.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#if !defined(__CLING__)
#include "CbmTrdRawMessageSpadic.h"
#include "CbmTrdSpadic.h"

#include <FairLogger.h>
#include <FairRootFileSink.h>
#include <FairRunOnline.h>
#include <Logger.h>

#include <TStopwatch.h>
#include <TSystem.h>
#endif

void run_unpack_tsa_algo(std::string infile, std::string outpath, std::string paramsdir, UInt_t runid,
                         std::int32_t nevents = -1, bool bCbmrootLikeOutput = false)
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  TString myName = "run_unpack_tsa_algo";          // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   Output filename   ----------------------------------------------
  std::string outfilename = infile;
  auto filenamepos        = infile.find_last_of("/");
  filenamepos++;
  std::string filename = infile.substr(filenamepos);
  if (filename.find("*") != infile.npos) filename = std::to_string(runid) + ".tsa";
  if (filename.find(";") != infile.npos) filename = std::to_string(runid) + "_merged" + ".tsa";
  if (outpath.empty()) {
    outpath = infile.substr(0, filenamepos);
  }
  outfilename = outpath + filename;
  outfilename.replace(outfilename.find(".tsa"), 4, "_algo.digi.root");
  std::cout << "-I- " << myName << ": Output file will be " << outfilename << std::endl;
  // ------------------------------------------------------------------------


  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  auto source = new CbmSourceTs(infile.c_str());

  // -----   FairRunAna   ---------------------------------------------------
  auto run  = new FairRunOnline(source);
  auto sink = new FairRootFileSink(outfilename.data());

  // -----    Algo Unpacker -------------------------------------------------
  //run->AddTask(new CbmTaskUnpack());
  auto unpackTask = new CbmTaskUnpack(paramsdir, runid);
  unpackTask->SetOutputModeCbmrootLike(bCbmrootLikeOutput);
  run->AddTask(unpackTask);

  run->SetSink(sink);
  auto eventheader = new CbmTsEventHeader();
  run->SetRunId(runid);
  run->SetEventHeader(eventheader);
  // ------------------------------------------------------------------------

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());
  // ------------------------------------------------------------------------

  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Initialise run" << std::endl;
  run->Init();
  // ------------------------------------------------------------------------

  // -----   Start run   ----------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Starting run" << std::endl;
  if (nevents < 0)
    run->Run(-1, 0);
  else
    run->Run(0, nevents);
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "After CpuTime = " << timer.CpuTime() << " s RealTime = " << timer.RealTime() << " s." << std::endl;
  // ------------------------------------------------------------------------

  // --   Release all shared pointers to config before ROOT destroys things -
  // => We need to destroy things by hand because run->Finish calls (trhought the FairRootManager) Source->Close which
  //    does call the Source destructor, so due to share pointer things stay alive until out of macro scope...
  run->SetSource(nullptr);
  delete run;
  // ------------------------------------------------------------------------

}  // End of main macro function
