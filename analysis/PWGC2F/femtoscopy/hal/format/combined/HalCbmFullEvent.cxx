/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmFullEvent.h"

#include "CbmMCEvent.h"
#include "CbmTrackMatchNew.h"
#include "HalCbmEvent.h"
#include "HalCbmEventInterface.h"
#include "HalCbmMCEvent.h"
#include "HalCbmMCEventInterface.h"

#include <AnalysisTree/Constants.hpp>

#include <Hal/ComplexEvent.h>
#include <Hal/ComplexEventInterface.h>
#include <Hal/ComplexTrack.h>


HalCbmFullEvent::HalCbmFullEvent() : Hal::ComplexEvent(new HalCbmEvent(), new HalCbmMCEvent()) {}

void HalCbmFullEvent::Update(Hal::EventInterface* interface)
{
  Hal::ComplexEventInterface* interf = static_cast<Hal::ComplexEventInterface*>(interface);
  fImgEvent->Update(interf->GetImag());
  fRealEvent->Update(interf->GetReal());
  Hal::Event::ShallowCopyEvent(fRealEvent);
  fTracks->Clear();
  fTotalTracksNo = fRealEvent->GetTotalTrackNo();
  fTracks->ExpandCreateFast(fTotalTracksNo);
  HalCbmEventInterface* evInt = (HalCbmEventInterface*) interf->GetReal();
  switch (evInt->GetFormatType()) {
    case HalCbm::DataFormat::kAnalysisTree: {
      UpdateAnalysisTree(interf);
    } break;
    case HalCbm::DataFormat::kDST: {
      UpdateDst(interf);
    } break;

    case HalCbm::DataFormat::kUnknown: {
      //Do nothing
    } break;
  }
}

Hal::Event* HalCbmFullEvent::GetNewEvent() const { return new HalCbmFullEvent(); }

HalCbmFullEvent::~HalCbmFullEvent() {}

HalCbmFullEvent::HalCbmFullEvent(Hal::Event* re, Hal::Event* im) : Hal::ComplexEvent(re, im) {}

void HalCbmFullEvent::UpdateAnalysisTree(Hal::ComplexEventInterface* interface)
{
  HalCbmEventInterface* s             = (HalCbmEventInterface*) interface->GetReal();
  CbmAnaTreeRecoSourceContainer* reco = (CbmAnaTreeRecoSourceContainer*) s->GetContainer();
  auto vecToSim                       = reco->GetVtx2Sim();
  for (Int_t i = 0; i < fTotalTracksNo; i++) {
    Hal::ComplexTrack* track = (Hal::ComplexTrack*) fTracks->UncheckedAt(i);
    track->ResetTrack(i, this);
    track->SetRealTrack(fRealEvent->GetTrack(i));
    track->Hal::Track::CopyData(fRealEvent->GetTrack(i));
    Int_t match = vecToSim->GetMatchDirect(i);

    if (match < 0 || match == AnalysisTree::UndefValueInt) {
      track->SetImgTrack(nullptr);
    }
    else {
      track->SetImgTrack(fImgEvent->GetTrack(match));
      track->SetMatchID(match);
    }
  }
}

void HalCbmFullEvent::UpdateDst(Hal::ComplexEventInterface* interface)
{
  HalCbmEventInterface* s     = (HalCbmEventInterface*) interface;
  HalCbmMCEventInterface* ims = (HalCbmMCEventInterface*) interface->GetImag();
  for (Int_t i = 0; i < fTotalTracksNo; i++) {
    Hal::ComplexTrack* track = (Hal::ComplexTrack*) fTracks->UncheckedAt(i);
    track->ResetTrack(i, this);
    track->SetRealTrack(fRealEvent->GetTrack(i));
    track->Hal::Track::CopyData(fRealEvent->GetTrack(i));
    CbmGlobalTrack* glob = (CbmGlobalTrack*) s->fGlobalTracks->UncheckedAt(i);
    Int_t sts_index      = glob->GetStsTrackIndex();
    if (sts_index >= 0) {
      CbmTrackMatchNew* match = (CbmTrackMatchNew*) ims->fStsMatches->UncheckedAt(sts_index);
      Int_t mcId              = match->GetMatchedLink().GetIndex();
      if (mcId > 0) {
        track->SetImgTrack(fImgEvent->GetTrack(mcId));
        track->SetMatchID(mcId);
      }
    }
  }
  //
}
