/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmEvent.h"

#include "CbmAnaTreeContainer.h"
#include "CbmGlobalTrack.h"
#include "CbmTofHit.h"
#include "HalCbmEventInterface.h"
#include "HalCbmTrack.h"

#include <TLorentzVector.h>
#include <TMath.h>

#include <AnalysisTree/Constants.hpp>
#include <AnalysisTree/Matching.hpp>

#include <Hal/DataFormat.h>
#include <Hal/Event.h>
#include <Hal/ExpEvent.h>

HalCbmEvent::HalCbmEvent() : Hal::ExpEvent("HalCbmTrack") {}

HalCbmEvent::HalCbmEvent(const HalCbmEvent& other) : Hal::ExpEvent(other) {}

void HalCbmEvent::Update(Hal::EventInterface* interface)
{
  fTracks->Clear();
  HalCbmEventInterface* source = (HalCbmEventInterface*) interface;

  switch (source->fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      UpdateAnaTree(source);
    } break;
    case HalCbm::DataFormat::kDST: {
      UpdateDST(source);
    } break;
    default: break;
  }
}

HalCbmEvent::HalCbmEvent(TString classname) : Hal::ExpEvent(classname) {}

HalCbmEvent::~HalCbmEvent() {}

Bool_t HalCbmEvent::ExistInTree() const
{
  Bool_t exist  = CheckBranches(1, "CbmAnaTreeSourceContainer.");
  Bool_t exist2 = CheckBranches(1, "GlobalTrack");
  if (exist || exist2) return kTRUE;
  return kFALSE;
}

void HalCbmEvent::UpdateDST(HalCbmEventInterface* ei)
{
  fTotalTracksNo       = ei->GetTotalTrackNo();
  TLorentzVector start = ei->GetVertex();
  fVertex->SetXYZT(start.X(), start.Y(), start.Z(), start.T());
  fTracks->ExpandCreateFast(fTotalTracksNo);
  for (int i = 0; i < fTotalTracksNo; i++) {
    HalCbmTrack* track = (HalCbmTrack*) fTracks->UncheckedAt(i);
    track->ResetTrack(i, this);
    CbmGlobalTrack* glob = (CbmGlobalTrack*) ei->fGlobalTracks->UncheckedAt(i);
    track->SetChi2(glob->GetChi2());
    track->SetVertexChi2(0);
    track->SetTrackLenght(glob->GetLength());

    const CbmTrackParam* track_param = glob->GetParamVertex();
    track->SetMomentum(track_param->GetPx(), track_param->GetPy(), track_param->GetPz(), 0);
    track->SetDCA(track_param->GetX() - fVertex->X(), track_param->GetY() - fVertex->Y(),
                  track_param->GetZ() - fVertex->Z());
    Int_t sts_index = glob->GetStsTrackIndex();
    Int_t tof_index = glob->GetTofHitIndex();
    if (track_param->GetQp() > 0) {
      track->SetCharge(1);
    }
    else {
      track->SetCharge(-1);
    }
    if (sts_index >= 0) {
      CbmStsTrack* sts = (CbmStsTrack*) ei->fStsTracks->UncheckedAt(sts_index);
      track->SetNMvdHits(sts->GetNofMvdHits());
      track->SetNStsHits(sts->GetNofStsHits());
    }
    else {
      track->SetNMvdHits(0);
      track->SetNStsHits(0);
    }
    track->BuildHelix();
    Hal::ToFTrack* tof = (Hal::ToFTrack*) track->GetDetTrack(Hal::DetectorID::kTOF);

    if (tof_index >= 0) {
      CbmTofHit* hit = (CbmTofHit*) ei->fTofHits->UncheckedAt(tof_index);
      Double_t t     = hit->GetTime();
      Double_t beta  = track->GetTrackLenght() / t / (29.9792458);
      Double_t p     = track->GetMomentum().P();
      Double_t m2    = p * p * (1. / beta / beta - 1.);
      tof->SetMass2(m2);
      tof->SetBeta(beta);
      tof->SetFlag(1);
    }
    else {
      tof->SetMass2(Hal::ToFTrack::DummyVal());
      tof->SetBeta(Hal::ToFTrack::DummyVal());
      tof->SetFlag(0);
    }
  }
}

void HalCbmEvent::UpdateAnaTree(HalCbmEventInterface* ei)
{
  CbmAnaTreeRecoSourceContainer* container = ei->GetContainer();
  fTotalTracksNo                           = ei->GetTotalTrackNo();
  TLorentzVector start                     = ei->GetVertex();
  fVertex->SetXYZT(start.X(), start.Y(), start.Z(), start.T());
  fTracks->ExpandCreateFast(fTotalTracksNo);
  AnaTreeRecoIds ids = container->GetFieldIds();
  for (int i = 0; i < fTotalTracksNo; i++) {
    AnalysisTree::Track p          = container->GetVtxTracks()->GetChannel(i);
    AnalysisTree::ShortInt_t match = container->GetVtx2ToFMatch()->GetMatchDirect(i);
    HalCbmTrack* track             = (HalCbmTrack*) fTracks->UncheckedAt(i);
    track->ResetTrack(i, this);
    track->SetChi2(p.GetField<float>(ids.vtx_chi2));
    track->SetVertexChi2(p.GetField<float>(ids.vtx_vtxchi2));
    track->SetMomentum(p.GetPx(), p.GetPy(), p.GetPz(), 0);
    track->SetDCA(p.GetField<float>(ids.vtx_dcax), p.GetField<float>(ids.vtx_dcay), p.GetField<float>(ids.vtx_dcaz));
    track->SetNMvdHits(p.GetField<int>(ids.vtx_mvdhits));
    track->SetNStsHits(p.GetField<int>(ids.vtx_nhits) - track->GetNMvdHits());
    track->SetCharge(p.GetField<int>(ids.vtx_q));
    track->BuildHelix();
    Hal::ToFTrack* tof = (Hal::ToFTrack*) track->GetDetTrack(Hal::DetectorID::kTOF);
    if (match == AnalysisTree::UndefValueShort) {
      /*no tof*/
      tof->SetMass2(Hal::ToFTrack::DummyVal());
      tof->SetBeta(Hal::ToFTrack::DummyVal());
      tof->SetFlag(0);
    }
    else {
      AnalysisTree::Hit tof_hit = container->GetTofHits()->GetChannel(match);
      Double_t P                = track->GetMomentum().P();
      Double_t m2               = tof_hit.GetField<float>(ids.tof_mass2);
      const TLorentzVector& vec = track->GetMomentum();
      Double_t mom2             = vec.Px() * vec.Px() + vec.Py() * vec.Py() + vec.Pz() * vec.Pz();
      Double_t E                = TMath::Sqrt(mom2 + m2);
      Double_t beta             = P / E;
      tof->SetMass2(m2);
      tof->SetBeta(beta);
      tof->SetFlag(1);
    }
  }
}

Hal::EventInterface* HalCbmEvent::CreateInterface() const { return new HalCbmEventInterface(); }
