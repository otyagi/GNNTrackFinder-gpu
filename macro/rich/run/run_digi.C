/* Copyright (C) 2018-2023 UGiessen, JINR-LIT
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Andrey Lebedev, Martin Beyer */

#if !defined(__CLING__) || defined(__ROOTCLING__)
#include "CbmDigitization.h"
#include "CbmRichDigitizer.h"

#include <FairLogger.h>

#include <TRandom.h>
#include <TStopwatch.h>
#include <TTree.h>

#include <iostream>
#endif

void run_digi(TString traFile = "", TString parFile = "", TString digiFile = "",  // i/o files
              Int_t nEvents = -1, Double_t eventRate = -1.e7, Double_t tsLength = -1, Int_t randomSeed = 0,
              Bool_t monitor = true)
{
  TTree::SetMaxTreeSize(90000000000);

  gRandom->SetSeed(randomSeed);

  // -----   Files   --------------------------------------------------------
  TString macroPath = __FILE__;
  TString macroDir  = macroPath(0, macroPath.Last('/') + 1);
  if (traFile.IsNull()) traFile = macroDir + "data/test.tra.root";
  if (parFile.IsNull()) parFile = macroDir + "data/test.par.root";
  if (digiFile.IsNull()) digiFile = macroDir + "data/test.digi.root";

  remove(digiFile);
  // ------------------------------------------------------------------------

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("LOW");
  // ------------------------------------------------------------------------

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // -----   Digitization run   ---------------------------------------------
  CbmDigitization run;
  cbm::sim::Mode mode         = (eventRate < 0. ? cbm::sim::Mode::EventByEvent : cbm::sim::Mode::Timebased);
  cbm::sim::TimeDist timeDist = cbm::sim::TimeDist::Poisson;
  run.AddInput(0, traFile, timeDist, eventRate);
  run.SetOutputFile(digiFile);
  if (monitor) run.SetMonitorFile(" ");
  run.SetParameterRootFile(parFile);
  run.GenerateRunInfo(kFALSE);
  run.SetTimeSliceLength(tsLength);
  run.SetMode(mode);
  run.SetProduceNoise(false);
  // run.DeactivateAllBut(ECbmModuleId::kRich);

  // -----   Rich Digitizer   -----------------------------------------------
  CbmRichDigitizer* richDigitizer = new CbmRichDigitizer();
  run.SetDigitizer(ECbmModuleId::kRich, richDigitizer, "RichPoint", true);
  // ------------------------------------------------------------------------

  (nEvents < 0) ? run.Run() : run.Run(nEvents);
  // ------------------------------------------------------------------------

  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  std::cout << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Digi file is " << digiFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << timer.RealTime() << " s, CPU time " << timer.CpuTime() << " s" << std::endl;
  std::cout << std::endl << "Test passed" << std::endl << "All ok" << std::endl;
  // ------------------------------------------------------------------------
}
