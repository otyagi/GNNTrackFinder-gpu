/* Copyright (C) 2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

// --------------------------------------------------------------------------
//
// Macro for simulation & reconstruction QA
//
// The following naming conventions are assumed:
// Raw data file:  [dataset].event.raw.root
// Transport file: [dataset].tra.root
// Parameter file:  [dataset].par.root
// Reconstruction file: [dataset].rec.root
//
// S. Gorbunov 28/09/2020
//
// --------------------------------------------------------------------------

// Includes needed for IDE
#if !defined(__CLING__)
#include "CbmDefs.h"
#include "CbmMCDataManager.h"
#include "CbmMuchTransportQa.h"
#include "CbmQaManager.h"
#include "CbmSetup.h"

#include <FairFileSource.h>
#include <FairMonitor.h>
#include <FairParAsciiFileIo.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <FairSystemInfo.h>

#include <TStopwatch.h>
#endif

void run_BbaAlignment_mcbm(Int_t nEvents = -1, TString dataset = "data/mcbm_beam_2020_03_test",
                           TString setupName = "mcbm_beam_2022_05_23_nickel", double SimulatedMisalignmentRange = 0.,
                           Bool_t bUseMC = kFALSE)
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("LOW");
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  int verbose    = 6;                              // verbose level
  TString myName = "BBA Alignment";                // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // NOTE: SZh 28.07.2024: config can depend from the setup
  // ------------------------------------------------------------------------

  // -----   In- and output file names   ------------------------------------
  TString traFile = dataset + ".tra.root";
  //TString rawFile  = dataset + ".event.raw.root";
  TString parFile  = dataset + ".par.root";
  TString recFile  = dataset + ".rec.root";
  TString sinkFile = dataset + ".ali.root";
  TString geoFile  = dataset + ".geo.root";

  // ------------------------------------------------------------------------

  // -----   Load the geometry setup   -------------------------------------
  std::cout << '\n';

  CbmSetup* setup = CbmSetup::Instance();
  setup->LoadSetup(setupName);

  // In general, the following parts need not be touched
  // ========================================================================

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // ----    Debug option   -------------------------------------------------
  gDebug = 0;
  // ------------------------------------------------------------------------

  // -----   FairRunAna   ---------------------------------------------------
  FairFileSource* inputSource = new FairFileSource(recFile);
  if (bUseMC) {
    inputSource->AddFriend(traFile);
  }
  //inputSource->AddFriend(rawFile);

  FairRunAna* run = new FairRunAna();
  run->SetSource(inputSource);
  run->SetGenerateRunInfo(kFALSE);
  run->SetGeomFile(geoFile);

  FairRootFileSink* sink = new FairRootFileSink(sinkFile);
  run->SetSink(sink);

  TString monitorFile{sinkFile};
  monitorFile.ReplaceAll("qa", "qa.monitor");
  FairMonitor::GetMonitor()->EnableMonitor(kTRUE, monitorFile);
  // ------------------------------------------------------------------------

  // -----   MCDataManager (legacy mode)  -----------------------------------
  if (bUseMC) {
    std::cout << "-I- " << myName << ": Adding MC manager and MC to reco matching tasks\n";

    auto* mcManager = new CbmMCDataManager("MCDataManager", 1);
    mcManager->AddFile(traFile);
    run->AddTask(mcManager);

    auto* matchRecoToMC = new CbmMatchRecoToMC();
    // NOTE: Matching is suppressed, if there are hit and cluster matches already in the tree. If there
    //       are no hit matches, they are produced on this stage.
    matchRecoToMC->SuppressHitReMatching();
    run->AddTask(matchRecoToMC);
  }
  // ------------------------------------------------------------------------

  // ----- Tracking detector interface --------------------------------------
  run->AddTask(new CbmTrackingDetectorInterfaceInit());
  // ------------------------------------------------------------------------


  // ------------------------------------------------------------------------

  // TODO: read tracking parameters from the file like CaOutputQa does
  //TODO:  instead of initializing the tracker
  //TString caParFile = recFile;
  //caParFile.ReplaceAll(".root", ".ca.par");
  //auto* pCaOutputQa = new cbm::ca::OutputQa(verbose, bUseMC);
  //pCaOutputQa->ReadParameters(caParFile.Data());

  // L1 CA track finder setup
  auto l1 = new CbmL1("CA");
  l1->SetMcbmMode();

  // User configuration example for CA:
  //l1->SetConfigUser(srcDir + "/macro/L1/configs/ca_params_user_example.yaml");
  run->AddTask(l1);

  // ----- BBA alignment   --------------------------------------------
  CbmBbaAlignmentTask* alignment = new CbmBbaAlignmentTask();
  alignment->SetMcbmTrackingMode();
  alignment->SetSimulatedMisalignmentRange(SimulatedMisalignmentRange);

  run->AddTask(alignment);
  run->SetGenerateRunInfo(kFALSE);

  // -----  Parameter database   --------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Set runtime DB" << std::endl;
  FairRuntimeDb* rtdb       = run->GetRuntimeDb();
  FairParRootFileIo* parIo1 = new FairParRootFileIo();
  parIo1->open(parFile.Data(), "in");
  rtdb->setFirstInput(parIo1);

  // ------------------------------------------------------------------------

  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Initialise run" << std::endl;
  run->Init();

  rtdb->print();

  // -----   Start run   ----------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Starting run" << std::endl;
  run->Run(0, nEvents);
  // ------------------------------------------------------------------------

  // -----   Finish   -------------------------------------------------------
  timer.Stop();


  FairMonitor::GetMonitor()->Print();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Output file is " << sinkFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << std::endl;
  std::cout << std::endl;
  std::cout << " Test passed" << std::endl;
  std::cout << " All ok " << std::endl;
  // ------------------------------------------------------------------------

  // -----   Resource monitoring   ------------------------------------------
  // Extract the maximal used memory an add is as Dart measurement
  // This line is filtered by CTest and the value send to CDash
  FairSystemInfo sysInfo;
  Float_t maxMemory = sysInfo.GetMaxMemory();
  std::cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  std::cout << maxMemory;
  std::cout << "</DartMeasurement>" << std::endl;

  Float_t cpuUsage = ctime / rtime;
  std::cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  std::cout << cpuUsage;
  std::cout << "</DartMeasurement>" << std::endl;
  // ------------------------------------------------------------------------

  // -----   Function needed for CTest runtime dependency   -----------------
  RemoveGeoManager();
  // ------------------------------------------------------------------------
}
