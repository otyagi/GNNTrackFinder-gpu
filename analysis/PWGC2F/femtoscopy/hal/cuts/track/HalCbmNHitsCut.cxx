/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmNHitsCut.h"

#include "HalCbmTrack.h"

HalCbmNHitsCut::HalCbmNHitsCut() : Hal::TrackExpCut(4)
{
  SetUnitName("N_{hits}", 0);
  SetUnitName("N_{hits MVD}", 1);
  SetUnitName("N_{hits STS}", 2);
  SetUnitName("N_{hits TRD}", 3);
}

Bool_t HalCbmNHitsCut::Init(Int_t task_id) { return FormatInhertis("HalCbmEvent", task_id); }

Bool_t HalCbmNHitsCut::Pass(Hal::Track* track)
{
  HalCbmTrack* tr = (HalCbmTrack*) track;
  SetValue(tr->GetNHits(), 0);
  SetValue(tr->GetNMvdHits(), 1);
  SetValue(tr->GetNStsHits(), 2);
  SetValue(tr->GetNTrdHits(), 3);
  return Validate();
}

HalCbmNHitsCut::~HalCbmNHitsCut() {}
