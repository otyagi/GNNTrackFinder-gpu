/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmTofM2PolyCut.h"

#include "HalCbmDetectorID.h"

#include <Hal/Cut.h>
#include <Hal/ExpTrack.h>
#include <Hal/Track.h>


HalCbmTofM2PolyCut::HalCbmTofM2PolyCut() {}

Bool_t HalCbmTofM2PolyCut::Pass(Hal::Track* track)
{
  Hal::ToFTrack* tof = (Hal::ToFTrack*) static_cast<Hal::ExpTrack*>(track)->GetDetTrack(HalCbm::DetectorID::kTOF);
  if (tof->GetFlag() == 0) {
    SetValue(Hal::ToFTrack::DummyVal());
  }
  SetValue(tof->GetMass2());
  return Validate();
}

HalCbmTofM2PolyCut::~HalCbmTofM2PolyCut() {}
