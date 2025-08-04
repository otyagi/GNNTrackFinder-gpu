/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Alex Bercuci*/

/// \file    CbmPVFinderKFGlobal.h
/// \brief   Primary vertex finder from the global tracks (implementation)
/// \since   09.10.2023
/// \authors Anna Senger, Sergei Zharko

#include "CbmPVFinderKFGlobal.h"

#include "CbmEvent.h"
#include "CbmGlobalTrack.h"
#include "CbmKFPrimaryVertexFinder.h"
#include "CbmKFTrack.h"
#include "CbmKFVertex.h"
#include "TClonesArray.h"

#include <cmath>
#include <vector>

ClassImp(CbmPVFinderKFGlobal);

// ---------------------------------------------------------------------------------------------------------------------
//
Int_t CbmPVFinderKFGlobal::FindPrimaryVertex(TClonesArray* tracks, CbmVertex* vertex)
{
  Int_t nTracks = tracks->GetEntriesFast();

  CbmKFPrimaryVertexFinder finder;
  std::vector<CbmKFTrack> vKFTracks(nTracks);
  for (int iT = 0; iT < nTracks; ++iT) {
    auto* globalTrack = dynamic_cast<CbmGlobalTrack*>(tracks->At(iT));

    if (globalTrack->GetStsTrackIndex() < 0) {
      continue;
    }
    if (globalTrack->GetTrdTrackIndex() < 0) {
      continue;
    }
    if (globalTrack->GetTofTrackIndex() < 0) {
      continue;
    }

    if (globalTrack->GetChi2() < 0.) continue;
    CbmKFTrack& kfTrack = vKFTracks[iT];
    kfTrack.SetGlobalTrack(*globalTrack);
    if (!std::isfinite(kfTrack.GetTrack()[0]) || !std::isfinite(kfTrack.GetCovMatrix()[0])) continue;
    finder.AddTrack(&kfTrack);
  }
  CbmKFVertex kfVertex;
  finder.Fit(kfVertex);
  kfVertex.GetVertex(*vertex);

  // Re-fit vertices of the global tracks
  for (int iT = 0; iT < nTracks; ++iT) {
    auto* globalTrack = dynamic_cast<CbmGlobalTrack*>(tracks->At(iT));
    auto& kfTrack     = vKFTracks[iT];
    kfTrack.Fit2Vertex(kfVertex);
    FairTrackParam par;
    kfTrack.GetTrackParam(par);
    globalTrack->SetParamPrimaryVertex(&par);
  }

  return vertex->GetNTracks();
}

// ---------------------------------------------------------------------------------------------------------------------
//
Int_t CbmPVFinderKFGlobal::FindEventVertex(CbmEvent* event, TClonesArray* tracks)
{
  assert(event);
  CbmKFPrimaryVertexFinder finder;

  // Get vertex object
  CbmVertex* vertex = event->GetVertex();

  // Copy input tracks to KF tracks
  Int_t nTracks = event->GetNofData(ECbmDataType::kGlobalTrack);
  if (nTracks <= 0) return 0;

  std::vector<CbmKFTrack> vKFTracks(nTracks);
  for (Int_t iT = 0; iT < nTracks; ++iT) {
    auto iTrkEvent    = event->GetIndex(ECbmDataType::kGlobalTrack, iT);
    auto* globalTrack = dynamic_cast<CbmGlobalTrack*>(tracks->At(iTrkEvent));

    // if (globalTrack->GetStsTrackIndex() < 0) {
    //   continue;
    // }
    // if (globalTrack->GetTrdTrackIndex() < 0) {
    //   continue;
    // }
    // if (globalTrack->GetTofTrackIndex() < 0) {
    //   continue;
    // }

    if (globalTrack->GetChi2() < 0.) continue;
    CbmKFTrack& kfTrack = vKFTracks[iT];
    kfTrack.SetGlobalTrack(*globalTrack);
    if (!std::isfinite(kfTrack.GetTrack()[0]) || !std::isfinite(kfTrack.GetCovMatrix()[0])) continue;
    finder.AddTrack(&kfTrack, iTrkEvent);
  }

  // Do the vertex finding
  CbmKFVertex kfVertex;
  finder.Fit(kfVertex);

  // Copy KFVertex into CbmVertex
  kfVertex.GetVertex(*vertex);
  std::vector<uint32_t> idx;
  finder.GetUsedTracks(idx);
  vertex->SetTracks(idx);
  // Re-fit vertices of the global tracks
  for (int iT = 0; iT < nTracks; ++iT) {
    auto iTrkEvent    = event->GetIndex(ECbmDataType::kGlobalTrack, iT);
    auto* globalTrack = dynamic_cast<CbmGlobalTrack*>(tracks->At(iTrkEvent));
    auto& kfTrack     = vKFTracks[iT];
    // TODO: SZh. 03.07.2024: Provide time initialization (to CbmTrackParam)
    kfTrack.Fit2Vertex(kfVertex);
    FairTrackParam par;
    kfTrack.GetTrackParam(par);
    globalTrack->SetParamPrimaryVertex(&par);
  }
  return vertex->GetNTracks();
}
