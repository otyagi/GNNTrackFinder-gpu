/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Denis Bertini [committer], Florian Uhlig, Sergei Zharko */

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
#include "CbmMuchDigitizerQa.h"
#include "CbmMuchHitFinderQa.h"
#include "CbmMuchTransportQa.h"
#include "CbmSetup.h"

#include <FairFileSource.h>
#include <FairMonitor.h>
#include <FairParAsciiFileIo.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <FairSystemInfo.h>

#include <TROOT.h>
#include <TStopwatch.h>
#include <TString.h>

#include <Logger>
#endif

void run_qa(TString data = "test", TString setup = "sis100_electron", Int_t nEvents = -1, TString dataTra2 = "",
            TString dataTra3 = "", TString configName = "", TString sEvBuildRaw = "");

/// @brief Main macro execution function
/// @param dataTraColl  Collision transport input
/// @param dataRaw      Digitization input
/// @param dataReco     Reconstruction input
/// @param dataPar      Parameter file
/// @param dataSink     QA output
/// @param setup        Setup name
/// @param nEvents      Number of events to procede
/// @param dataTraSign  Signal transport input
/// @param dataTraBeam  Beam transport input
/// @param config       QA configuration file input
/// @param sEvBuildRaw  Event builder type ("", "Ideal", "Real")
/// @param sBenchmark   Benchmark file input
/* clang-format off */
void run_qa(TString dataTraColl, 
            TString dataRaw, 
            TString dataReco, 
            TString dataPar, 
            TString dataSink,
            TString setup       = "sis100_electron", 
            Int_t nEvents       = -1, 
            TString dataTraSign = "", 
            TString dataTraBeam = "",
            TString config      = "", 
            TString sEvBuildRaw = "",
            TString sBenchmark  = ""
            )
/* clang-format on */
{

  gROOT->SetBatch(kTRUE);

  // ========================================================================
  //          Adjust this part according to your requirements

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  fair::Logger::DefineVerbosity(
    "user1", fair::VerbositySpec::Make(fair::VerbositySpec::Info::severity, fair::VerbositySpec::Info::file_line));
  FairLogger::GetLogger()->SetLogVerbosityLevel("user1");
  FairLogger::GetLogger()->SetColoredLog(true);
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  bool bUseMC    = true;                           // MC flag: used or not
  int verbose    = 3;                              // verbose level
  TString myName = "run_qa";                       // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------

  // -----   In- and output file names   ------------------------------------
  TString traFileColl  = dataTraColl + ".tra.root";
  TString traFileSign  = dataTraSign + ".tra.root";
  TString traFileBeam  = dataTraBeam + ".tra.root";
  TString rawFile      = dataRaw + ".raw.root";
  TString parFile      = dataPar + ".par.root";
  TString recFile      = dataReco + ".reco.root";
  TString sinkFile     = dataSink + ".qa.root";
  TString benchmarkOut = dataSink + ".qa.benchmark.root";
  TString qaConfig     = (config.Length() ? config : srcDir + "/macro/qa/configs/qa_tasks_config_" + setup + ".yaml");
  // ------------------------------------------------------------------------

  // -----   Load the geometry setup   -------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Loading setup " << setup << std::endl;
  auto* geo = CbmSetup::Instance();
  geo->LoadSetup(setup);

  // You can modify the pre-defined setup by using
  // CbmSetup::Instance()->RemoveModule(ESystemId) or
  // CbmSetup::Instance()->SetModule(ESystemId, const char*, Bool_t) or
  // CbmSetup::Instance()->SetActive(ESystemId, Bool_t)
  // See the class documentation of CbmSetup.
  // ------------------------------------------------------------------------

  // -----   Some global switches   -----------------------------------------
  ECbmRecoMode recoMode = !sEvBuildRaw.IsNull() ? ECbmRecoMode::EventByEvent : ECbmRecoMode::Timeslice;
  bool bUseMvd  = geo->IsActive(ECbmModuleId::kMvd);
  bool bUseSts  = geo->IsActive(ECbmModuleId::kSts);
  bool bUseRich = geo->IsActive(ECbmModuleId::kRich);
  bool bUseMuch = geo->IsActive(ECbmModuleId::kMuch);
  bool bUseTrd  = geo->IsActive(ECbmModuleId::kTrd);
  bool bUseTof  = geo->IsActive(ECbmModuleId::kTof);
  bool bUsePsd  = geo->IsActive(ECbmModuleId::kPsd);
  std::cout << "  MVD: " << (bUseMvd ? "ON" : "OFF") << '\n';
  std::cout << "  STS: " << (bUseSts ? "ON" : "OFF") << '\n';
  std::cout << "  RICH: " << (bUseRich ? "ON" : "OFF") << '\n';
  std::cout << "  MUCH: " << (bUseMuch ? "ON" : "OFF") << '\n';
  std::cout << "  TRD: " << (bUseTrd ? "ON" : "OFF") << '\n';
  std::cout << "  TOF: " << (bUseTof ? "ON" : "OFF") << '\n';
  std::cout << "  PSD: " << (bUsePsd ? "ON" : "OFF") << '\n';
  // ------------------------------------------------------------------------

  // -----   Parameter files as input to the runtime database   -------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Defining parameter files " << std::endl;
  TList* parFileList = new TList();
  TString geoTag;

  // - MUCH digitisation parameters
  TString muchParFile{};
  if (CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kMuch, geoTag)) {
    bool mcbmFlag = geoTag.Contains("mcbm", TString::kIgnoreCase);
    muchParFile   = srcDir + "/parameters/much/much_";
    muchParFile += (mcbmFlag) ? geoTag : geoTag(0, 4);
    muchParFile += "_digi_sector.root";
    {  // init geometry from the file
      TFile* f            = new TFile(muchParFile, "R");
      TObjArray* stations = (TObjArray*) f->Get("stations");
      CbmMuchGeoScheme::Instance()->Init(stations, mcbmFlag);
    }
  }

  // - TRD digitisation parameters
  if (CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kTrd, geoTag)) {
    const Char_t* npar[4] = {"asic", "digi", "gas", "gain"};
    TObjString* trdParFile(NULL);
    for (Int_t i(0); i < 4; i++) {
      trdParFile = new TObjString(srcDir + "/parameters/trd/trd_" + geoTag + "." + npar[i] + ".par");
      parFileList->Add(trdParFile);
      std::cout << "-I- " << myName << ": Using parameter file " << trdParFile->GetString() << std::endl;
    }
  }

  // - TOF digitisation parameters
  if (CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kTof, geoTag)) {
    TObjString* tofBdfFile = new TObjString(srcDir + "/parameters/tof/tof_" + geoTag + ".digibdf.par");
    parFileList->Add(tofBdfFile);
    std::cout << "-I- " << myName << ": Using parameter file " << tofBdfFile->GetString() << std::endl;
  }
  // ------------------------------------------------------------------------

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
  FairFileSource* inputSource = new FairFileSource(rawFile);
  inputSource->AddFriend(traFileColl);
  inputSource->AddFriend(recFile);
  if (!dataTraSign.IsNull()) inputSource->AddFriend(traFileSign);
  if (!dataTraBeam.IsNull()) inputSource->AddFriend(traFileBeam);

  FairRunAna* run = new FairRunAna();
  run->SetSource(inputSource);
  run->SetGenerateRunInfo(kFALSE);

  FairRootFileSink* sink = new FairRootFileSink(sinkFile);
  run->SetSink(sink);

  TString monitorFile{sinkFile};
  monitorFile.ReplaceAll("qa", "qa.monitor");
  FairMonitor::GetMonitor()->EnableMonitor(kTRUE, monitorFile);

  auto* qaManager = new CbmQaManager(verbose);
  //qaManager->SetConfigName(qaConfig);
  if (!sBenchmark.IsNull()) {
    qaManager->OpenBenchmarkInput(sBenchmark);
    qaManager->OpenBenchmarkOutput(benchmarkOut);
    qaManager->SetDefaultTag("default");
    qaManager->SetVersionTag("this");
  }
  run->AddTask(qaManager);
  // ------------------------------------------------------------------------

  // -----   MCDataManager  -----------------------------------
  // TODO: provide to mcbm_qa.C, run_reco_.....C, mcbm_reco_......C
  CbmMCDataManager* mcManager = new CbmMCDataManager("MCDataManager", 0);
  mcManager->AddFileToChain(traFileColl, 0);
  if (!dataTraSign.IsNull()) mcManager->AddFileToChain(traFileSign, 1);
  if (!dataTraBeam.IsNull()) mcManager->AddFileToChain(traFileBeam, 2);

  qaManager->AddTask(mcManager);
  // ------------------------------------------------------------------------

  qaManager->AddTask(new CbmTrackingDetectorInterfaceInit());  // Geometry interface initializer for tracker

  // ----- Match reco to MC ------
  if (bUseMC) {
    CbmMatchRecoToMC* matchTask = new CbmMatchRecoToMC();
    // NOTE: Matching of hits and clusters is suppressed when the matches are already present in the tree.
    matchTask->SuppressHitReMatching();
    qaManager->AddTask(matchTask);
  }
  // ----- MUCH QA  ---------------------------------
  if (bUseMuch) {
    qaManager->AddTask(new CbmMuchTransportQa());
    qaManager->AddTask(new CbmMuchDigitizerQa());
    CbmMuchHitFinderQa* muchHitFinderQa = new CbmMuchHitFinderQa();
    muchHitFinderQa->SetGeoFileName(muchParFile);
    qaManager->AddTask(muchHitFinderQa);
    qaManager->AddTask(new CbmCaInputQaMuch(verbose, bUseMC));
  }

  // ----- TRD QA  ---------------------------------
  if (bUseTrd) {
    qaManager->AddTask(new CbmTrdMCQa());
    //run->AddTask(new CbmTrdHitRateQa());  //opens lots of windows
    //run->AddTask(new CbmTrdDigitizerPRFQa()); //works put currently doesn't do anything
    //run->AddTask(new CbmTrdHitRateFastQa());  //opens lots of windows
    CbmTrdHitProducerQa* trdHitProducerQa = new CbmTrdHitProducerQa();
    if ("sis300_electron" == setup) {
      /// Larger number of stations, needed to fit geometry and avoid warning
      /// => fast-fix to accomodate the 4+4+2 complexity of tentative SIS300 geometry
      trdHitProducerQa->SetNumberStations(10);
    }
    qaManager->AddTask(trdHitProducerQa);
    qaManager->AddTask(new CbmTrdCalibTracker());
    qaManager->AddTask(new CbmCaInputQaTrd(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- TOF QA  ----------------------------------------------------------
  if (bUseTof) {
    qaManager->AddTask(new CbmCaInputQaTof(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- MVD QA  ----------------------------------------------------------
  if (bUseMvd && !sEvBuildRaw.EqualTo("Real", TString::ECaseCompare::kIgnoreCase)) {
    qaManager->AddTask(new CbmCaInputQaMvd(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- STS QA  ----------------------------------------------------------
  if (bUseSts) {
    qaManager->AddTask(new CbmCaInputQaSts(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- Event builder QA  ---------------------------------
  qaManager->AddTask(new CbmBuildEventsQa());
  // ------------------------------------------------------------------------

  // ----- Tracking QA ------------------------------------------------------
  // KF is currently needed to access magnetic field. In future we will
  // delegate track fit routines to CbmKF as well.
  qaManager->AddTask(new CbmKF());  // TODO: Needed?

  TString caParFile = recFile;
  caParFile.ReplaceAll(".root", ".ca.par");

  auto* pCaInputQaSetup = new cbm::ca::InputQaSetup(verbose, bUseMC);
  pCaInputQaSetup->ReadParameters(caParFile.Data());
  pCaInputQaSetup->SetSetupName(setup.Data());
  qaManager->AddTask(pCaInputQaSetup);

  auto* pCaOutputQa = new cbm::ca::OutputQa(verbose, bUseMC, recoMode);
  pCaOutputQa->SetStsTrackingMode();
  pCaOutputQa->ReadParameters(caParFile.Data());
  if (config.Length() != 0) {
    pCaOutputQa->SetConfigName(config);
  }
  // TODO: Provide detector selection interface from the L1Parameters
  pCaOutputQa->SetUseMvd(!(sEvBuildRaw.EqualTo("Real", TString::ECaseCompare::kIgnoreCase)) ? bUseMvd : false);
  pCaOutputQa->SetUseSts(bUseSts);
  //pCaOutputQa->SetUseMuch(bUseMuch);
  //pCaOutputQa->SetUseTrd(bUseTrd);
  //pCaOutputQa->SetUseTof(bUseTof);
  qaManager->AddTask(pCaOutputQa);
  // ------------------------------------------------------------------------

  // -----  Parameter database   --------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Set runtime DB" << std::endl;
  FairRuntimeDb* rtdb       = run->GetRuntimeDb();
  FairParRootFileIo* parIo1 = new FairParRootFileIo();
  parIo1->open(parFile.Data(), "in");
  rtdb->setFirstInput(parIo1);
  if (!parFileList->IsEmpty()) {
    FairParAsciiFileIo* parIo2 = new FairParAsciiFileIo();
    parIo2->open(parFileList, "in");
    rtdb->setSecondInput(parIo2);
  }
  rtdb->print();
  // ------------------------------------------------------------------------

  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Initialise run" << std::endl;
  run->Init();

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
  std::cout << " QA checks " << (qaManager->GetStatus() ? "\e[1;32mpassed" : "\e[1;31mfailed") << "\e[0m\n";
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

void run_qa(TString data, TString setup, Int_t nEvents, TString dataTra2, TString dataTra3, TString configName,
            TString sEvBuildRaw)
{
  run_qa(data, data, data, data, data, setup, nEvents, dataTra2, dataTra3, configName, sEvBuildRaw);
}
