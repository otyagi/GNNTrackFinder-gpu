/* Copyright (C) 2006-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Denis Bertini [committer] */

#include "CbmPVFinderKF.h"

#include "CbmEvent.h"
#include "CbmKFPrimaryVertexFinder.h"
#include "CbmKFTrack.h"
#include "CbmKFVertex.h"
#include "CbmStsTrack.h"
#include "TClonesArray.h"

#include <cmath>

// ---------------------------------------------------------------------------------------------------------------------
//
Int_t CbmPVFinderKF::FindPrimaryVertex(TClonesArray* tracks, CbmVertex* vertex)
{

  Int_t NTracks = tracks->GetEntriesFast();

  CbmKFPrimaryVertexFinder Finder;
  CbmKFTrack* CloneArray = new CbmKFTrack[NTracks];
  for (Int_t i = 0; i < NTracks; i++) {
    CbmStsTrack* st = (CbmStsTrack*) tracks->At(i);
    Int_t NHits     = st->GetTotalNofHits();
    if (NHits < 4) {
      continue;
    }
    if (st->GetFlag()) {
      continue;
    }
    if (st->GetChiSq() < 0. || st->GetChiSq() > 3.5 * 3.5 * st->GetNDF()) {
      continue;
    }
    CbmKFTrack& T = CloneArray[i];
    T.SetStsTrack(*st);
    if (!std::isfinite(T.GetTrack()[0]) || !std::isfinite(T.GetCovMatrix()[0])) {
      continue;
    }
    Finder.AddTrack(&T);
  }
  CbmKFVertex v;
  Finder.Fit(v);
  v.GetVertex(*vertex);
  delete[] CloneArray;
  return vertex->GetNTracks();
}

// ---------------------------------------------------------------------------------------------------------------------
//
Int_t CbmPVFinderKF::FindEventVertex(CbmEvent* event, TClonesArray* tracks)
{

  assert(event);
  CbmKFPrimaryVertexFinder vFinder;

  // Get vertex object
  CbmVertex* vertex = event->GetVertex();

  // Copy input tracks to KF tracks
  Int_t nTracks = event->GetNofData(ECbmDataType::kStsTrack);
  if (nTracks <= 0) {
    return 0;
  }

  CbmKFTrack* trackArray = new CbmKFTrack[nTracks];
  for (Int_t iTrack = 0; iTrack < nTracks; iTrack++) {
    Int_t trackIndex   = event->GetIndex(ECbmDataType::kStsTrack, iTrack);
    CbmStsTrack* track = (CbmStsTrack*) tracks->At(trackIndex);
    Int_t nHits        = track->GetTotalNofHits();
    if (nHits < 4) {  // use only tracks with at least 4 hits
      continue;
    }
    if (track->GetFlag()) {  // do not use suspicious tracks
      continue;
    }
    if (track->GetChiSq() < 0. || track->GetChiSq() > 12.25 * track->GetNDF()) {  // use only good-quality tracks
      continue;
    }
    CbmKFTrack& kTrack = trackArray[iTrack];
    kTrack.SetStsTrack(*track);
    if (!std::isfinite(kTrack.GetTrack()[0]) || !std::isfinite(kTrack.GetCovMatrix()[0])) {
      continue;
    }
    vFinder.AddTrack(&kTrack);
  }

  // Do the vertex finding
  CbmKFVertex v;
  vFinder.Fit(v);

  // Copy KFVertex into CbmVertex
  v.GetVertex(*vertex);

  delete[] trackArray;
  return vertex->GetNTracks();
}

ClassImp(CbmPVFinderKF);
