/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file TaskFactory.cxx
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 06.06.2023
 **/

#include "TaskFactory.h"

#include "CbmBuildEventsFromTracksReal.h"
#include "CbmBuildEventsIdeal.h"
#include "CbmFindPrimaryVertex.h"
#include "CbmFsdHitProducer.h"
#include "CbmKF.h"
#include "CbmL1.h"
#include "CbmL1StsTrackFinder.h"
#include "CbmLitFindGlobalTracks.h"
#include "CbmMuchFindHitsGem.h"
#include "CbmMuchGeoScheme.h"
#include "CbmMvdClusterfinder.h"
#include "CbmMvdHitfinder.h"
#include "CbmPVFinderKF.h"
#include "CbmPsdHitProducer.h"
#include "CbmRecoSts.h"
#include "CbmRecoT0.h"
#include "CbmRichHitProducer.h"
#include "CbmRichReconstruction.h"
#include "CbmSetup.h"
#include "CbmStsFindTracks.h"
#include "CbmStsFindTracksEvents.h"
#include "CbmTaskBuildRawEvents.h"
#include "CbmTofSimpClusterizer.h"
#include "CbmTrackingDetectorInterfaceInit.h"
#include "CbmTrdClusterFinder.h"
#include "CbmTrdHitProducer.h"
#include "CbmTrdSetTracksPidLike.h"

#include <TSystem.h>


namespace cbm::reco::offline
{


  // -----   Constructor   ----------------------------------------------------
  TaskFactory::TaskFactory(Run* run) : fRun(run) {}
  // --------------------------------------------------------------------------


  // -----   CA tracking   ----------------------------------------------------
  void TaskFactory::RegisterCaTracking()
  {
    assert(fRun);

    // --- Tracking geometry interface
    auto interface = new CbmTrackingDetectorInterfaceInit();
    fRun->AddTask(interface);

    // --- Kalman Filter
    auto kf = new CbmKF();
    fRun->AddTask(kf);

    // --- CA Track Finder
    auto ca = new CbmL1("CA Track Finder");
    fRun->AddTask(ca);

    // --- Track finder steering class
    auto trackFinder = new CbmL1StsTrackFinder();

    // --- Track finder task
    FairTask* findTracks = nullptr;
    if (fRun->GetConfig().f_glb_mode == ECbmRecoMode::Timeslice)
      findTracks = new CbmStsFindTracks(0, trackFinder, true);
    else
      findTracks = new CbmStsFindTracksEvents(trackFinder, true);
    fRun->AddTask(findTracks);
  }
  // --------------------------------------------------------------------------


  // -----   Event building from digis   --------------------------------------
  void TaskFactory::RegisterDigiEventBuilder()
  {
    assert(fRun);
    if (fRun->GetConfig().f_evbuild_type == cbm::reco::offline::ECbmEvbuildType::Undefined)
      throw std::out_of_range("Undefined event builder type");

    // --- Ideal digi event builder (using MC truth information)
    if (fRun->GetConfig().f_evbuild_type == cbm::reco::offline::ECbmEvbuildType::Ideal) {
      FairTask* evBuildRaw = new CbmBuildEventsIdeal();
      fRun->AddTask(evBuildRaw);
    }  //? Ideal event builder

    // --- Real digi event builder
    // TODO: This should be replaced by the newer algorithm algo/evbuild/EventBuilder
    else {

      const cbm::reco::offline::Config& config = fRun->GetConfig();
      ECbmModuleId triggerDetector             = config.f_evbuild_trigDet;

      if (!fRun->IsDataPresent(triggerDetector)) throw std::runtime_error("No input data for trigger detector");

      // --- Task
      CbmTaskBuildRawEvents* evBuildRaw = new CbmTaskBuildRawEvents();

      // --- Mode
      evBuildRaw->SetEventOverlapMode(config.f_evbuild_ovlapmode);

      // --- Reference detector
      switch (triggerDetector) {
        case ECbmModuleId::kSts: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetSts); break;
        case ECbmModuleId::kMuch: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetMuch); break;
        case ECbmModuleId::kTrd: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetTrd); break;
        case ECbmModuleId::kTrd2d: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetTrd2D); break;
        case ECbmModuleId::kTof: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetTof); break;
        case ECbmModuleId::kRich: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetRich); break;
        case ECbmModuleId::kPsd: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetPsd); break;
        case ECbmModuleId::kFsd: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetFsd); break;
        case ECbmModuleId::kBmon: evBuildRaw->SetReferenceDetector(kRawEventBuilderDetBmon); break;
        default: throw std::out_of_range("Event builder: Undefined trigger detector"); break;
      }

      // --- Make BMON (default reference detector) a selected detector (with default parameters)
      if (triggerDetector != ECbmModuleId::kBmon) evBuildRaw->AddDetector(kRawEventBuilderDetBmon);

      // --- Remove detectors of which there are no input data
      if (!fRun->IsDataPresent(ECbmModuleId::kRich)) evBuildRaw->RemoveDetector(kRawEventBuilderDetRich);
      if (!fRun->IsDataPresent(ECbmModuleId::kMuch)) evBuildRaw->RemoveDetector(kRawEventBuilderDetMuch);
      if (!fRun->IsDataPresent(ECbmModuleId::kPsd)) evBuildRaw->RemoveDetector(kRawEventBuilderDetPsd);
      if (!fRun->IsDataPresent(ECbmModuleId::kTof)) evBuildRaw->RemoveDetector(kRawEventBuilderDetTof);
      if (!fRun->IsDataPresent(ECbmModuleId::kTrd)) evBuildRaw->RemoveDetector(kRawEventBuilderDetTrd);
      if (!fRun->IsDataPresent(ECbmModuleId::kFsd)) evBuildRaw->RemoveDetector(kRawEventBuilderDetFsd);

      // --- Timeslice parameters
      evBuildRaw->SetTsParameters(0.0, 1.e7, 0.0);

      // --- Use CbmMuchDigi instead of CbmMuchBeamtimeDigi
      // TODO: Seems legacy.
      evBuildRaw->ChangeMuchBeamtimeDigiFlag(kFALSE);

      // --- Set trigger parameters
      evBuildRaw->SetTriggerMinNumber(triggerDetector, config.f_evbuild_trigNumMin);
      evBuildRaw->SetTriggerMaxNumber(triggerDetector, config.f_evbuild_trigNumMax);
      evBuildRaw->SetTriggerWindow(triggerDetector, config.f_evbuild_trigWinMin, config.f_evbuild_trigWinMax);

      fRun->AddTask(evBuildRaw);

    }  //? Real event builder
  }
  // --------------------------------------------------------------------------


  // -----   LIT Global Tracking   --------------------------------------------
  void TaskFactory::RegisterGlobalTracking()
  {
    assert(fRun);
    CbmLitFindGlobalTracks* finder = new CbmLitFindGlobalTracks();
    finder->SetTrackingType(fRun->GetConfig().f_lit_trackType);
    finder->SetMergerType(fRun->GetConfig().f_lit_mergeType);
    fRun->AddTask(finder);
  }
  // --------------------------------------------------------------------------


  // -----   MUCH reconstruction   --------------------------------------------
  void TaskFactory::RegisterMuchReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kMuch)) return;

    // --- Parameter file name
    TString geoTag;
    if (CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kMuch, geoTag)) {
      Int_t muchFlag  = (geoTag.Contains("mcbm") ? 1 : 0);
      TString parFile = gSystem->Getenv("VMCWORKDIR");
      parFile += "/parameters/much/much_" + geoTag(0, 4) + "_digi_sector.root";

      auto muchGeoScheme = CbmMuchGeoScheme::Instance();
      if (!muchGeoScheme->IsInitialized()) {
        muchGeoScheme->Init(parFile, muchFlag);
      }

      FairTask* muchReco = new CbmMuchFindHitsGem(parFile.Data(), muchFlag);
      fRun->AddTask(muchReco);
    }
  }
  // --------------------------------------------------------------------------


  // -----   MVD reconstruction   ---------------------------------------------
  void TaskFactory::RegisterMvdReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kMvd)) return;

    // The real eventbuilder currently doesn't support the MVD detector
    if (fRun->GetConfig().f_evbuild_type == cbm::reco::offline::ECbmEvbuildType::Real) {
      return;
    }

    CbmMvdClusterfinder* mvdCluster = new CbmMvdClusterfinder("MVD Cluster Finder", 0, 0);
    if (fRun->GetConfig().f_glb_mode == ECbmRecoMode::EventByEvent) {
      mvdCluster->SetMode(ECbmRecoMode::EventByEvent);
    }
    else {
      mvdCluster->SetMode(ECbmRecoMode::Timeslice);
    }
    fRun->AddTask(mvdCluster);

    CbmMvdHitfinder* mvdHit = new CbmMvdHitfinder("MVD Hit Finder", 0, 0);
    mvdHit->UseClusterfinder(kTRUE);
    if (fRun->GetConfig().f_glb_mode == ECbmRecoMode::EventByEvent) {
      mvdHit->SetMode(ECbmRecoMode::EventByEvent);
    }
    else {
      mvdHit->SetMode(ECbmRecoMode::Timeslice);
    }
    fRun->AddTask(mvdHit);
  }
  // --------------------------------------------------------------------------


  // -----   Primary Vertex Finding   -----------------------------------------
  void TaskFactory::RegisterPvFinder()
  {
    assert(fRun);
    CbmPrimaryVertexFinder* pvFinder = new CbmPVFinderKF();
    CbmFindPrimaryVertex* findVertex = new CbmFindPrimaryVertex(pvFinder);
    fRun->AddTask(findVertex);
  }
  // --------------------------------------------------------------------------


  // -----   RICH hit finding   -----------------------------------------------
  void TaskFactory::RegisterRichHitFinder()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kRich)) return;

    CbmRichHitProducer* richHitProd = new CbmRichHitProducer();
    fRun->AddTask(richHitProd);
  }
  // --------------------------------------------------------------------------


  // -----   RICH reconstruction   --------------------------------------------
  void TaskFactory::RegisterRichReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kRich)) return;

    CbmRichReconstruction* richReco = new CbmRichReconstruction();
    fRun->AddTask(richReco);
  }
  // --------------------------------------------------------------------------


  // -----   STS reconstruction   ---------------------------------------------
  void TaskFactory::RegisterStsReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kSts)) return;

    CbmRecoSts* stsReco = new CbmRecoSts(fRun->GetConfig().f_glb_mode);
    stsReco->SetUseGpuReco(fRun->GetConfig().f_sts_usegpu);
    fRun->AddTask(stsReco);
  }
  // --------------------------------------------------------------------------


  // -----   TOF reconstruction   ---------------------------------------------
  void TaskFactory::RegisterTofReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kTof)) return;

    CbmTofSimpClusterizer* tofCluster = new CbmTofSimpClusterizer("TOF Simple Clusterizer", 0);
    tofCluster->SetOutputBranchPersistent("TofHit", kTRUE);
    tofCluster->SetOutputBranchPersistent("TofDigiMatch", kTRUE);
    fRun->AddTask(tofCluster);
  }
  // --------------------------------------------------------------------------


  // -----   PSD reconstruction   ---------------------------------------------
  void TaskFactory::RegisterPsdReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kPsd)) return;

    CbmPsdHitProducer* psdHit = new CbmPsdHitProducer();
    fRun->AddTask(psdHit);
  }
  // --------------------------------------------------------------------------


  // -----   FSD reconstruction   ---------------------------------------------
  void TaskFactory::RegisterFsdReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kFsd)) return;

    CbmFsdHitProducer* fsdHit = new CbmFsdHitProducer();
    fRun->AddTask(fsdHit);
  }
  // --------------------------------------------------------------------------


  // -----   Event building from tracks   -------------------------------------
  void TaskFactory::RegisterTrackEventBuilder()
  {
    assert(fRun);
    auto eventBuilder = new CbmBuildEventsFromTracksReal();
    fRun->AddTask(eventBuilder);
  }
  // --------------------------------------------------------------------------


  // -----   TRD PID   --------------------------------------------------------
  void TaskFactory::RegisterTrdPid()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kTrd)) return;

    CbmTrdSetTracksPidLike* trdPid = new CbmTrdSetTracksPidLike("TRDLikelihood", "TRDLikelihood");
    trdPid->SetUseMCInfo(kTRUE);
    trdPid->SetUseMomDependence(kTRUE);
    fRun->AddTask(trdPid);
  }
  // --------------------------------------------------------------------------


  // -----   TRD reconstruction   ---------------------------------------------
  void TaskFactory::RegisterTrdReco()
  {
    assert(fRun);
    if (!fRun->IsDataPresent(ECbmModuleId::kTrd)) return;

    CbmTrdClusterFinder* trdCluster = new CbmTrdClusterFinder();
    if (fRun->GetConfig().f_glb_mode == ECbmRecoMode::EventByEvent)
      trdCluster->SetTimeBased(kFALSE);
    else
      trdCluster->SetTimeBased(kTRUE);
    trdCluster->SetNeighbourEnable(true, false);
    trdCluster->SetMinimumChargeTH(fRun->GetConfig().f_trd_trigThresh);
    trdCluster->SetRowMerger(true);
    fRun->AddTask(trdCluster);

    CbmTrdHitProducer* trdHit = new CbmTrdHitProducer();
    fRun->AddTask(trdHit);
  }
  // --------------------------------------------------------------------------


  // -----   Bmon reconstruction   ----------------------------------------------
  void TaskFactory::RegisterBmonReco()
  {
    assert(fRun);
    CbmRecoT0* recoBmon = new CbmRecoT0();
    fRun->AddTask(recoBmon);
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::reco::offline
