/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Calibrate.h
/// \brief  Calibrator for the BMON digis (implementation)
/// \since  04.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "Calibrate.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTofAddress.h"
#include "util/TimingsFormat.h"

#include <bitset>
#include <chrono>

using cbm::algo::bmon::Calibrate;
using cbm::algo::bmon::CalibrateSetup;
using fles::Subsystem;

// ---------------------------------------------------------------------------------------------------------------------
//
Calibrate::Calibrate(CalibrateSetup setup) : fSetup(setup), fSelectionBitsOffset(0)
{
  // Initialize offset arrays for channel deadtime check
  int nDiamondsInSetup = fSetup.diamonds.size();
  if (nDiamondsInSetup == 0) {
    throw std::runtime_error("No diamonds found in the BMON calibration config");
  }
  if (!(fSetup.selectionMask) != (nDiamondsInSetup == 1)) {
    throw std::runtime_error("Wrong diamond selection mask: for a single diamond it must be zero, and for multiple"
                             " diamonds it must be non-zero");
  }
  if (nDiamondsInSetup > 1) {
    // Define the selection bit offset
    while (!((fSetup.selectionMask >> fSelectionBitsOffset) % 2)) {
      ++fSelectionBitsOffset;
    }

    // Sort the diamonds in the setup by their SM or Side or other distinguishing index
    std::sort(fSetup.diamonds.begin(), fSetup.diamonds.end(), [&](const auto& lhs, const auto& rhs) {
      return GetDiamondIndex(lhs.refAddress) < GetDiamondIndex(rhs.refAddress);
    });
  }

  // Remove the channel information from the reference address
  for (auto& diamond : fSetup.diamonds) {
    diamond.refAddress &= ~CbmTofAddress::GetChannelIdBitmask();
  }

  // Calculate the channel offsets, needed for the dead time
  fChannelOffset = std::vector<size_t>(nDiamondsInSetup + 1, 0);  // the last element -- total number of channels
  for (int32_t iD = 0; iD < nDiamondsInSetup; ++iD) {
    fChannelOffset[iD + 1] = fChannelOffset[iD] + fSetup.diamonds[iD].chanPar.size();
  }
  fChannelDeadTime = std::vector<double>(fChannelOffset.back(), std::numeric_limits<double>::quiet_NaN());

  // **** DEBUG: END
}

// ---------------------------------------------------------------------------------------------------------------------
//
Calibrate::resultType Calibrate::operator()(gsl::span<const CbmBmonDigi> digiIn)
{
  xpu::push_timer("BmonCalibrate");
  xpu::t_add_bytes(digiIn.size_bytes());

  // --- Output data
  resultType result = {};

  auto& calDigiOut = std::get<0>(result);
  auto& monitor    = std::get<1>(result);
  calDigiOut.reserve(digiIn.size());

  // Reset the channel dead time
  std::fill(fChannelDeadTime.begin(), fChannelDeadTime.end(), std::numeric_limits<double>::quiet_NaN());

  const auto& diamonds = fSetup.diamonds;
  for (const auto& digi : digiIn) {
    uint32_t address = static_cast<uint32_t>(digi.GetAddress());
    int32_t iChannel = CbmTofAddress::GetChannelId(address);
    size_t iDiamond  = GetDiamondIndex(address);
    if (iDiamond >= diamonds.size()
        || (address & ~CbmTofAddress::GetChannelIdBitmask()) != diamonds[iDiamond].refAddress) {
      monitor.fDigiCalibUnknownRPC++;
      L_(error) << "Unknown BMON digi address: " << CbmTofAddress::ToString(address) << ", iDiamond = " << iDiamond;
      continue;
    }

    const auto& diamondPar = diamonds[iDiamond];
    const auto& channelPar = diamondPar.chanPar[iChannel];

    // Check dead time
    const size_t iChannelGlb = fChannelOffset[iDiamond] + iChannel;
    const double deadTime    = fChannelDeadTime[iChannelGlb];

    if (!std::isnan(deadTime) && digi.GetTime() <= deadTime) {
      fChannelDeadTime[iChannelGlb] = digi.GetTime() + diamondPar.channelDeadtime;
      monitor.fDigiDeadTimeCount++;
      continue;
    }
    fChannelDeadTime[iChannelGlb] = digi.GetTime() + diamondPar.channelDeadtime;

    // Create calibrated digi
    CbmBmonDigi& pCalDigi = calDigiOut.emplace_back(digi);
    pCalDigi.SetAddress(address);

    // calibrate Digi Time
    pCalDigi.SetTime(pCalDigi.GetTime() - channelPar.vCPTOff);

    // subtract Offset
    const double dTot = std::max(pCalDigi.GetCharge() - channelPar.vCPTotOff, 0.001);

    // calibrate Digi charge (corresponds to TOF ToT)
    pCalDigi.SetCharge(dTot * channelPar.vCPTotGain);

    // walk correction
    const std::vector<double>& walk = channelPar.vCPWalk;
    const double dTotBinSize        = (diamondPar.TOTMax - diamondPar.TOTMin) / diamondPar.numClWalkBinX;
    int32_t iWx                     = std::max((int32_t)((pCalDigi.GetCharge() - diamondPar.TOTMin) / dTotBinSize), 0);
    iWx                             = std::min(iWx, diamondPar.numClWalkBinX - 1);

    const double dDTot = (pCalDigi.GetCharge() - diamondPar.TOTMin) / dTotBinSize - (double) iWx - 0.5;
    double dWT         = walk[iWx];

    // linear interpolation to next bin
    if (dDTot > 0) {
      if (iWx < diamondPar.numClWalkBinX - 1) {
        dWT += dDTot * (walk[iWx + 1] - walk[iWx]);
      }
    }
    else {
      if (0 < iWx) {
        dWT -= dDTot * (walk[iWx - 1] - walk[iWx]);
      }
    }
    pCalDigi.SetTime(pCalDigi.GetTime() - dWT);  // calibrate Digi Time
  }

  /// Sort the buffers of hits due to the time offsets applied
  // (insert-sort faster than std::sort due to pre-sorting)
  for (std::size_t i = 1; i < calDigiOut.size(); ++i) {
    CbmTofDigi digi = calDigiOut[i];
    std::size_t j   = i;
    while (j > 0 && calDigiOut[j - 1].GetTime() > digi.GetTime()) {
      calDigiOut[j] = calDigiOut[j - 1];
      --j;
    }
    calDigiOut[j] = digi;
  }

  //  Kept for possible unsorted input
  //  std::sort(calDigiOut.begin(), calDigiOut.end(),
  //  [](const CbmTofDigi& a, const CbmTofDigi& b) -> bool { return a.GetTime() < b.GetTime(); });

  monitor.fTime     = xpu::pop_timer();
  monitor.fNumDigis = digiIn.size();
  return result;
}
