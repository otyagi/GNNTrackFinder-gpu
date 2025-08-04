/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmUnigenEventInterface.h"

#include <Hal/DataManager.h>
#include <Hal/TrackInterface.h>
#include <HalCbmUnigenTrackInterface.h>

HalCbmUnigenEventInterface::HalCbmUnigenEventInterface() : fEvent(nullptr) { fEvent = new UEvent(); }

void HalCbmUnigenEventInterface::Compress(Int_t* map, Int_t map_size)
{
  Int_t track_pos = 0;
  for (int i = 0; i < map_size; i++) {
    Int_t good_track = map[i];
    for (int j = track_pos; j < good_track; j++) {
      fEvent->RemoveAt(j);
    }
    track_pos = good_track + 1;
  }
}

void HalCbmUnigenEventInterface::CopyData(Hal::EventInterface* s)
{
#ifdef UNIGEN_OLD
  CopyUnigen(((HalCbmUnigenEventInterface*) s)->fEvent, fEvent);
#else
  *fEvent = *((HalCbmUnigenEventInterface*) s)->fEvent;
#endif
}

void HalCbmUnigenEventInterface::CopyAndCompress(Hal::EventInterface* s, Int_t* map, Int_t map_size)
{
  HalCbmUnigenEventInterface* ev = (HalCbmUnigenEventInterface*) s;
  fEvent->SetB(ev->fEvent->GetB());
  fEvent->SetPhi(ev->fEvent->GetPhi());
  fEvent->SetNes(ev->fEvent->GetNes());
  fEvent->SetStepNr(ev->fEvent->GetStepNr());
  fEvent->SetStepT(ev->fEvent->GetStepT());
#ifdef UNIGEN_OLD
  fEvent->GetParticleList()->Clear();
#else
  TString comment;
  ev->fEvent->GetComment(comment);
  fEvent->SetComment(comment);
  fEvent->Clear();
#endif
  for (int i = 0; i < map_size; i++) {
    fEvent->AddParticle(*ev->fEvent->GetParticle(map[i]));
  }
}

void HalCbmUnigenEventInterface::ConnectToTreeInternal(EventInterface::eMode /*mode*/)
{
  Hal::DataManager* manager = Hal::DataManager::Instance();
  fEvent                    = (UEvent*) manager->GetObject("UEvent.");
}

void HalCbmUnigenEventInterface::Boost(Double_t vx, Double_t vy, Double_t vz)
{
  for (int i = 0; i < fEvent->GetNpa(); i++) {
    UParticle* p       = fEvent->GetParticle(i);
    TLorentzVector mom = p->GetMomentum();
    TLorentzVector pos = p->GetPosition();
    mom.Boost(vx, vy, vz);
    pos.Boost(vx, vy, vz);
    p->SetMomentum(mom);
    p->SetPosition(pos);
  }
}

HalCbmUnigenEventInterface::~HalCbmUnigenEventInterface() {}

Hal::TrackInterface* HalCbmUnigenEventInterface::GetTrackInterface() const { return new HalCbmUnigenTrackInterface(); }

void HalCbmUnigenEventInterface::Register(Bool_t write)
{
  if (fEvent == NULL) fEvent = new UEvent();
  Hal::DataManager* manager = Hal::DataManager::Instance();
  manager->Register("Event", "", (TNamed*) fEvent, write);
}

void HalCbmUnigenEventInterface::FillTrackInterface(Hal::TrackInterface* track, Int_t index)
{
  track->SetRawTrack(fEvent->GetParticle(index));
}
