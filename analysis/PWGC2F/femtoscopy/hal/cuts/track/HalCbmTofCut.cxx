/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmTofCut.h"

#include "Hal/Std.h"
#include "HalCbmTrack.h"

HalCbmTofCut::HalCbmTofCut() : Hal::TrackToFMass2Cut(2) { SetUnitName("Flag [AU]", 1); }

Bool_t HalCbmTofCut::Init(Int_t task_id)
{
  if (FormatInhertis("HalCbmEvent", task_id, Hal::EFormatDepth::kNonBuffered)) return kTRUE;
  return kFALSE;
}

Bool_t HalCbmTofCut::Pass(Hal::Track* track)
{
  HalCbmTrack* reco  = (HalCbmTrack*) track;
  Hal::ToFTrack* tof = reco->GetTofTrack();
  SetValue(tof->GetFlag(), Flag());
  SetValue(tof->GetMass2(), M2());
  Double_t m2 = tof->GetMass2();
  Double_t p  = reco->GetMomentum().P();
  SetValue(m2, 0);
  SetValue(tof->GetFlag(), 1);
  Double_t min = fLowLine[0] + fLowLine[1] * p + fLowLine[2] * p * p;
  Double_t max = fHighLine[0] + fHighLine[1] * p + fHighLine[2] * p * p;
  if (m2 > max || m2 < min) {
    return ForcedUpdate(kFALSE);
  }
  return Validate();
}

HalCbmTofCut::~HalCbmTofCut() {}
