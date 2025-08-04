/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmUnigenEvent.h"

#include "HalCbmUnigenEventInterface.h"

#include <TClonesArray.h>
#include <TDatabasePDG.h>
#include <TLorentzVector.h>
#include <TParticlePDG.h>

#include <Hal/DataManager.h>
#include <Hal/EventInterface.h>
#include <Hal/McTrack.h>
#include <Hal/Track.h>

HalCbmUnigenEvent::HalCbmUnigenEvent() : Hal::McEvent("HalCbmUnigenTrack") {}

void HalCbmUnigenEvent::Update(Hal::EventInterface* interface)
{
  UEvent* temp   = ((HalCbmUnigenEventInterface*) interface)->fEvent;
  fB             = temp->GetB();
  fPhi           = temp->GetPhi();
  fTotalTracksNo = temp->GetNpa();
  fTracks->Clear();
  for (int i = 0; i < fTotalTracksNo; i++) {
    UParticle* particle    = temp->GetParticle(i);
    TParticlePDG* pdg_part = fPDG->GetParticle(particle->GetPdg());
    Double_t charge        = 0;
    if (pdg_part) {
      charge = pdg_part->Charge() / 3.0;
    }
    Hal::McTrack* target_track = (Hal::McTrack*) fTracks->ConstructedAt(i);
    target_track->ResetTrack(i, this);
    target_track->SetCharge(charge);
    target_track->SetPdg(particle->GetPdg());
    if (particle->GetParent() < 0) {
      target_track->SetPrimary();
    }
    else {
      target_track->SetMotherIndex(particle->GetParent());
    }
    target_track->SetMomentum(particle->Px(), particle->Py(), particle->Pz(), particle->E());
    target_track->SetFreezoutPosition(particle->X(), particle->Y(), particle->Z(), particle->T());
    target_track->SetStatus(particle->GetStatus());
  }
}

void HalCbmUnigenEvent::Clear(Option_t* opt) { Hal::McEvent::Clear(opt); }

HalCbmUnigenEvent::HalCbmUnigenEvent(const HalCbmUnigenEvent& other) : Hal::McEvent(other) {}

HalCbmUnigenEvent::~HalCbmUnigenEvent() {}

TString HalCbmUnigenEvent::GetFormatName() const { return "UnigenFormat"; }

Bool_t HalCbmUnigenEvent::ExistInTree() const
{
  Hal::DataManager* manager = Hal::DataManager::Instance();
  if (manager->CheckBranch("UEvent.")) {
    return kTRUE;
  }
  return kFALSE;
}

Hal::EventInterface* HalCbmUnigenEvent::CreateInterface() const { return new HalCbmUnigenEventInterface(); }
