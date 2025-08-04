/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   BmonHitfindQaParameters.cxx
/// \brief  A BMON hitfinder QA parameter configuration (implementation)
/// \since  10.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "qa/hitfind/BmonHitfindQaParameters.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTofAddress.h"

using cbm::algo::bmon::CalibrateSetup;
using cbm::algo::bmon::HitfindQaParameters;
using cbm::algo::bmon::HitfindSetup;

// ---------------------------------------------------------------------------------------------------------------------
//
HitfindQaParameters::HitfindQaParameters(const CalibrateSetup& calSetup, const HitfindSetup& hitSetup)
{

  this->selectionMask = calSetup.selectionMask;
  if (this->selectionMask != hitSetup.selectionMask) {
    throw std::runtime_error("Mismatch of the selection bitmask in the BMON CalibrateSetup and HitfindSetup configs");
  }

  auto nDiamonds = calSetup.diamonds.size();
  if (nDiamonds != hitSetup.diamonds.size()) {
    throw std::runtime_error("Mismatch of number of diamonds in the BMON CalibrateSetup and HitfindSetup configs");
  }

  if (nDiamonds > 1) {
    while (!((this->selectionMask >> fSelectionBitsOffset) % 2)) {
      ++fSelectionBitsOffset;
    }
  }

  this->diamonds.resize(nDiamonds);
  for (const auto& calDiamond : calSetup.diamonds) {
    uint32_t address      = calDiamond.refAddress & ~CbmTofAddress::GetChannelIdBitmask();
    auto& thisDiamond     = this->diamonds[GetDiamondIndex(address)];
    thisDiamond.address   = address;
    thisDiamond.nChannels = calDiamond.chanPar.size();
  }

  for (const auto& hitDiamond : hitSetup.diamonds) {
    int32_t address   = hitDiamond.refAddress & ~CbmTofAddress::GetChannelIdBitmask();
    auto& thisDiamond = this->diamonds[GetDiamondIndex(address)];
    if (thisDiamond.address != address) {
      throw std::runtime_error("Mismatch between diamond addresses in BMON CalibrateSetup and HitfindSetup configs");
    }
    thisDiamond.deadStrips  = hitDiamond.deadStrips;
    thisDiamond.timeRes     = hitDiamond.timeRes;
    thisDiamond.maxTimeDist = hitDiamond.maxTimeDist;
  }
}
