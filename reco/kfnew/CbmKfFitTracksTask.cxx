/* Copyright (C) 2023-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: S.Gorbunov[committer] */

/// @file    CbmKfFitTracksTask.cxx
/// @author  Sergey Gorbunov
/// @date    15.11.2023
/// @brief   Task class for refitting global or sts tracks


#include "CbmKfFitTracksTask.h"

#include "CbmGlobalTrack.h"
#include "CbmKfUtil.h"
#include "CbmMuchTrack.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdHit.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmStsHit.h"
#include "CbmStsSetup.h"
#include "CbmStsTrack.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofTrack.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdTrack.h"
#include "CbmTrdTrackingInterface.h"
#include "FairRootManager.h"
#include "TClonesArray.h"

#include <cmath>
#include <iostream>


// ClassImp(CbmKfFitTracksTask);

namespace
{
  using namespace cbm::algo;
}


CbmKfFitTracksTask::CbmKfFitTracksTask(FitMode mode, Int_t iVerbose)
  : FairTask("CbmKfFitTracksTask", iVerbose)
  , fFitMode(mode)
{
}

CbmKfFitTracksTask::~CbmKfFitTracksTask() {}


InitStatus CbmKfFitTracksTask::Init()
{

  fFitter.Init();

  //Get ROOT Manager
  FairRootManager* ioman = FairRootManager::Instance();

  if (!ioman) {
    LOG(error) << "CbmKfFitTracksTask::Init :: RootManager not instantiated!";
    return kERROR;
  }

  // Get global tracks

  fGlobalTracks = dynamic_cast<TClonesArray*>(ioman->GetObject("GlobalTrack"));

  // Get detector tracks
  fStsTracks  = dynamic_cast<TClonesArray*>(ioman->GetObject("StsTrack"));
  fMuchTracks = dynamic_cast<TClonesArray*>(ioman->GetObject("MuchTrack"));
  fTrdTracks  = dynamic_cast<TClonesArray*>(ioman->GetObject("TrdTrack"));
  fTofTracks  = dynamic_cast<TClonesArray*>(ioman->GetObject("TofTrack"));

  if (FitMode::kSts != fFitMode && !fGlobalTracks) {
    LOG(error) << "CbmKfFitTracksTask::Init: Global track array not found!";
    return kERROR;
  }

  if (FitMode::kSts == fFitMode && !fStsTracks) {
    LOG(error) << "CbmKfFitTracksTask::Init: Sts track array not found!";
    return kERROR;
  }

  return kSUCCESS;
}

void CbmKfFitTracksTask::Exec(Option_t* /*opt*/)
{

  LOG(info) << "CbmKfFitTracksTask: exec event N " << fNeventsProcessed++;

  // select tracks for alignment and store them
  if (FitMode::kSts == fFitMode && fStsTracks) {

    for (int iTr = 0; iTr < fStsTracks->GetEntriesFast(); iTr++) {
      CbmStsTrack* stsTrack = dynamic_cast<CbmStsTrack*>(fStsTracks->At(iTr));
      if (!stsTrack) {
        LOG(fatal) << "CbmKfFitTracksTask: null pointer to the sts track!";
        return;
      }
      CbmKfTrackFitter::Trajectory t;
      if (!fFitter.CreateMvdStsTrack(t, iTr)) {
        LOG(fatal) << "CbmKfFitTracksTask: can not create the sts track for the fit! ";
        return;
      }
      fFitter.FitTrajectory(t);
      {
        const auto& parV = t.fNodes.front().fParamUp;
        cbm::algo::kf::TrackParamD parD;
        parD.Set(parV, 0);
        FairTrackParam trackFirst = cbm::kf::ConvertTrackParam(parD);
        stsTrack->SetParamFirst(&trackFirst);
      }
      {
        const auto& parV = t.fNodes.back().fParamDn;
        cbm::algo::kf::TrackParamD parD;
        parD.Set(parV, 0);
        FairTrackParam trackLast = cbm::kf::ConvertTrackParam(parD);
        stsTrack->SetParamLast(&trackLast);
      }
    }
  }

  if (FitMode::kMcbm == fFitMode && fGlobalTracks) {

    fFitter.SetDefaultMomentumForMs(0.1);  // 0.1 GeV/c
    fFitter.FixMomentumForMs(true);        // fix the momentum for the Multiple Scattering calculation

    for (int iTr = 0; iTr < fGlobalTracks->GetEntriesFast(); iTr++) {
      CbmGlobalTrack* globalTrack = dynamic_cast<CbmGlobalTrack*>(fGlobalTracks->At(iTr));
      if (!globalTrack) {
        LOG(fatal) << "CbmKfFitTracksTask: null pointer to the global track!";
        return;
      }
      CbmKfTrackFitter::Trajectory t;
      if (!fFitter.CreateGlobalTrack(t, *globalTrack)) {
        LOG(fatal) << "CbmKfFitTracksTask: can not create the global track for the fit! ";
        return;
      }
      fFitter.FitTrajectory(t);
      {
        const auto& parV = t.fNodes.front().fParamUp;
        cbm::algo::kf::TrackParamD parD;
        parD.Set(parV, 0);
        FairTrackParam trackFirst = cbm::kf::ConvertTrackParam(parD);
        globalTrack->SetParamFirst(&trackFirst);
      }
      {
        const auto& parV = t.fNodes.back().fParamDn;
        cbm::algo::kf::TrackParamD parD;
        parD.Set(parV, 0);
        FairTrackParam trackLast = cbm::kf::ConvertTrackParam(parD);
        globalTrack->SetParamLast(&trackLast);
      }
    }
  }
}


void CbmKfFitTracksTask::Finish() {}
