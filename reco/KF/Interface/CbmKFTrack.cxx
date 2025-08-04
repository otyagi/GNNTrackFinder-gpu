/* Copyright (C) 2006-2013 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Denis Bertini [committer], Florian Uhlig, Andrey Lebedev */

#include "CbmKFTrack.h"

#include "CbmGlobalTrack.h"
#include "CbmKFMath.h"
#include "CbmStsTrack.h"
#include "FairTrackParam.h"
#include "TDatabasePDG.h"
#include "TMath.h"
#include "TParticlePDG.h"

ClassImp(CbmKFTrack)

  CbmKFTrack::CbmKFTrack()
  : fMass(0)
  , fChi2(0)
  , fIsElectron(kFALSE)
  , fNDF(0)
  , fHits()
{
  for (Int_t i = 0; i < 6; i++) {
    fT[i] = 0.;
  }
  for (Int_t i = 0; i < 15; i++) {
    fC[i] = 0.;
  }
}


void CbmKFTrack::SetTrack(CbmKFTrackInterface& track)
{
  for (Int_t i = 0; i < 6; i++) {
    fT[i] = track.GetTrack()[i];
  }
  for (Int_t i = 0; i < 15; i++) {
    fC[i] = track.GetCovMatrix()[i];
  }
  fMass       = track.GetMass();
  fIsElectron = track.IsElectron();
  fChi2       = track.GetRefChi2();
  fNDF        = track.GetRefNDF();
}

void CbmKFTrack::SetTrackParam(const FairTrackParam& track) { CbmKFMath::CopyTrackParam2TC(&track, fT, fC); }

void CbmKFTrack::SetStsTrack(CbmStsTrack& track, bool first)
{
  SetPID(track.GetPidHypo());
  SetTrackParam(first ? *track.GetParamFirst() : *track.GetParamLast());
  GetRefChi2() = track.GetChiSq();
  GetRefNDF()  = track.GetNDF();
}

void CbmKFTrack::SetGlobalTrack(CbmGlobalTrack& track, bool first)
{
  SetPID(track.GetPidHypo());
  SetTrackParam(first ? *track.GetParamFirst() : *track.GetParamLast());
  GetRefChi2() = track.GetChi2();
  GetRefNDF()  = track.GetNDF();
}

void CbmKFTrack::GetTrackParam(FairTrackParam& track) { CbmKFMath::CopyTC2TrackParam(&track, fT, fC); }

void CbmKFTrack::GetStsTrack(CbmStsTrack& track, bool first)
{
  FairTrackParam par(first ? *track.GetParamFirst() : *track.GetParamLast());
  GetTrackParam(par);  //first? *track.GetParamFirst() : *track.GetParamLast() );
  first ? track.SetParamFirst(&par) : track.SetParamLast(&par);
  track.SetChiSq(GetRefChi2());
  track.SetNDF(GetRefNDF());
}

void CbmKFTrack::GetGlobalTrack(CbmGlobalTrack& track, bool first)
{
  FairTrackParam par(first ? *track.GetParamFirst() : *track.GetParamLast());
  GetTrackParam(par);  //first? *track.GetParamFirst() : *track.GetParamLast() );
  first ? track.SetParamFirst(&par) : track.SetParamLast(&par);
  track.SetChi2(GetRefChi2());
  track.SetNDF(GetRefNDF());
}

void CbmKFTrack::SetPID(Int_t pidHypo)
{
  TParticlePDG* particlePDG = TDatabasePDG::Instance()->GetParticle(pidHypo);
  fMass                     = (particlePDG) ? particlePDG->Mass() : 0.13957;
  fIsElectron               = (TMath::Abs(pidHypo) == 11);
}
