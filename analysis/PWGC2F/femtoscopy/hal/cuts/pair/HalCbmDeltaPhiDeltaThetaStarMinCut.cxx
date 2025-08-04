/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmDeltaPhiDeltaThetaStarMinCut.h"

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


Bool_t HalCbmDeltaPhiDeltaThetaStarMinCut::PassHbt(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());

  Double_t phiMin = 1E+9;
  Double_t etaMin = 1E+9;
  for (int i = 0; i < 8; i++) {
    mom1          = tr1->GetMomAtPlane(i + 1);
    mom2          = tr2->GetMomAtPlane(i + 1);
    Double_t dphi = TVector2::Phi_mpi_pi(mom1.Phi() - mom2.Phi());
    if (TMath::Abs(dphi) < TMath::Abs(phiMin)) phiMin = dphi;
    etaMin = TMath::Min(TMath::Abs(mom1.Eta() - mom2.Eta()), etaMin);
  }

  SetValue(mom1.Phi() - mom2.Phi(), 0);  //dleta phi
  SetValue(mom1.Theta() - mom2.Theta(), 1);
  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

Bool_t HalCbmDeltaPhiDeltaThetaStarMinCut::PassAnaTree(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmTrack* tr1    = static_cast<HalCbmTrack*>(pair->GetTrack1());
  HalCbmTrack* tr2    = static_cast<HalCbmTrack*>(pair->GetTrack2());
  CbmHelix& h1        = tr1->GetHelix();
  CbmHelix& h2        = tr2->GetHelix();
  Double_t phiMin     = 1E+9;
  Double_t etaMin     = 1E+9;
  const Double_t R[8] = {30, 40, 50, 60, 70, 80, 90, 100};
  for (int i = 0; i < 8; i++) {
    h1.Eval(R[i] + tr1->GetEvent()->GetVertex()->Z(), mom1);
    h2.Eval(R[i] + tr2->GetEvent()->GetVertex()->Z(), mom2);
    Double_t dphi = TVector2::Phi_mpi_pi(mom1.Phi() - mom2.Phi());
    if (TMath::Abs(dphi) < TMath::Abs(phiMin)) phiMin = dphi;
    etaMin = TMath::Min(TMath::Abs(mom1.Eta() - mom2.Eta()), etaMin);
  }

  SetValue(phiMin, 0);  //dleta phi
  SetValue(etaMin, 1);


  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

HalCbmDeltaPhiDeltaThetaStarMinCut::HalCbmDeltaPhiDeltaThetaStarMinCut() : HalCbmPairCut(2)
{
  SetUnitName("#Delta#phi^{*}_{min}", 0);
  SetUnitName("#Delta#eta^{*}_{min}", 1);
  SetMinMax(0, 0, 0);
  SetMinMax(0, 0, 1);
}
