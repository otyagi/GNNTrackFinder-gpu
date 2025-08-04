/* Copyright (C) 2024 Hulubei National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

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

/** @brief Macro for simulation & reconstruction QA
 ** @author Alex Bercuci <abercuci@niham.nipne.ro>
 ** @since  22 March 2024
 ** @param nEvents        Number of time-slices to process [-1 for all]
 ** @param recFile        Name of input reconstruction file (with extension .rec.root)
 ** @param setup          Name of predefined geometry setup
 ** @param bUseMC         Option to access also MC info if available and generate also pure QA plots 
 ** @param bUseAlignment  Option to access the alignment file used during the reconstruction. This is based on setup name and mCBM parameters repo.
 ** 
 ** This macro generats a list of 2D histograms which can be used to QA the reocnstruction chain 
 ** from the level of Detector local reconstruction to the level of primary vertex definition.
 ** The output can be found under the directory structure "RecoQA" in file with extension 
 ** .rqa.root. The histograms are organized as function of Detectors/Sensors/Projections. The user 
 ** is responsible for the moment to provide a grouping of these projections with a relevant 
 ** meaning and any further post-processing work.
 ** 
 **/
/* clang-format off */
void run_recoQa(Int_t nEvents = -1,
                TString recFile = "2391_node8_0_0000.rec.root",
                TString setupName = "mcbm_beam_2022_05_23_nickel",
                Bool_t bUseMC = kFALSE,
                Bool_t bUseAlignment = kTRUE)
/* clang-format on */
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("LOW");
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  int verbose    = 6;                              // verbose level
  TString myName = "recoQA";                       // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------

  // -----   In- and output file names   ------------------------------------
  TString dataset = recFile;
  dataset.ReplaceAll(".rec.root", "");
  TString traFile = dataset + ".tra.root";
  //TString rawFile  = dataset + ".event.raw.root";
  TString parFile  = dataset + ".par.root";
  TString sinkFile = dataset + ".rqa.root";
  TString geoFile  = setupName + ".geo.root";

  // ------------------------------------------------------------------------

  // -----   Load the geometry setup   -------------------------------------
  std::cout << '\n';

  CbmSetup* setup = CbmSetup::Instance();
  setup->LoadSetup(setupName);
  //setup->SetActive(ECbmModuleId::kSts, false);
  //setup->SetActive(ECbmModuleId::kTof, false);

  // In general, the following parts need not be touched
  // ========================================================================

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // ----    Debug option   -------------------------------------------------
  gDebug = 0;
  // ------------------------------------------------------------------------

  std::map<std::string, TGeoHMatrix>* matrices{nullptr};
  if (bUseAlignment) {
    TString alignmentMatrixFileName = srcDir + "/parameters/mcbm/AlignmentMatrices_" + setupName + ".root";
    LOG(info) << "Trying to load alignment matrices from file " << alignmentMatrixFileName;
    TFile* misalignmentMatrixRootfile = TFile::Open(alignmentMatrixFileName, "READ");
    if (misalignmentMatrixRootfile && misalignmentMatrixRootfile->IsOpen()) {
      gDirectory->GetObject("MisalignMatrices", matrices);
      misalignmentMatrixRootfile->Close();
      LOG(info) << "Read alignment matrices ";
    }
    else {
      LOG(warning) << "Could not find usable alignment file. Skip alignment matrices.";
    }
  }
  else {
    LOG(warning) << "Usage of alignment matrices disabled.";
  }

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
  monitorFile.ReplaceAll("rqa", "rqa.moni");
  FairMonitor::GetMonitor()->EnableMonitor(kTRUE, monitorFile);
  // ------------------------------------------------------------------------

  if (matrices) run->AddAlignmentMatrices(*matrices);

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
  auto pCa = new CbmL1("CA");
  // TODO: uncomment next line after !1862 will be merged !!!!!
  // pCa->SetInitMode(CbmL1::EInitMode::Param);
  pCa->SetMcbmMode();

  // User configuration example for CA:
  //pCa->SetConfigUser(srcDir + "/macro/L1/configs/ca_params_user_example.yaml");
  run->AddTask(pCa);

  // ----- Reco QA --------------------------------------------
  CbmRecoQaTask* recoQa = new CbmRecoQaTask();
  // recoQa->SetSetupClass(CbmRecoQaTask::kMcbm22);
  recoQa->SetSetupClass(CbmRecoQaTask::kMcbm24);
  // // example 1. filtering events and tracks (e.g. useful for preparing sampling for alignment)
  // recoQa->AddEventFilter(CbmRecoQaTask::EventFilter::eEventCut::kMultTrk)->SetFilter({1, 2});
  // recoQa->AddTrackFilter(CbmRecoQaTask::TrackFilter::eTrackCut::kSts)->SetFilter({3});
  // recoQa->AddTrackFilter(CbmRecoQaTask::TrackFilter::eTrackCut::kTrd)->SetFilter({2});
  // recoQa->AddTrackFilter(CbmRecoQaTask::TrackFilter::eTrackCut::kTof)->SetFilter({1});
  // // example 2. user access to the settings of view projections
  // CbmRecoQaTask::Detector* det(nullptr);
  // CbmRecoQaTask::View* view(nullptr);
  //   if ((det = recoQa->GetDetector(ECbmModuleId:kSts))) {
  //     view = det->GetView("U0L0M0");
  //     view->SetProjection();
  //   }
  // // example 3. user access to planes defining global track extrapolation
  //   double cos25 = TMath::Cos(TMath::DegToRad() * 25.);
  //   recoQa->SetProjections({15.1 * cos25, 0., -20 * cos25, -38 * cos25, -50.5 * cos25});
  run->AddTask(recoQa);
  run->SetGenerateRunInfo(kFALSE);

  // ------------------------------------------------------------------------
  //-----  Load Parameters --------------------------------------------------
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
