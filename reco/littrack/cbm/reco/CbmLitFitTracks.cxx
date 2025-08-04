/* Copyright (C) 2013-2020 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

/**
 * \file CbmLitFitTracks.cxx
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2013
 */

#include "CbmLitFitTracks.h"

#include "CbmStsTrack.h"
#include "CbmTrack.h"
#include "FairRootManager.h"
#include "FairTrackParam.h"
#include "TClonesArray.h"
#include "base/CbmLitPtrTypes.h"
#include "base/CbmLitToolFactory.h"
#include "base/CbmLitTypes.h"
#include "cbm/utils/CbmLitConverter.h"
#include "cbm/utils/CbmLitConverterFairTrackParam.h"
#include "std/data/CbmLitTrack.h"
#include "std/utils/CbmLitMemoryManagment.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::for_each;
using std::vector;

CbmLitFitTracks::CbmLitFitTracks()
  : FairTask()
  , fGlobalTracks(NULL)
  , fStsTracks(NULL)
  , fMuchTracks(NULL)
  , fTrdTracks(NULL)
  , fMuchPixelHits(NULL)
  , fMuchStrawHits(NULL)
  , fTrdHits(NULL)
{
}

CbmLitFitTracks::~CbmLitFitTracks() {}

InitStatus CbmLitFitTracks::Init()
{
  ReadDataBranches();

  fFitWatch.Reset();
  fFitWithIOWatch.Reset();

  return kSUCCESS;
}

void CbmLitFitTracks::Exec(Option_t* opt)
{
  static Int_t eventNo = 0;
  std::cout << "CbmLitFitTracks::Exec: eventNo=" << eventNo++ << std::endl;

  DoFit();
}

void CbmLitFitTracks::Finish() { PrintStopwatchStatistics(); }

void CbmLitFitTracks::ReadDataBranches()
{
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman != NULL);

  fGlobalTracks  = (TClonesArray*) ioman->GetObject("GlobalTrack");
  fStsTracks     = (TClonesArray*) ioman->GetObject("StsTrack");
  fMuchTracks    = (TClonesArray*) ioman->GetObject("MuchTrack");
  fTrdTracks     = (TClonesArray*) ioman->GetObject("TrdTrack");
  fMuchPixelHits = (TClonesArray*) ioman->GetObject("MuchPixelHit");
  fMuchStrawHits = (TClonesArray*) ioman->GetObject("MuchStrawHit");
  fTrdHits       = (TClonesArray*) ioman->GetObject("TrdHit");
}

void CbmLitFitTracks::DoFit()
{
  static Bool_t firstTime = true;
  static TrackFitterPtr fitter;
  if (firstTime) {
    fitter    = CbmLitToolFactory::CreateTrackFitter("lit_kalman");
    firstTime = false;
  }

  fFitWithIOWatch.Start(kFALSE);

  // Convert input data
  TrackPtrVector ltracks;
  HitPtrVector lhits;
  CbmLitConverter::HitArrayToHitVector(0, ECbmDataType::kTrdHit, fTrdHits, lhits);
  CbmLitConverter::CbmTrackArrayToCbmLitTrackArray(fTrdTracks, lhits, ltracks);

  // Replace first track parameter of the converted tracks with the last STS track parameter
  Int_t nofTracks = ltracks.size();
  for (Int_t iTrack = 0; iTrack < nofTracks; iTrack++) {
    CbmLitTrack* track    = ltracks[iTrack];
    Int_t stsTrackId      = track->GetPreviousTrackId();
    CbmStsTrack* stsTrack = static_cast<CbmStsTrack*>(fStsTracks->At(stsTrackId));
    CbmTrackParam cbmLPar;
    cbmLPar.Set(*stsTrack->GetParamLast(), stsTrack->GetLastHitTime(), stsTrack->GetLastHitTimeError());
    CbmLitTrackParam lpar;
    CbmLitConverterFairTrackParam::FairTrackParamToCbmLitTrackParam(&cbmLPar, &lpar);
    track->SetParamFirst(&lpar);
    track->SetPDG(211);
  }

  fFitWatch.Start(kFALSE);
  for (Int_t iTrack = 0; iTrack < nofTracks; iTrack++) {
    CbmLitTrack* track = ltracks[iTrack];
    fitter->Fit(track);
  }
  fFitWatch.Stop();

  // Replace first and last track parameters for the fitted track
  for (Int_t iTrack = 0; iTrack < nofTracks; iTrack++) {
    CbmLitTrack* track = ltracks[iTrack];
    CbmTrack* trdTrack = static_cast<CbmTrack*>(fTrdTracks->At(iTrack));
    FairTrackParam firstParam, lastParam;
    CbmLitConverterFairTrackParam::CbmLitTrackParamToFairTrackParam(track->GetParamFirst(), &firstParam);
    CbmLitConverterFairTrackParam::CbmLitTrackParamToFairTrackParam(track->GetParamLast(), &lastParam);
    trdTrack->SetParamFirst(&firstParam);
    trdTrack->SetParamLast(&lastParam);
  }

  // Clear memory
  for_each(ltracks.begin(), ltracks.end(), DeleteObject());
  for_each(lhits.begin(), lhits.end(), DeleteObject());
  ltracks.clear();
  lhits.clear();

  fFitWithIOWatch.Stop();
}

void CbmLitFitTracks::PrintStopwatchStatistics()
{
  cout << "CbmLitFitTracks::PrintStopwatchStatistics: " << endl;
  cout << "fit without IO: counts=" << fFitWatch.Counter() << ", real=" << fFitWatch.RealTime() / fFitWatch.Counter()
       << "/" << fFitWatch.RealTime() << " s, cpu=" << fFitWatch.CpuTime() / fFitWatch.Counter() << "/"
       << fFitWatch.CpuTime() << endl;
  cout << "fit with IO: counts=" << fFitWithIOWatch.Counter()
       << ", real=" << fFitWithIOWatch.RealTime() / fFitWithIOWatch.Counter() << "/" << fFitWithIOWatch.RealTime()
       << " s, cpu=" << fFitWithIOWatch.CpuTime() / fFitWithIOWatch.Counter() << "/" << fFitWithIOWatch.CpuTime()
       << endl;
}
ClassImp(CbmLitFitTracks);
