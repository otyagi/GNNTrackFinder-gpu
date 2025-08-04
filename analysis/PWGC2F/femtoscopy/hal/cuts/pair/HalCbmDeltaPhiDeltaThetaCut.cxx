/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmDeltaPhiDeltaThetaCut.h"

#include "CbmHelix.h"
#include "HalCbmHbtTrack.h"
#include "HalCbmTrack.h"

#include <TLorentzVector.h>
#include <TVector3.h>

#include <Hal/Cut.h>
#include <Hal/Event.h>
#include <Hal/Package.h>
#include <Hal/Parameter.h>
#include <Hal/Std.h>
#include <Hal/Track.h>
#include <Hal/TwoTrack.h>

HalCbmDeltaPhiDeltaThetaStarCut::HalCbmDeltaPhiDeltaThetaStarCut() : HalCbmPairCut(2), fR(0.0)
{
  SetUnitName("#Delta#phi^{*} [rad]", 0);
  SetUnitName("#Delta#eta^{*} []", 1);
}

Bool_t HalCbmDeltaPhiDeltaThetaStarCut::PassHbt(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());
  tr1->CalculateAtR(fR);
  tr2->CalculateAtR(fR);
  mom1 = tr1->GetMomAtCustom();
  mom2 = tr2->GetMomAtCustom();
  SetValue(mom1.Phi() - mom2.Phi(), 0);  //dleta phi
  SetValue(mom1.Theta() - mom2.Theta(), 1);
  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

Bool_t HalCbmDeltaPhiDeltaThetaStarCut::PassAnaTree(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmTrack* tr1 = static_cast<HalCbmTrack*>(pair->GetTrack1());
  HalCbmTrack* tr2 = static_cast<HalCbmTrack*>(pair->GetTrack2());
  CbmHelix& h1     = tr1->GetHelix();
  CbmHelix& h2     = tr2->GetHelix();
  h1.Eval(fR + tr1->GetEvent()->GetVertex()->Z(), mom1);
  h2.Eval(fR + tr2->GetEvent()->GetVertex()->Z(), mom2);
  SetValue(mom1.Phi() - mom2.Phi(), 0);  //dleta phi
  SetValue(mom1.Theta() - mom2.Theta(), 1);
  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

//===============================================================================
Hal::Package* HalCbmDeltaPhiDeltaThetaStarCut::Report() const
{
  Hal::Package* pack = HalCbmPairCut::Report();
  pack->AddObject(new Hal::ParameterDouble("R", fR));
  return pack;
}

HalCbmDeltaPhiDeltaThetaStarCut&
HalCbmDeltaPhiDeltaThetaStarCut::operator=(const HalCbmDeltaPhiDeltaThetaStarCut& other)
{
  if (this == &other) return *this;
  HalCbmPairCut::operator=(other);
  fDataType              = other.fDataType;
  return *this;
}

HalCbmDeltaPhiDeltaThetaStarCut::~HalCbmDeltaPhiDeltaThetaStarCut() {}

HalCbmDeltaPhiDeltaThetaCut::HalCbmDeltaPhiDeltaThetaCut() : HalCbmPairCut(2)
{
  SetUnitName("#Delta#phi [rad]", 0);
  SetUnitName("#Delta#eta [rad]", 1);
}

Bool_t HalCbmDeltaPhiDeltaThetaCut::Pass(Hal::TwoTrack* pair)
{
  Hal::Track* tr1 = static_cast<Hal::Track*>(pair->GetTrack1());
  Hal::Track* tr2 = static_cast<Hal::Track*>(pair->GetTrack2());
  SetValue(tr1->GetMomentum().Phi() - tr2->GetMomentum().Phi(), 0);
  SetValue(tr1->GetMomentum().Theta() - tr2->GetMomentum().Theta(), 1);
  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

HalCbmDeltaPhiDeltaThetaCut::~HalCbmDeltaPhiDeltaThetaCut() {}

//===============================================================================

HalCbmDeltaPhiDeltaThetaStarCutLayers::HalCbmDeltaPhiDeltaThetaStarCutLayers() : HalCbmPairCut(22), fR(0.0)
{
  for (int i = 0; i < 9; i++) {
    SetUnitName(Form("#Delta#phi^{*}_{lay%i} [rad]", i), i * 2);
    SetUnitName(Form("#Delta#eta^{*}_{lay%i} [AU]", i), i * 2 + 1);
    SetMinMax(1, -1, i * 2);
    SetMinMax(1, -1, i * 2 + 1);
  }
  SetUnitName("#Delta#phi_{min}^{*} [rad]", 18);
  SetUnitName("#Delta#eta_{min}^{*} [AU]", 19);
  SetUnitName("#Delta#phi_{max}^{*} [rad]", 20);
  SetUnitName("#Delta#eta_{max}^{*} [AU]", 21);
  for (int i = 18; i < 22; i++) {
    SetMinMax(1, -1, i);
  }
}

Bool_t HalCbmDeltaPhiDeltaThetaStarCutLayers::PassHbt(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  TVector3 pos1, pos2;
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());
  Double_t phiMin = 9E+9, etaMin = 9e+9;
  Double_t phiMax = 0, etaMax = 0;

  for (int i = 0; i < 9; i++) {
    mom1          = tr1->GetMomAtPlane(i);
    mom2          = tr2->GetMomAtPlane(i);
    pos1          = tr1->GetPosAtPlane(i);
    pos2          = tr2->GetPosAtPlane(i);
    Double_t dphi = TVector2::Phi_mpi_pi(mom1.Phi() - mom2.Phi());
    Double_t deta = mom1.Theta() - mom2.Theta();

    SetValue(dphi, 2 * i);
    SetValue(deta, 2 * i + 1);
    if (TMath::Abs(dphi) < TMath::Abs(phiMin)) phiMin = dphi;
    if (TMath::Abs(deta) < TMath::Abs(etaMin)) etaMin = deta;

    if (TMath::Abs(dphi) > TMath::Abs(phiMax)) phiMax = dphi;
    if (TMath::Abs(deta) > TMath::Abs(etaMax)) etaMax = deta;
  }
  SetValue(phiMin, 18);
  SetValue(etaMin, 19);
  SetValue(phiMax, 20);
  SetValue(etaMax, 21);
  Int_t passed = 0;
  for (int i = 0; i < 22; i++) {
    if (GetValue(i) > GetMin(i) && GetValue(i) < GetMax(i)) passed++;
  }
  if (passed != 0) return ForcedUpdate(kFALSE);
  return ForcedUpdate(kTRUE);
}

Bool_t HalCbmDeltaPhiDeltaThetaStarCutLayers::PassAnaTree(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmTrack* tr1 = static_cast<HalCbmTrack*>(pair->GetTrack1());
  HalCbmTrack* tr2 = static_cast<HalCbmTrack*>(pair->GetTrack2());
  CbmHelix& h1     = tr1->GetHelix();
  CbmHelix& h2     = tr2->GetHelix();
  Double_t phiMin = 9E+9, etaMin = 9e+9;
  Double_t phiMax = 0, etaMax = 0;
  const Double_t R[9] = {20, 30, 40, 50, 60, 70, 80, 90, 100};

  for (int i = 0; i < 9; i++) {
    h1.Eval(R[i] + tr1->GetEvent()->GetVertex()->Z(), mom1);
    h2.Eval(R[i] + tr2->GetEvent()->GetVertex()->Z(), mom2);
    Double_t dphi = TVector2::Phi_mpi_pi(mom1.Phi() - mom2.Phi());
    Double_t deta = mom1.Theta() - mom2.Theta();

    SetValue(dphi, 2 * i);
    SetValue(deta, 2 * i + 1);
    if (TMath::Abs(dphi) < TMath::Abs(phiMin)) phiMin = dphi;
    if (TMath::Abs(deta) < TMath::Abs(etaMin)) etaMin = deta;

    if (TMath::Abs(dphi) > TMath::Abs(phiMax)) phiMax = dphi;
    if (TMath::Abs(deta) > TMath::Abs(etaMax)) etaMax = deta;
  }
  SetValue(phiMin, 18);
  SetValue(etaMin, 19);
  SetValue(phiMax, 20);
  SetValue(etaMax, 21);
  Int_t passed = 0;
  for (int i = 0; i < 22; i++) {
    if (GetValue(i) > GetMin(i) && GetValue(i) < GetMax(i)) passed++;
  }
  if (passed != 0) return ForcedUpdate(kFALSE);
  return ForcedUpdate(kTRUE);
}


Hal::Package* HalCbmDeltaPhiDeltaThetaStarCutLayers::Report() const
{
  Hal::Package* pack = HalCbmPairCut::Report();
  pack->AddObject(new Hal::ParameterDouble("R", fR));
  return pack;
}

HalCbmDeltaPhiDeltaThetaStarCutLayers&
HalCbmDeltaPhiDeltaThetaStarCutLayers::operator=(const HalCbmDeltaPhiDeltaThetaStarCutLayers& other)
{
  if (this == &other) return *this;
  HalCbmPairCut::operator=(other);
  fDataType              = other.fDataType;
  return *this;
}

HalCbmDeltaPhiDeltaThetaStarCutLayers::~HalCbmDeltaPhiDeltaThetaStarCutLayers() {}

//===============================================================================
