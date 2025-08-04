/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmMCEvent.h"

#include "AnalysisTree/EventHeader.hpp"
#include "AnalysisTree/Particle.hpp"
#include "CbmAnaTreeSource.h"
#include "CbmMCTrack.h"
#include "HalCbmMCEventInterface.h"
#include "HalCbmMCTrack.h"

#include <FairMCEventHeader.h>

HalCbmMCEvent::HalCbmMCEvent() : Hal::McEvent("HalCbmMCTrack") {}

HalCbmMCEvent::HalCbmMCEvent(const HalCbmMCEvent& other) : Hal::McEvent(other) {}

void HalCbmMCEvent::Update(Hal::EventInterface* interface)
{
  HalCbmMCEventInterface* s = static_cast<HalCbmMCEventInterface*>(interface);
  fTotalTracksNo            = s->GetTotalTrackNo();
  fTracks->Clear();
  fTracks->ExpandCreateFast(fTotalTracksNo);
  switch (s->fFormatType) {
    case HalCbm::DataFormat::kAnalysisTree: {
      UpdateAnalysisTree(s);
    } break;
    case HalCbm::DataFormat::kDST: {
      UpdateDst(s);
    } break;
    case HalCbm::DataFormat::kUnknown: {
      //Do nothing
    } break;
  }
}

Bool_t HalCbmMCEvent::ExistInTree() const
{
  Bool_t exist  = CheckBranches(1, "CbmAnaTreeMcSourceContainer.");
  Bool_t exist2 = CheckBranches(1, "CbmMCTrack");
  if (exist || exist2) return kTRUE;
  return kFALSE;
}

HalCbmMCEvent::~HalCbmMCEvent() {}

void HalCbmMCEvent::ShallowCopyTracks(Hal::Event* event)
{
  fTracks->Clear();
  Hal::McEvent* mc_event = (Hal::McEvent*) event;
  fTotalTracksNo         = mc_event->GetTotalTrackNo();
  for (int i = 0; i < fTotalTracksNo; i++) {
    Hal::McTrack* to   = (Hal::McTrack*) fTracks->ConstructedAt(i);
    Hal::McTrack* from = (Hal::McTrack*) mc_event->GetTrack(i);
    to->ResetTrack(i, this);
    to->CopyData(from);
  }
}

void HalCbmMCEvent::UpdateAnalysisTree(Hal::EventInterface* source)
{
  HalCbmMCEventInterface* s         = static_cast<HalCbmMCEventInterface*>(source);
  CbmAnaTreeMcSourceContainer* data = (CbmAnaTreeMcSourceContainer*) s->GetRawEventPointer();
  AnaTreeMcIds conf                 = data->GetFieldIds();
  Bool_t UseFreez                   = kTRUE;
  if (conf.freezX == AnalysisTree::UndefValueShort) {
    UseFreez = kFALSE;
  }
  fB   = data->GetEventHeader()->GetField<Float_t>(conf.event_b);
  fPhi = data->GetEventHeader()->GetField<Float_t>(conf.event_psi);

  for (int i = 0; i < fTotalTracksNo; i++) {
    HalCbmMCTrack* track = (HalCbmMCTrack*) fTracks->UncheckedAt(i);
    track->ResetTrack(i, this);
    AnalysisTree::Particle particle = data->GetParticles()->GetChannel(i);
    Double_t px                     = particle.GetField<float>(conf.px);
    Double_t py                     = particle.GetField<float>(conf.py);
    Double_t pz                     = particle.GetField<float>(conf.pz);
    Double_t mass                   = particle.GetField<float>(conf.mass);

    Int_t mother_id = particle.GetField<int>(conf.motherId);
    Int_t pid       = particle.GetField<int>(conf.pdg);
    Double_t e      = TMath::Sqrt(px * px + py * py + pz * pz + mass * mass);
    track->SetMomentum(px, py, pz, e);
    track->SetPdg(pid);
    if (UseFreez) {
      Double_t x = particle.GetField<float>(conf.freezX);
      Double_t y = particle.GetField<float>(conf.freezY);
      Double_t z = particle.GetField<float>(conf.freezZ);
      Double_t t = particle.GetField<float>(conf.freezT);
      track->SetFreezoutPosition(x, y, z, t);
    }

    if (mother_id == -1) {
      track->SetPrimary();
    }
    else {
      track->SetMotherIndex(track->GetMotherIndex());
    }
    track->SetCharge(CalculateCharge(pid));
  }
}

void HalCbmMCEvent::UpdateDst(Hal::EventInterface* source)
{
  HalCbmMCEventInterface* s = static_cast<HalCbmMCEventInterface*>(source);
  TClonesArray* mcTracks    = s->fCbmMCtracks;
  fB                        = s->fEventHeader->GetB();
  for (int i = 0; i < fTotalTracksNo; i++) {
    HalCbmMCTrack* track = (HalCbmMCTrack*) fTracks->UncheckedAt(i);
    track->ResetTrack(i, this);
    CbmMCTrack* mc = (CbmMCTrack*) mcTracks->UncheckedAt(i);
    track->SetMomentum(mc->GetPx(), mc->GetPy(), mc->GetPz(), mc->GetEnergy());
    track->SetMotherIndex(mc->GetMotherId());
    track->SetPdg(mc->GetPdgCode());
    track->SetCharge(mc->GetCharge());
  }
}

Hal::EventInterface* HalCbmMCEvent::CreateInterface() const { return new HalCbmMCEventInterface(); }
