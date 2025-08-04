/* Copyright (C) 2009-2023 UGiessen, JINR-LIT
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Andrey Lebedev, Martin Beyer */

#if !defined(__CLING__) || defined(__ROOTCLING__)
#include "CbmBuildEventsFromTracksReal.h"
#include "CbmBuildEventsIdeal.h"
#include "CbmBuildEventsQa.h"
#include "CbmFindPrimaryVertex.h"
#include "CbmKF.h"
#include "CbmL1.h"
#include "CbmL1StsTrackFinder.h"
#include "CbmLitFindGlobalTracks.h"
#include "CbmMCDataManager.h"
#include "CbmMatchRecoToMC.h"
#include "CbmMvdClusterfinder.h"
#include "CbmMvdHitfinder.h"
#include "CbmPVFinderKF.h"
#include "CbmPrimaryVertexFinder.h"
#include "CbmRecoSts.h"
#include "CbmRecoT0.h"
#include "CbmRichHitProducer.h"
#include "CbmRichReconstruction.h"
#include "CbmSetup.h"
#include "CbmStsFindTracks.h"
#include "CbmStsFindTracksEvents.h"
#include "CbmStsTrackFinder.h"
#include "CbmTaskBuildRawEvents.h"
#include "CbmTofSimpClusterizer.h"
#include "CbmTrackingDetectorInterfaceInit.h"
#include "CbmTrdClusterFinder.h"
#include "CbmTrdHitProducer.h"
#include "CbmTrdSetTracksPidLike.h"

#include <FairFileSource.h>
#include <FairLogger.h>
#include <FairMonitor.h>
#include <FairParAsciiFileIo.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>

#include <TGeoManager.h>
#include <TObjString.h>
#include <TROOT.h>
#include <TStopwatch.h>
#include <TTree.h>

#include <iostream>
#endif

void run_reco(TString traFile = "", TString parFile = "", TString digiFile = "", TString recoFile = "",  // i/o files
              Int_t nofTimeSlices = -1, TString geoSetup = "sis100_electron", TString sEvBuildRaw = "Ideal",
              Bool_t useMC = true, Bool_t monitor = true)
{
  TTree::SetMaxTreeSize(90000000000);

  // -----   Files   --------------------------------------------------------
  TString macroPath = __FILE__;
  TString macroDir  = macroPath(0, macroPath.Last('/') + 1);
  if (traFile.IsNull()) traFile = macroDir + "data/test.tra.root";
  if (parFile.IsNull()) parFile = macroDir + "data/test.par.root";
  if (digiFile.IsNull()) digiFile = macroDir + "data/test.digi.root";
  if (recoFile.IsNull()) recoFile = macroDir + "data/test.reco.root";

  remove(recoFile);
  // ------------------------------------------------------------------------

  // --- Logger settings ----------------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("LOW");
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  TString myName = "run_reco";
  TString srcDir = gSystem->Getenv("VMCWORKDIR");
  // ------------------------------------------------------------------------

  // -----   Load the geometry setup   --------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Loading setup " << geoSetup << std::endl;
  CbmSetup* geo = CbmSetup::Instance();
  geo->LoadSetup(geoSetup);
  // ------------------------------------------------------------------------

  // -----   Some global switches   -----------------------------------------
  Bool_t eventBased = (sEvBuildRaw != "");
  Bool_t useMvd     = geo->IsActive(ECbmModuleId::kMvd);
  Bool_t useSts     = geo->IsActive(ECbmModuleId::kSts);
  Bool_t useRich    = geo->IsActive(ECbmModuleId::kRich);
  Bool_t useMuch    = geo->IsActive(ECbmModuleId::kMuch);
  Bool_t useTrd     = geo->IsActive(ECbmModuleId::kTrd);
  Bool_t useTof     = geo->IsActive(ECbmModuleId::kTof);
  Bool_t usePsd     = geo->IsActive(ECbmModuleId::kPsd);
  // Bool_t useFsd     = geo->IsActive(ECbmModuleId::kFsd);
  // ------------------------------------------------------------------------

  // -----   Parameter files as input to the runtime database   -------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Defining parameter files " << std::endl;
  TList* parFileList = new TList();
  TString geoTag;

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

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // -----   FairRunAna   ---------------------------------------------------
  FairFileSource* inputSource = new FairFileSource(digiFile);
  if (useMC) inputSource->AddFriend(traFile);

  FairRunAna* run = new FairRunAna();
  run->SetSource(inputSource);
  run->SetGenerateRunInfo(kFALSE);

  FairRootFileSink* sink = new FairRootFileSink(recoFile);
  run->SetSink(sink);

  FairMonitor::GetMonitor()->EnableMonitor(monitor);
  // ------------------------------------------------------------------------

  // -----   MCDataManager  -------------------------------------------------
  if (useMC) {
    CbmMCDataManager* mcManager = new CbmMCDataManager("MCDataManager", 0);
    mcManager->AddFile(traFile);
    run->AddTask(mcManager);
    std::cout << "-I- " << myName << ": Added task " << mcManager->GetName() << std::endl;
  }
  // ------------------------------------------------------------------------

  // -----   Raw event building from digis   --------------------------------
  if (eventBased) {
    if (sEvBuildRaw.EqualTo("Ideal", TString::ECaseCompare::kIgnoreCase)) {
      FairTask* evBuildRaw = new CbmBuildEventsIdeal();
      run->AddTask(evBuildRaw);
      std::cout << "-I- " << myName << ": Added task " << evBuildRaw->GetName() << std::endl;
    }  //? Ideal raw event building
    else if (sEvBuildRaw.EqualTo("Real", TString::ECaseCompare::kIgnoreCase)) {
      CbmTaskBuildRawEvents* evBuildRaw = new CbmTaskBuildRawEvents();

      //Choose between NoOverlap, MergeOverlap, AllowOverlap
      evBuildRaw->SetEventOverlapMode(EOverlapModeRaw::AllowOverlap);

      // Remove detectors where digis not found
      if (!useRich) evBuildRaw->RemoveDetector(kRawEventBuilderDetRich);
      if (!useMuch) evBuildRaw->RemoveDetector(kRawEventBuilderDetMuch);
      if (!usePsd) evBuildRaw->RemoveDetector(kRawEventBuilderDetPsd);
      // if (!useFsd) evBuildRaw->RemoveDetector(kRawEventBuilderDetFsd);
      if (!useTof) evBuildRaw->RemoveDetector(kRawEventBuilderDetTof);
      if (!useTrd) evBuildRaw->RemoveDetector(kRawEventBuilderDetTrd);
      if (!useSts) {
        std::cerr << "-E- " << myName << ": Sts must be present for raw event "
                  << "building using ``Real2019'' option. Terminating macro." << std::endl;
        return;
      }
      // Set STS as reference detector
      evBuildRaw->SetReferenceDetector(kRawEventBuilderDetSts);
      // Make Bmon (previous reference detector) a selected detector (with default parameters)
      evBuildRaw->AddDetector(kRawEventBuilderDetBmon);

      // Use sliding window seed builder with STS
      //evBuildRaw->SetReferenceDetector(kRawEventBuilderDetUndef);
      //evBuildRaw->AddSeedTimeFillerToList(kRawEventBuilderDetSts);
      //evBuildRaw->SetSlidingWindowSeedFinder(1000, 500, 500);
      //evBuildRaw->SetSeedFinderQa(true);  // optional QA information for seed finder

      evBuildRaw->SetTsParameters(0.0, 1.e7, 0.0);

      // Use CbmMuchDigi instead of CbmMuchBeamtimeDigi
      evBuildRaw->ChangeMuchBeamtimeDigiFlag(kFALSE);

      evBuildRaw->SetTriggerMinNumber(ECbmModuleId::kSts, 1000);
      evBuildRaw->SetTriggerMaxNumber(ECbmModuleId::kSts, -1);
      evBuildRaw->SetTriggerWindow(ECbmModuleId::kSts, -500, 500);

      run->AddTask(evBuildRaw);
      std::cout << "-I- " << myName << ": Added task " << evBuildRaw->GetName() << std::endl;
    }  //? Real raw event building
    else {
      std::cerr << "-E- " << myName << ": Unknown option " << sEvBuildRaw
                << " for raw event building! Terminating macro execution." << std::endl;
      return;
    }
  }  //? event-based reco
  // ------------------------------------------------------------------------

  // -----   QA for raw event builder   -------------------------------------
  // if (eventBased && useMC) {
  //   CbmBuildEventsQa* evBuildQA = new CbmBuildEventsQa();
  //   run->AddTask(evBuildQA);
  //   std::cout << "-I- " << myName << ": Added task " << evBuildQA->GetName() << std::endl;
  // }
  // ------------------------------------------------------------------------

  // -----   Local reconstruction in MVD   ----------------------------------
  if (useMvd) {
    CbmMvdClusterfinder* mvdCluster = new CbmMvdClusterfinder("MVD Cluster Finder", 0, 0);
    run->AddTask(mvdCluster);
    std::cout << "-I- " << myName << ": Added task " << mvdCluster->GetName() << std::endl;

    CbmMvdHitfinder* mvdHit = new CbmMvdHitfinder("MVD Hit Finder", 0, 0);
    mvdHit->UseClusterfinder(kTRUE);
    run->AddTask(mvdHit);
    std::cout << "-I- " << myName << ": Added task " << mvdHit->GetName() << std::endl;
  }
  // ------------------------------------------------------------------------

  // -----   Local reconstruction in STS   ----------------------------------
  if (useSts) {
    CbmRecoSts* stsReco = new CbmRecoSts(ECbmRecoMode::Timeslice);
    if (eventBased) stsReco->SetMode(ECbmRecoMode::EventByEvent);
    run->AddTask(stsReco);
    std::cout << "-I- " << myName << ": Added task " << stsReco->GetName() << std::endl;
  }
  // ------------------------------------------------------------------------

  // -----   Local reconstruction in RICH   ---------------------------------
  if (useRich) {
    CbmRichHitProducer* richHitProd = new CbmRichHitProducer();
    run->AddTask(richHitProd);
    std::cout << "-I- " << myName << ": Added task " << richHitProd->GetName() << std::endl;
  }
  // ------------------------------------------------------------------------

  // -----   Local reconstruction in TRD   ----------------------------------
  if (useTrd) {

    Double_t triggerThreshold       = 0.5e-6;  // SIS100
    CbmTrdClusterFinder* trdCluster = new CbmTrdClusterFinder();
    if (eventBased) {
      trdCluster->SetTimeBased(kFALSE);
    }
    else {
      trdCluster->SetTimeBased(kTRUE);
    }
    trdCluster->SetNeighbourEnable(true, false);
    trdCluster->SetMinimumChargeTH(triggerThreshold);
    trdCluster->SetRowMerger(true);

    // Uncomment if you want to use all available digis.
    // In that case clusters hits will not be added to the CbmEvent
    // trdCluster->SetUseOnlyEventDigis(kFALSE);

    run->AddTask(trdCluster);
    std::cout << "-I- " << myName << ": Added task " << trdCluster->GetName() << std::endl;

    CbmTrdHitProducer* trdHit = new CbmTrdHitProducer();
    run->AddTask(trdHit);
    std::cout << "-I- " << myName << ": Added task " << trdHit->GetName() << std::endl;
  }
  // ------------------------------------------------------------------------

  // -----   Local reconstruction in TOF   ----------------------------------
  if (useTof) {
    CbmTofSimpClusterizer* tofCluster = new CbmTofSimpClusterizer("TofSimpClusterizer", 0);
    tofCluster->SetOutputBranchPersistent("TofHit", kTRUE);
    tofCluster->SetOutputBranchPersistent("TofDigiMatch", kTRUE);
    run->AddTask(tofCluster);
    std::cout << "-I- " << myName << ": Added task " << tofCluster->GetName() << std::endl;
  }
  // ------------------------------------------------------------------------

  // TODO: add Fsd

  // -----   Track finding in STS (+ MVD)    --------------------------------
  if (useMvd || useSts) {
    auto pDetIF = new CbmTrackingDetectorInterfaceInit();
    run->AddTask(pDetIF);  // Geometry interface initializer for tracker
    std::cout << "-I- " << myName << ": Added task " << pDetIF->GetName() << std::endl;

    // Kalman filter
    auto kalman = new CbmKF();
    run->AddTask(kalman);
    std::cout << "-I- " << myName << ": Added task " << kalman->GetName() << std::endl;

    // L1 tracking
    auto l1 = new CbmL1("CA");
    // auto l1 = (useMC) ? new CbmL1("CA", 2, 3) : new CbmL1("CA");

    // L1 configuration file (optional)
    // At the moment, the YAML configuration file defines different parameters for a sequence of track finder
    // iterations. The same file should be used in ca::tools::WindowFinder class for hit search window estimation
    //l1->SetInputConfigName(TString(gSystem->Getenv("VMCWORKDIR")) + "/reco/L1/L1Algo/L1ConfigExample.yaml");

    run->AddTask(l1);
    std::cout << "-I- " << myName << ": Added task " << l1->GetName() << std::endl;

    CbmStsTrackFinder* stsTrackFinder = new CbmL1StsTrackFinder();
    if (eventBased) {
      FairTask* stsFindTracks = new CbmStsFindTracksEvents(stsTrackFinder, useMvd);
      run->AddTask(stsFindTracks);
      std::cout << "-I- " << myName << ": Added task " << stsFindTracks->GetName() << std::endl;
    }
    else {
      FairTask* stsFindTracks = new CbmStsFindTracks(0, stsTrackFinder, useMvd);
      run->AddTask(stsFindTracks);
      std::cout << "-I- " << myName << ": Added task " << stsFindTracks->GetName() << std::endl;
    }
  }
  // ----------------------------------------------------------------------

  if (eventBased) {
    // -----   Primary vertex finding   -------------------------------------
    CbmPrimaryVertexFinder* pvFinder = new CbmPVFinderKF();
    CbmFindPrimaryVertex* findVertex = new CbmFindPrimaryVertex(pvFinder);
    run->AddTask(findVertex);
    std::cout << "-I- " << myName << ": Added task " << findVertex->GetName() << std::endl;
    // ----------------------------------------------------------------------

    // ---   Global track finding   -----------------------------------------
    CbmLitFindGlobalTracks* finder = new CbmLitFindGlobalTracks();
    finder->SetTrackingType("branch");
    finder->SetMergerType("nearest_hit");
    run->AddTask(finder);
    std::cout << "-I- " << myName << ": Added task " << finder->GetName() << std::endl;
    // ----------------------------------------------------------------------

    // ---   Particle Id in TRD   -------------------------------------------
    if (useTrd) {
      CbmTrdSetTracksPidLike* trdLI = new CbmTrdSetTracksPidLike("TRDLikelihood", "TRDLikelihood");
      trdLI->SetUseMCInfo(kTRUE);
      trdLI->SetUseMomDependence(kTRUE);
      run->AddTask(trdLI);
      std::cout << "-I- " << myName << ": Added task " << trdLI->GetName() << std::endl;
    }
    // ----------------------------------------------------------------------

    // -----   RICH reconstruction   ----------------------------------------
    if (useRich) {
      CbmRichReconstruction* richReco = new CbmRichReconstruction();
      run->AddTask(richReco);
      std::cout << "-I- " << myName << ": Added task " << richReco->GetName() << std::endl;
    }
    // ----------------------------------------------------------------------

    // ----- Bmon reconstruction   --------------------------------------------
    CbmRecoT0* recoBmon = new CbmRecoT0();
    run->AddTask(recoBmon);
    std::cout << "-I- " << myName << ": Added task " << recoBmon->GetName() << std::endl;
    // ----------------------------------------------------------------------

    // -----   Match reco to MC   -------------------------------------------
    if (useMC) {
      CbmMatchRecoToMC* match = new CbmMatchRecoToMC();
      std::cout << "-I- " << myName << ": Added task " << match->GetName() << std::endl;
      run->AddTask(match);
    }
    // ----------------------------------------------------------------------

  }  //? event-based reco
  else {
    if (useRich) {  // Place it here to test rich ring reco on timeslices
      CbmRichReconstruction* richReco = new CbmRichReconstruction();
      richReco->SetRunExtrapolation(kFALSE);
      richReco->SetRunProjection(kFALSE);
      richReco->SetRunTrackAssign(kFALSE);
      run->AddTask(richReco);
      std::cout << "-I- " << myName << ": Added task " << richReco->GetName() << std::endl;
    }
    // -----   Event building from STS tracks   -----------------------------
    run->AddTask(new CbmBuildEventsFromTracksReal());
    // ----------------------------------------------------------------------
    //FIXME: if placing rich reco here:
    //FIXME: hits are found in events, but no rings are reconstructed
  }  //? time-based reco

  // -----  Parameter database   --------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- "
            << ": Set runtime DB" << std::endl;
  FairRuntimeDb* rtdb        = run->GetRuntimeDb();
  FairParRootFileIo* parIo1  = new FairParRootFileIo();
  FairParAsciiFileIo* parIo2 = new FairParAsciiFileIo();
  parIo1->open(parFile.Data(), "UPDATE");
  rtdb->setFirstInput(parIo1);
  if (!parFileList->IsEmpty()) {
    parIo2->open(parFileList, "in");
    rtdb->setSecondInput(parIo2);
  }
  // ------------------------------------------------------------------------

  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- "
            << ": Initialise run" << std::endl;
  run->Init();
  rtdb->setOutput(parIo1);
  rtdb->saveOutput();
  rtdb->print();
  // ------------------------------------------------------------------------

  // -----   Register light ions (d, t, He3, He4)   -------------------------
  std::cout << std::endl;
  TString registerLightIonsMacro = gSystem->Getenv("VMCWORKDIR");
  registerLightIonsMacro += "/macro/KF/registerLightIons.C";
  std::cout << "Loading macro " << registerLightIonsMacro << std::endl;
  gROOT->LoadMacro(registerLightIonsMacro);
  gROOT->ProcessLine("registerLightIons()");
  // ------------------------------------------------------------------------

  // -----   Start run   ----------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- "
            << ": Starting run" << std::endl;
  run->Run(0, nofTimeSlices);
  // ------------------------------------------------------------------------

  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  std::cout << std::endl;
  FairMonitor::GetMonitor()->Print();
  std::cout << std::endl;
  std::cout << "Macro finished succesfully." << std::endl;
  std::cout << "Output file is " << recoFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << timer.RealTime() << " s, CPU time " << timer.CpuTime() << " s" << std::endl;
  if (gROOT->GetVersionInt() >= 60602) {
    gGeoManager->GetListOfVolumes()->Delete();
    gGeoManager->GetListOfShapes()->Delete();
    delete gGeoManager;
  }
  std::cout << std::endl << "Test passed" << std::endl << "All ok" << std::endl;
  // ------------------------------------------------------------------------
}
