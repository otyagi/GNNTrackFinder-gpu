/* Copyright (C) 2008-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "UEvent.h"

#include "TClonesArray.h"
#include "TString.h"

#include <iostream>

#include "UParticle.h"
using namespace std;

//____________________________________________________________________
//
// UEvent
//
// Class for event description. Contains the particle array
//


//--------------------------------------------------------------------
UEvent::UEvent()
  : TObject()
  , fEventNr(0)
  , fB(0.)
  , fPhi(0.)
  , fNes(1)
  , fStepNr(0)
  , fStepT(0.)
  , fNpa(0)
  , fComment("")
  , fParticles(new TClonesArray("UParticle", 100))
{
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
UEvent::UEvent(const UEvent& right)
  : TObject(right)
  , fEventNr(right.fEventNr)
  , fB(right.fB)
  , fPhi(right.fPhi)
  , fNes(right.fNes)
  , fStepNr(right.fStepNr)
  , fStepT(right.fStepT)
  , fNpa(right.fNpa)
  , fComment(right.fComment)
  , fParticles(new TClonesArray("UParticle", 100))
{
  UParticle* p;
  for (Int_t i = 0; i < fNpa; i++) {
    p = (UParticle*) right.fParticles->At(i);
    new ((*fParticles)[i]) UParticle(*p);
  }
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
UEvent::~UEvent()
{
  // Destructor
  Clear();
  delete fParticles;
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
void UEvent::Print(Option_t* option) const
{
  // Print data members to the standard output
  cout << "---------------------------------------------" << endl
       << "-I-                 Event                 -I-" << endl
       << "Event number               : " << fEventNr << endl
       << "Impact parameter (fm)      : " << fB << endl
       << "Reaction plane angle (rad) : " << fPhi << endl
       << "Number of time steps       : " << fNes << endl
       << "Time step number           : " << fStepNr << endl
       << "Time of the time step (fm) : " << fStepT << endl
       << "Number of particles        : " << fNpa << endl
       << "Comment                    :\n"
       << fComment << endl;
  TString opt = option;
  if (opt.Contains("all")) {
    UParticle* particle;
    for (Int_t iPa = 0; iPa < fNpa; iPa++) {
      particle = (UParticle*) fParticles->At(iPa);
      particle->Print(option);
    }
  }
  cout << "---------------------------------------------" << endl;
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
UParticle* UEvent::GetParticle(Int_t index) const
{
  // Get pointer to the particle.
  // index - index of the particle
  if (index < 0) { return nullptr; }
  if (index >= fNpa) { return nullptr; }
  return ((UParticle*) fParticles->At(index));
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
void UEvent::AddParticle(Int_t index, Int_t pdg, Int_t status, Int_t parent, Int_t parentDecay, Int_t mate, Int_t decay,
                         Int_t child[2], Double_t px, Double_t py, Double_t pz, Double_t e, Double_t x, Double_t y,
                         Double_t z, Double_t t, Double_t weight)
{
  // Add particle to the array
  new ((*fParticles)[fNpa])
    UParticle(index, pdg, status, parent, parentDecay, mate, decay, child, px, py, pz, e, x, y, z, t, weight);
  fNpa += 1;
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
void UEvent::AddParticle(Int_t index, Int_t pdg, Int_t status, Int_t parent, Int_t parentDecay, Int_t mate, Int_t decay,
                         Int_t child[2], TLorentzVector mom, TLorentzVector pos, Double_t weight)
{
  // Add particle to the array
  new ((*fParticles)[fNpa]) UParticle(index, pdg, status, parent, parentDecay, mate, decay, child, mom, pos, weight);
  fNpa += 1;
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
void UEvent::AddParticle(const UParticle& particle)
{
  // Add particle to the array
  new ((*fParticles)[fNpa]) UParticle(particle);
  fNpa += 1;
}
//--------------------------------------------------------------------
UEvent& UEvent::operator=(const UEvent& right)
{
  if (this != &right) {
    fEventNr = right.fEventNr;
    fB       = right.fB;
    fPhi     = right.fPhi;
    fNes     = right.fNes;
    fStepNr  = right.fStepNr;
    fStepT   = right.fStepT;
    fNpa     = right.fNpa;
    fComment = right.fComment;
    fParticles->Clear();
    UParticle* p;
    for (Int_t i = 0; i < fNpa; i++) {
      p = (UParticle*) right.fParticles->At(i);
      new ((*fParticles)[i]) UParticle(*p);
    }
  }
  return *this;
}
//--------------------------------------------------------------------

//--------------------------------------------------------------------
void UEvent::SetParameters(Int_t eventNr, Double_t b, Double_t phi, Int_t nes, Int_t stepNr, Double_t stepT,
                           const char* comment)
{
  // Set the event parameters
  fEventNr = eventNr;
  fB       = b;
  fPhi     = phi;
  fNes     = nes;
  fStepNr  = stepNr;
  fStepT   = stepT;
  fComment = comment;
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
void UEvent::Clear(Option_t*)
{
  // Remove the particles from the array and reset counter
  fParticles->Clear();
  fNpa = 0;
}
//--------------------------------------------------------------------


//--------------------------------------------------------------------
void UEvent::RemoveAt(Int_t i)
{
  // Remove one particle from the array.
  // i - index of the particle.
  // Array is automaticaly compressed afterwards, mind the indexing
  fParticles->RemoveAt(i);
  fParticles->Compress();
}
//--------------------------------------------------------------------


ClassImp(UEvent);
