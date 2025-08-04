/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmPairCut.h"


HalCbmPairCut::HalCbmPairCut(Int_t pairNo) : Hal::TwoTrackCut(pairNo), fDataType(HalCbm::EFormatType::kUnknown) {}

HalCbmPairCut::~HalCbmPairCut() {}

HalCbmPairCut& HalCbmPairCut::operator=(const HalCbmPairCut& other)
{
  if (this == &other) return *this;
  Hal::TwoTrackCut::operator=(other);
  fDataType                 = other.fDataType;
  return *this;
}

Bool_t HalCbmPairCut::Pass(Hal::TwoTrack* pair)
{
  switch (fDataType) {
    case HalCbm::EFormatType::kHbt: return PassHbt(pair); break;
    case HalCbm::EFormatType::kAnaTree: return PassAnaTree(pair); break;
    default: return kFALSE; break;
  }
}

Bool_t HalCbmPairCut::Init(Int_t format_id)
{
  fDataType = HalCbm::GetFormatType(format_id, Hal::EFormatDepth::kBuffered);
  if (fDataType == HalCbm::EFormatType::kUnknown) return kFALSE;
  return kTRUE;
}

HalCbmPairCut::HalCbmPairCut(const HalCbmPairCut& other) : Hal::TwoTrackCut(other), fDataType(other.fDataType) {}
