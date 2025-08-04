/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmStsSepCut.h"

#include "CbmHelix.h"
#include "HalCbmHbtTrack.h"
#include "HalCbmTrack.h"

#include <TLorentzVector.h>
#include <TVector3.h>

#include <Hal/ExpEvent.h>
#include <Hal/TwoTrack.h>


HalCbmStsExitSepCut::HalCbmStsExitSepCut() : HalCbmPairCut(1)
{
  SetUnitName("StsExitSep [cm]");
  SetMinMax(0, 1E+5);
}

HalCbmStsExitSepCut::~HalCbmStsExitSepCut() {}

Bool_t HalCbmStsExitSepCut::PassHbt(Hal::TwoTrack* pair)
{
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());
  Double_t dist       = (tr1->GetPosAtStsExit() - tr2->GetPosAtStsExit()).Mag();
  SetValue(dist);
  return Validate();
}

Bool_t HalCbmStsExitSepCut::PassAnaTree(Hal::TwoTrack* pair)
{
  Double_t dist        = 0;
  TLorentzVector *Pos1 = nullptr, *Pos2 = nullptr;
  HalCbmTrack* tr1 = static_cast<HalCbmTrack*>(pair->GetTrack1());
  HalCbmTrack* tr2 = static_cast<HalCbmTrack*>(pair->GetTrack2());
  Pos1             = static_cast<Hal::ExpEvent*>(tr1->GetEvent())->GetVertex();
  Pos2             = static_cast<Hal::ExpEvent*>(tr2->GetEvent())->GetVertex();
  CbmHelix& h1     = tr1->GetHelix();
  CbmHelix& h2     = tr2->GetHelix();
  TVector3 pos1    = h1.Eval(101.1 + Pos1->Z());
  TVector3 pos2    = h2.Eval(101.1 + Pos2->Z());
  pos1.SetX(pos1.X() - Pos1->X());
  pos1.SetY(pos1.Y() - Pos1->Y());
  pos2.SetX(pos2.X() - Pos2->X());
  pos2.SetY(pos2.Y() - Pos2->Y());
  dist = (pos1 - pos2).Mag();
  SetValue(dist);
  return Validate();
}

Bool_t HalCbmStsEntranceSepCut::PassHbt(Hal::TwoTrack* pair)
{
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());
  Double_t dist       = (tr1->GetPosAtStsEntrance() - tr2->GetPosAtStsEntrance()).Mag();
  SetValue(dist);
  return Validate();
}

Bool_t HalCbmStsEntranceSepCut::PassAnaTree(Hal::TwoTrack* pair)
{
  Double_t dist        = 0;
  TLorentzVector *Pos1 = nullptr, *Pos2 = nullptr;
  HalCbmTrack* tr1 = static_cast<HalCbmTrack*>(pair->GetTrack1());
  HalCbmTrack* tr2 = static_cast<HalCbmTrack*>(pair->GetTrack2());
  Pos1             = static_cast<Hal::ExpEvent*>(tr1->GetEvent())->GetVertex();
  Pos2             = static_cast<Hal::ExpEvent*>(tr2->GetEvent())->GetVertex();
  CbmHelix& h1     = tr1->GetHelix();
  CbmHelix& h2     = tr2->GetHelix();
  TVector3 pos1    = h1.Eval(30 + Pos1->Z());
  TVector3 pos2    = h2.Eval(30 + Pos2->Z());
  pos1.SetX(pos1.X() - Pos1->X());
  pos1.SetY(pos1.Y() - Pos1->Y());
  pos2.SetX(pos2.X() - Pos2->X());
  pos2.SetY(pos2.Y() - Pos2->Y());
  dist = (pos1 - pos2).Mag();
  SetValue(dist);
  return Validate();
}

HalCbmStsEntranceSepCut::HalCbmStsEntranceSepCut() : HalCbmPairCut(1)
{
  SetUnitName("StsEntranceSep [cm]");
  SetMinMax(0, 1E+5);
}

Bool_t HalCbmStsSeparationCutLayers::PassHbt(Hal::TwoTrack* pair)
{
  HalCbmHbtTrack* tr1 = static_cast<HalCbmHbtTrack*>(pair->GetTrack1());
  HalCbmHbtTrack* tr2 = static_cast<HalCbmHbtTrack*>(pair->GetTrack2());
  TVector3 poz1, poz2, dif;
  Double_t x_min = 9E+4, y_min = 9E+4, xy_min = 9E+4;
  Double_t x_max = 0, y_max = 0, xy_max = 0;
  for (int i = 0; i < 9; i++) {
    poz1          = tr1->GetPosAtPlane(i);
    poz2          = tr2->GetPosAtPlane(i);
    dif           = poz1 - poz2;
    Double_t dx   = dif.X();
    Double_t dy   = dif.Y();
    Double_t adx  = TMath::Abs(dif.X());
    Double_t ady  = TMath::Abs(dif.Y());
    Double_t adxy = dif.Pt();
    SetValue(dx, i * 3);
    SetValue(dy, i * 3 + 1);
    SetValue(adxy, i * 3 + 2);
    if (adx < x_min) x_min = dx;
    if (ady < y_min) y_min = dy;
    if (adxy < xy_min) xy_min = adxy;
    if (adx > x_max) x_max = dx;
    if (ady > y_max) y_max = dy;
    if (adxy > xy_max) xy_max = adxy;
  }
  SetValue(x_min, 27);
  SetValue(y_min, 28);
  SetValue(xy_min, 29);
  SetValue(x_max, 30);
  SetValue(y_max, 31);
  SetValue(xy_max, 32);

  Int_t passed = 0;
  for (int i = 0; i < 33; i++) {
    if (GetValue(i) > GetMin(i) && GetValue(i) < GetMax(i)) passed++;
  }
  if (passed != 0) return ForcedUpdate(kFALSE);
  return ForcedUpdate(kTRUE);
}

Bool_t HalCbmStsSeparationCutLayers::PassAnaTree(Hal::TwoTrack* /*pair*/)
{
  LOG(warning) << "not implemented";
  return kTRUE;
}

HalCbmStsSeparationCutLayers::HalCbmStsSeparationCutLayers() : HalCbmPairCut(33)
{
  for (int i = 0; i < 9; i++) {
    SetUnitName(Form("#Delta#X^{*}_{lay%i} [cm]", i), i * 3);
    SetUnitName(Form("#Delta#Y^{*}_{lay%i} [cm]", i), i * 3 + 1);
    SetUnitName(Form("#Delta#XY^{*}_{lay%i} [cm]", i), i * 3 + 2);
    SetMinMax(1, -1, i * 3);
    SetMinMax(1, -1, i * 3 + 1);
    SetMinMax(1, -1, i * 3 + 2);
  }
  SetUnitName("#Delta#X^{*}_{min} [cm]", 27);
  SetUnitName("#Delta#Y^{*}_{min} [cm]", 28);
  SetUnitName("#Delta#XY^{*}_{min} [cm]", 29);
  SetUnitName("#Delta#X^{*}_{max} [cm]", 30);
  SetUnitName("#Delta#Y^{*}_{max} [cm]", 31);
  SetUnitName("#Delta#XY^{*}_{max} [cm]", 32);
  SetMinMax(1, -1, 27);
  SetMinMax(1, -1, 28);
  SetMinMax(1, -1, 29);
  SetMinMax(1, -1, 30);
  SetMinMax(1, -1, 31);
  SetMinMax(1, -1, 32);
}

HalCbmStsSeparationCutLayers::~HalCbmStsSeparationCutLayers() {}
