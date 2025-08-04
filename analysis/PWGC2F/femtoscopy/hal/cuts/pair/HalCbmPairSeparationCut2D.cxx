/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmPairSeparationCut2D.h"

#include "CbmHelix.h"
#include "HalCbmHbtTrack.h"
#include "HalCbmPairCut.h"
#include "HalCbmTrack.h"

#include <RtypesCore.h>
#include <TLorentzVector.h>
#include <TVector3.h>

#include <Hal/Event.h>
#include <Hal/Package.h>
#include <Hal/Parameter.h>
#include <Hal/Track.h>
#include <Hal/TwoTrack.h>


HalCbmPairSeparationCut2D::HalCbmPairSeparationCut2D() : HalCbmPairCut(2), fR(-10)
{
  SetUnitName("#Delta Z [cm[", 0);
  SetUnitName("#Delta XY [cm]", 1);
}

Bool_t HalCbmPairSeparationCut2D::PassHbt(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());
  tr1->CalculateAtR(fR);
  tr2->CalculateAtR(fR);
  mom1         = tr1->GetMomAtCustom();
  mom2         = tr2->GetMomAtCustom();
  Double_t dxy = (mom1 - mom2).Pt();
  if (mom1.Y() < mom2.Y()) dxy = -dxy;
  Double_t dz = mom1.Z() - mom2.Z();
  SetValue(dz, 0);
  SetValue(dxy, 1);
  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

Bool_t HalCbmPairSeparationCut2D::PassAnaTree(Hal::TwoTrack* pair)
{
  TVector3 mom1, mom2;
  HalCbmTrack* tr1 = static_cast<HalCbmTrack*>(pair->GetTrack1());
  HalCbmTrack* tr2 = static_cast<HalCbmTrack*>(pair->GetTrack2());
  CbmHelix& h1     = tr1->GetHelix();
  CbmHelix& h2     = tr2->GetHelix();
  h1.Eval(fR + tr1->GetEvent()->GetVertex()->Z(), mom1);
  h2.Eval(fR + tr2->GetEvent()->GetVertex()->Z(), mom2);
  Double_t dxy = (mom1 - mom2).Pt();
  if (mom1.Y() < mom2.Y()) dxy = -dxy;
  Double_t dz = mom1.Z() - mom2.Z();
  SetValue(dz, 0);
  SetValue(dxy, 1);
  if (GetValue(0) > GetMin(0) && GetValue(0) < GetMax(0) && GetValue(1) > GetMin(1) && GetValue(1) < GetMax(1)) {
    return ForcedUpdate(kFALSE);
  }
  return ForcedUpdate(kTRUE);
}

Hal::Package* HalCbmPairSeparationCut2D::Report() const
{
  Hal::Package* pack = HalCbmPairCut::Report();
  pack->AddObject(new Hal::ParameterDouble("R", fR));
  return pack;
}

HalCbmPairSeparationCut2D::~HalCbmPairSeparationCut2D()
{
  // TODO Auto-generated destructor stub
}
