/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmHbtTrack.h"

#include "HalCbmHbtEvent.h"
#include "HalCbmTrack.h"

#include <Hal/Event.h>


HalCbmHbtTrack::HalCbmHbtTrack() : Hal::ExpTrack(), fHelix(CbmHelix()), fR(-1) {}

HalCbmHbtTrack::~HalCbmHbtTrack() {}

HalCbmHbtTrack::HalCbmHbtTrack(const HalCbmHbtTrack& other) : Hal::ExpTrack(other)
{
  fHelix = other.fHelix;
  fR     = other.fR;
  for (int i = 0; i < 9; i++) {
    fPosAt[i] = other.fPosAt[i];
    fMomAt[i] = other.fMomAt[i];
  }
  fPosAtCustom = other.fPosAtCustom;
  fMomAtCustom = other.fMomAtCustom;
}

HalCbmHbtTrack& HalCbmHbtTrack::operator=(const HalCbmHbtTrack& other)
{
  if (this == &other) return *this;
  Hal::ExpTrack::operator=(other);
  fHelix                 = other.fHelix;
  fR                     = other.fR;
  for (int i = 0; i < 9; i++) {
    fPosAt[i] = other.fPosAt[i];
    fMomAt[i] = other.fMomAt[i];
  }
  fPosAtCustom = other.fPosAtCustom;
  fMomAtCustom = other.fMomAtCustom;
  return *this;
}

void HalCbmHbtTrack::CopyData(Hal::Track* other)
{
  Hal::ExpTrack::CopyData(other);
  fHelix              = ((HalCbmTrack*) other)->GetHelix();
  Double_t x          = GetEvent()->GetVertex()->X();
  Double_t y          = GetEvent()->GetVertex()->Y();
  Double_t z          = GetEvent()->GetVertex()->Z();
  const Double_t R[9] = {20, 30, 40, 50, 60, 70, 80, 90, 100};
  for (int i = 0; i < 9; i++) {
    fPosAt[i] = fHelix.Eval(R[i] + z, fMomAt[i]);
    fPosAt[i] -= TVector3(x, y, z);
  }
  fR = -1;
}

void HalCbmHbtTrack::CalculateAtR(Double_t R)
{
  if (fR == R) return;
  Double_t x   = GetEvent()->GetVertex()->X();
  Double_t y   = GetEvent()->GetVertex()->Y();
  Double_t z   = GetEvent()->GetVertex()->Z();
  fPosAtCustom = GetHelix().Eval(z + R, fMomAtCustom);
  fPosAtCustom -= TVector3(x, y, z);
  fR = R;
}
