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
#include "CbmCaInputQaMuch.h"
#include "CbmCaInputQaSetup.h"
#include "CbmCaInputQaSts.h"
#include "CbmCaInputQaTof.h"
#include "CbmCaInputQaTrd.h"
#include "CbmCaOutputQa.h"
#include "CbmDefs.h"
#include "CbmKF.h"
#include "CbmMCDataManager.h"
#include "CbmMatchRecoToMC.h"
#include "CbmMuchDigitizerQa.h"
#include "CbmMuchGeoScheme.h"
#include "CbmMuchHitFinderQa.h"
#include "CbmMuchTransportQa.h"
#include "CbmQaManager.h"
#include "CbmSetup.h"
#include "CbmTrackingDetectorInterfaceInit.h"

#include <FairFileSource.h>
#include <FairMonitor.h>
#include <FairParAsciiFileIo.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <FairSystemInfo.h>

#include <TObjString.h>
#include <TStopwatch.h>
#endif

/* clang-format off */
/// \brief QA macro execution function
/// \param nEvents        Number of events to proceed
/// \param dataset        Prefix of the output files files upstream the simulation/reconstruction chain
/// \param setupName      Name of the setup
/// \param bUseMC         Flag for MC (simulation) usage
/// \param config         QA YAML configuraiton file
/// \param benchmarkInput Path to a benchmark QA output, obtained for a given setup and given number of events
void mcbm_qa(Int_t nEvents = 0,
             TString dataset = "data/mcbm_beam_2020_03_test",
             TString setupName = "mcbm_beam_2020_03",
             Bool_t bUseMC = kTRUE,
             TString config = "",
             TString benchmarkInput = "");
/* clang-format on */

/* clang-format off */
/// \brief Qa macro execution function with independent input names
/// \param nEvents        Number of events to proceed
/// \param traColFile     Collision transport input
/// \param rawFile        Digi input
/// \param recFile        Reconstruction input
/// \param sinkFile       Output filename
/// \param setupName      Name of the setup (TODO: can be a run number)
/// \param bUseMC         Flag for MC (simulation) usage
/// \param config         QA YAML configuraiton file
/// \param benchmarkInput Path to a benchmark QA output, obtained for a given setup and given number of events
/// \param bUseAlignment  Flag for enabling alignment matrices usage
void mcbm_qa(Int_t nEvents = 0,
             TString traColFile = "data/mcbm_beam_2022_05_nickel.tra.root",
             TString rawFile = "data/mcbm_beam_2022_05_nickel.event.raw.root",
             TString recFile = "data/mcbm_beam_2022_05_nickel.rec.root",
             TString parFile = "data/mcbm_beam_2022_05_nickel.par.root",
             TString geoFile = "data/mcbm_beam_2022_05_nickel.geo.root",
             TString sinkFile = "data/mcbm_beam_2022_05_nickel.qa.root",
             TString setupName = "mcbm_beam_2022_05_nickel",
             Bool_t bUseMC = kTRUE,
             TString config = "",
             TString benchmarkInput = "",
             Bool_t bUseAlignment = kFALSE)
/* clang-format on */
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("HIGH");
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  int verbose    = 2;                              // verbose level
  TString myName = "mcbm_qa";                      // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------

  // -----   In- and output file names   ------------------------------------
  TString qaConfig = (config.Length() ? config : srcDir + "/macro/qa/configs/qa_tasks_config_" + setupName + ".yaml");
  if (gSystem->AccessPathName(qaConfig.Data())) {  // file not found, using default one
    std::cout << "-I- " << myName << ": the QA configuration file " << qaConfig << " not found, using default one\n";
    qaConfig = srcDir + "/macro/qa/configs/qa_tasks_config_mcbm.yaml";
  }
  TString benchmarkOut = sinkFile + ".qa.benchmark.root";


  std::cout << "Use MC?     " << (bUseMC ? "Yes" : "No") << '\n';
  std::cout << "Digi input: " << rawFile << '\n';
  std::cout << "Reco input: " << recFile << '\n';
  std::cout << "Par input:  " << parFile << '\n';
  std::cout << "Geo input:  " << geoFile << '\n';
  if (bUseMC) {
    std::cout << "\nTransport input:  " << traColFile << '\n';
  }

  // ------------------------------------------------------------------------

  // -----   Load the geometry setup   -------------------------------------
  std::cout << '\n';
  TString setupFile  = srcDir + "/geometry/setup/setup_" + setupName + ".C";
  TString setupFunct = "setup_";
  setupFunct         = setupFunct + setupName + "()";
  std::cout << "-I- " << myName << ": Loading macro " << setupFile << '\n';
  gROOT->LoadMacro(setupFile);
  gROOT->ProcessLine(setupFunct);
  CbmSetup* setup = CbmSetup::Instance();
  setup->GetProvider()->GetSetup().SetTag(setupName.Data());
  //  setup->RemoveModule(ECbmModuleId::kTrd);
  // ------------------------------------------------------------------------

  // -----   Some global switches   -----------------------------------------
  //if (setupName == "mcbm_beam_2022_05_23_nickel") { setup->RemoveModule(ECbmModuleId::kMuch); }

  //bool eventBased = !sEvBuildRaw.IsNull();
  bool bUseMvd  = setup->IsActive(ECbmModuleId::kMvd);
  bool bUseSts  = setup->IsActive(ECbmModuleId::kSts);
  bool bUseRich = setup->IsActive(ECbmModuleId::kRich);
  bool bUseMuch = setup->IsActive(ECbmModuleId::kMuch);
  bool bUseTrd  = setup->IsActive(ECbmModuleId::kTrd);
  bool bUseTof  = setup->IsActive(ECbmModuleId::kTof);
  bool bUsePsd  = setup->IsActive(ECbmModuleId::kPsd);
  std::cout << "  MVD: " << (bUseMvd ? "ON" : "OFF") << '\n';
  std::cout << "  STS: " << (bUseSts ? "ON" : "OFF") << '\n';
  std::cout << "  RICH: " << (bUseRich ? "ON" : "OFF") << '\n';
  std::cout << "  MUCH: " << (bUseMuch ? "ON" : "OFF") << '\n';
  std::cout << "  TRD: " << (bUseTrd ? "ON" : "OFF") << '\n';
  std::cout << "  TOF: " << (bUseTof ? "ON" : "OFF") << '\n';
  std::cout << "  PSD: " << (bUsePsd ? "ON" : "OFF") << '\n';
  // ------------------------------------------------------------------------

  // -----   Parameter files as input to the runtime database   -------------
  std::cout << '\n';
  std::cout << "-I- " << myName << ": Defining paramete files\n";
  TList* parFileList = new TList();
  TString geoTag;

  // - MUCH digitisation parameters
  TString muchParFile{};
  if (setup->GetGeoTag(ECbmModuleId::kMuch, geoTag)) {
    bool mcbmFlag = geoTag.Contains("mcbm", TString::kIgnoreCase);
    muchParFile   = srcDir + "/parameters/much/much_";
    muchParFile += (mcbmFlag) ? geoTag : geoTag(0, 4);
    muchParFile += "_digi_sector.root";
    {  // init geometry from the file
      TFile* f            = new TFile(muchParFile, "R");
      TObjArray* stations = f->Get<TObjArray>("stations");
      assert(stations);
      CbmMuchGeoScheme::Instance()->Init(stations, mcbmFlag);
    }
  }

  // - TRD digitisation parameters
  if (setup->GetGeoTag(ECbmModuleId::kTrd, geoTag)) {
    const Char_t* npar[4] = {"asic", "digi", "gas", "gain"};
    TObjString* trdParFile(NULL);
    for (Int_t i(0); i < 4; i++) {
      trdParFile = new TObjString(srcDir + "/parameters/trd/trd_" + geoTag + "." + npar[i] + ".par");
      parFileList->Add(trdParFile);
      std::cout << "-I- " << myName << ": Using parameter file " << trdParFile->GetString() << std::endl;
    }
  }

  // - TOF digitisation parameters
  if (setup->GetGeoTag(ECbmModuleId::kTof, geoTag)) {
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
  if (bUseMC) {
    inputSource->AddFriend(traColFile);
  }
  inputSource->AddFriend(recFile);

  FairRunAna* run = new FairRunAna();
  run->SetSource(inputSource);
  run->SetGenerateRunInfo(kFALSE);
  run->SetGeomFile(geoFile);

  FairRootFileSink* sink = new FairRootFileSink(sinkFile);
  run->SetSink(sink);

  TString monitorFile{sinkFile};
  monitorFile.ReplaceAll(".qa.root", ".qa.monitor.root");
  FairMonitor::GetMonitor()->EnableMonitor(kTRUE, monitorFile);
  // ------------------------------------------------------------------------

  // -----   Alignment matrices   -------------------------------------------
  if (bUseAlignment) {
    std::map<std::string, TGeoHMatrix>* matrices{nullptr};
    TString alignmentMatrixFileName = srcDir + "/parameters/mcbm/AlignmentMatrices_" + setupName + ".root";
    LOG(info) << "Trying to load alignment matrices from file " << alignmentMatrixFileName;
    TFile* misalignmentMatrixRootfile = TFile::Open(alignmentMatrixFileName, "READ");
    if (misalignmentMatrixRootfile && misalignmentMatrixRootfile->IsOpen()) {
      gDirectory->GetObject("MisalignMatrices", matrices);
      misalignmentMatrixRootfile->Close();
      LOG(info) << "Read alignment matrices ";
    }
    else {
      LOG(info) << "Could not find usable alignment file. Skip alignment matrices.";
    }

    if (matrices) run->AddAlignmentMatrices(*matrices);
  }
  // ------------------------------------------------------------------------

  // -----   QA manager   ---------------------------------------------------
  auto* qaManager = new CbmQaManager(verbose);
  qaManager->SetConfigName(qaConfig);
  if (benchmarkInput.Length()) {
    qaManager->OpenBenchmarkInput(benchmarkInput);
    qaManager->OpenBenchmarkOutput(benchmarkOut);
    qaManager->SetDefaultTag("default");
    qaManager->SetVersionTag("this");  // TODO: read git SHA
  }
  run->AddTask(qaManager);
  // ------------------------------------------------------------------------

  // -----   MCDataManager (legacy mode)  -----------------------------------
  if (bUseMC) {
    std::cout << "-I- " << myName << ": Adding MC manager and MC to reco matching tasks\n";

    auto* mcManager = new CbmMCDataManager("MCDataManager", 1);
    mcManager->AddFile(traColFile);
    qaManager->AddTask(mcManager);

    auto* matchRecoToMC = new CbmMatchRecoToMC();
    // NOTE: Matching is suppressed, if there are hit and cluster matches already in the tree. If there
    //       are no hit matches, they are produced on this stage.
    matchRecoToMC->SuppressHitReMatching();
    qaManager->AddTask(matchRecoToMC);
  }
  // ------------------------------------------------------------------------

  // ----- Tracking detector interface --------------------------------------
  qaManager->AddTask(new CbmTrackingDetectorInterfaceInit());
  // ------------------------------------------------------------------------

  // NOTE (FIXME) SZh, 26.04.2023
  // For some reason QA tasks interfere one with another. It leads to the
  // segmentational violation in CbmMuchHitFinderQa::DrawCanvases() function,
  // if this task is called after the CbmCaInputQaSts task. The problem dis-
  // appears, if the CbmMuchHitFinderQa task runs before the CbmCaInputQaSts
  // task.

  // ----- STS QA -----------------------------------------------------------
  if (bUseSts) {
    // CA Input QA
    qaManager->AddTask(new CbmCaInputQaSts(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------


  // ----- MUCH QA  ---------------------------------------------------------
  if (bUseMuch) {
    qaManager->AddTask(new CbmMuchTransportQa());
    qaManager->AddTask(new CbmMuchDigitizerQa());


    CbmMuchHitFinderQa* muchHitFinderQa = new CbmMuchHitFinderQa();
    muchHitFinderQa->SetGeoFileName(muchParFile);
    run->AddTask(muchHitFinderQa);

    // CA Input QA
    qaManager->AddTask(new CbmCaInputQaMuch(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- TRD QA -----------------------------------------------------------
  if (bUseTrd) {
    // CA Input QA
    qaManager->AddTask(new CbmCaInputQaTrd(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- TOF QA -----------------------------------------------------------
  if (bUseTof) {
    // CA Input QA
    qaManager->AddTask(new CbmCaInputQaTof(verbose, bUseMC));
  }
  // ------------------------------------------------------------------------

  // ----- CA tracking QA ---------------------------------------------------
  // Kalman Filter (currently needed to access the magnetic filed, to be
  // removed soon)
  qaManager->AddTask(new CbmKF());

  // Tracking parameters file is required
  TString caParFile = recFile;
  caParFile.ReplaceAll(".root", ".ca.par");

  auto* pCaInputQaSetup = new cbm::ca::InputQaSetup(verbose, bUseMC);
  pCaInputQaSetup->ReadParameters(caParFile.Data());
  pCaInputQaSetup->SetSetupName(setupName.Data());
  qaManager->AddTask(pCaInputQaSetup);

  auto* pCaOutputQa = new cbm::ca::OutputQa(verbose, bUseMC);
  pCaOutputQa->SetMcbmTrackingMode();
  pCaOutputQa->ReadParameters(caParFile.Data());
  pCaOutputQa->SetUseSts(bUseSts);
  pCaOutputQa->SetUseMuch(bUseMuch);
  pCaOutputQa->SetUseTrd(bUseTrd);
  pCaOutputQa->SetUseTof(bUseTof);
  pCaOutputQa->SetEventDisplay(true);
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
  std::cout << " QA checks " << (qaManager->GetStatus() ? "passed" : "failed") << std::endl;
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

/* clang-format off */
/// \brief QA macro execution function
/// \param nEvents        Number of events to proceed
/// \param dataset        Prefix of the output files files upstream the simulation/reconstruction chain
/// \param setupName      Name of the setup
/// \param bUseMC         Flag for MC (simulation) usage
/// \param config         QA YAML configuraiton file
/// \param benchmarkInput Path to a benchmark QA output, obtained for a given setup and given number of events
void mcbm_qa(Int_t nEvents, TString dataset, TString setupName, Bool_t bUseMC, TString config, TString benchmarkInput)
{
  TString rawFile  = dataset + ".event.raw.root";
  TString traFile  = dataset + ".tra.root";
  TString parFile  = dataset + ".par.root";
  TString geoFile  = dataset + ".geo.root";
  TString recFile  = dataset + ".rec.root";
  TString sinkFile = dataset + ".qa.root";
  mcbm_qa(nEvents, traFile, rawFile, recFile, parFile, geoFile, sinkFile, setupName, bUseMC, config, benchmarkInput);
}
/* clang-format on */
