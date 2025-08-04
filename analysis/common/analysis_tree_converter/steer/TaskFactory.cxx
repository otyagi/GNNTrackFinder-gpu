/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file TaskFactory.cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 30.10.2023
 **/

#include "TaskFactory.h"

#include "CbmConverterManager.h"
#include "CbmFsdHitsConverter.h"
#include "CbmFsdModulesConverter.h"
#include "CbmKF.h"
#include "CbmL1.h"
#include "CbmMCDataManager.h"
#include "CbmMatchEvents.h"
#include "CbmMatchRecoToMC.h"
#include "CbmPsdModulesConverter.h"
#include "CbmRecEventHeaderConverter.h"
#include "CbmRichRingsConverter.h"
#include "CbmSetup.h"
#include "CbmSimEventHeaderConverter.h"
#include "CbmSimTracksConverter.h"
#include "CbmStsTracksConverter.h"
#include "CbmTofHitsConverter.h"
#include "CbmTrackingDetectorInterfaceInit.h"
#include "CbmTrdSetTracksPidLike.h"
#include "CbmTrdTracksConverter.h"

#include <TSystem.h>


namespace cbm::atconverter
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

    // --- Kalman Filter // Move after L1 task?
    auto kf = new CbmKF();
    fRun->AddTask(kf);

    // --- CA Track Finder
    auto ca = new CbmL1("CA Track Finder", 1, 3);
    fRun->AddTask(ca);
  }
  // --------------------------------------------------------------------------


  // -----   Converter Manager   ----------------------------------------------
  void TaskFactory::RegisterConverterManager(const TString& outputFile)
  {
    assert(fRun);

    // AnalysisTree converter
    auto* man = new CbmConverterManager();

    man->SetSystem(fRun->fConfig.f_glb_system);
    man->SetBeamMomentum(fRun->fConfig.f_glb_beamMom);
    man->SetTimeSliceLength(fRun->fConfig.f_glb_tslength);
    man->SetOutputName(outputFile.Data(), "rTree");

    if (fRun->fConfig.f_glb_mode == ECbmRecoMode::Timeslice) { man->AddTask(new CbmMatchEvents()); }

    // --- Converter for different branches
    man->AddTask(new CbmSimEventHeaderConverter("SimEventHeader"));
    man->AddTask(new CbmRecEventHeaderConverter("RecEventHeader"));
    man->AddTask(new CbmSimTracksConverter("SimParticles"));

    auto* ststracksconverter = new CbmStsTracksConverter("VtxTracks", "SimParticles");
    ststracksconverter->SetIsWriteKFInfo();
    ststracksconverter->SetIsReproduceCbmKFPF();
    man->AddTask(ststracksconverter);

    if (fRun->IsDataPresent(ECbmModuleId::kRich)) man->AddTask(new CbmRichRingsConverter("RichRings", "VtxTracks"));
    if (fRun->IsDataPresent(ECbmModuleId::kTof)) man->AddTask(new CbmTofHitsConverter("TofHits", "VtxTracks"));
    if (fRun->IsDataPresent(ECbmModuleId::kTrd)) man->AddTask(new CbmTrdTracksConverter("TrdTracks", "VtxTracks"));
    if (fRun->IsDataPresent(ECbmModuleId::kPsd)) man->AddTask(new CbmPsdModulesConverter("PsdModules"));

    if (fRun->IsDataPresent(ECbmModuleId::kFsd)) {
      man->AddTask(new CbmFsdModulesConverter("FsdModules"));

      CbmFsdHitsConverter* fsdHitsConverter = new CbmFsdHitsConverter("FsdHits", "VtxTracks");
      fsdHitsConverter->SetMinChi2GtrackHit(fRun->fConfig.f_fsd_minChi2match);
      fsdHitsConverter->SetMaxChi2GtrackHit(fRun->fConfig.f_fsd_maxChi2match);
      man->AddTask(fsdHitsConverter);
    }

    fRun->AddTask(man);
  }
  // --------------------------------------------------------------------------


  // -----   MC data manager   ------------------------------------------------
  void TaskFactory::RegisterMCDataManager(const std::vector<TString>& traFiles)
  {
    assert(fRun);
    CbmMCDataManager* mcManager = new CbmMCDataManager("MCDataManager", 0);
    for (auto traFile : traFiles) {
      mcManager->AddFile(traFile);
    }
    fRun->AddTask(mcManager);
  }
  // --------------------------------------------------------------------------


  // -----   STS track matching   ---------------------------------------------
  void TaskFactory::RegisterTrackMatching()
  {
    assert(fRun);

    auto* matchTask = new CbmMatchRecoToMC();
    fRun->AddTask(matchTask);
  }
  // --------------------------------------------------------------------------

  // -----   TID PID   --------------------------------------------------------
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

}  // namespace cbm::atconverter
