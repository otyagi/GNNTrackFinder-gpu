/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Hitfind.cxx
/// \brief  A BMON hitfinder steering class (implementation)
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>


#include "Hitfind.h"

#include "AlgoFairloggerCompat.h"
#include "compat/OpenMP.h"
#include "util/TimingsFormat.h"

#include <chrono>

using cbm::algo::bmon::ClusterizerPars;
using cbm::algo::bmon::Hitfind;
using cbm::algo::bmon::HitfindSetup;
using fles::Subsystem;

// ---------------------------------------------------------------------------------------------------------------------
//
Hitfind::Hitfind(HitfindSetup setup, uint32_t nThreads) : fNofThreads(nThreads)
{
  // Create one algorithm per diamond and per thread
  size_t nDiamondsInSetup{setup.diamonds.size()};
  if (nDiamondsInSetup == 0) {
    throw std::runtime_error("No diamonds found in the BMON calibration config");
  }
  if (!(setup.selectionMask) != (nDiamondsInSetup == 1)) {
    throw std::runtime_error("Wrong diamond selection mask: for a single diamond it must be zero, and for multiple"
                             " diamonds it must be non-zero");
  }

  if (nDiamondsInSetup > 1) {
    // Define the selection bit offset
    while (!((setup.selectionMask >> fSelectionBitsOffset) % 2)) {
      ++fSelectionBitsOffset;
    }

    // Sort the diamonds in the setup by their SM or Side or other distinguishing index
    std::sort(setup.diamonds.begin(), setup.diamonds.end(), [&](const auto& lhs, const auto& rhs) {
      return GetDiamondIndex(lhs.refAddress) < GetDiamondIndex(rhs.refAddress);
    });
  }

  fSelectionBitmask = setup.selectionMask;
  fDiamondAddress   = PODVector<uint32_t>(nDiamondsInSetup, 0);

  // Store diamond address
  for (size_t iDiamond = 0; iDiamond < nDiamondsInSetup; ++iDiamond) {
    fDiamondAddress[iDiamond] = setup.diamonds[iDiamond].refAddress & ~CbmTofAddress::GetChannelIdBitmask();
  }

  // Create and configure clusterizer algorithms per thread and per diamond
  fAlgo = std::vector<std::vector<Clusterizer>>(fNofThreads, std::vector<Clusterizer>());
  for (auto& algoPerThread : fAlgo) {
    algoPerThread.reserve(nDiamondsInSetup);
    for (size_t iDiamond = 0; iDiamond < nDiamondsInSetup; ++iDiamond) {
      auto par               = std::make_unique<ClusterizerPars>();
      const auto& diamondPar = setup.diamonds[iDiamond];
      par->fAddress          = diamondPar.refAddress;
      par->fDeadStrips       = diamondPar.deadStrips;
      par->fdMaxTimeDist     = diamondPar.maxTimeDist;
      par->fTimeRes          = diamondPar.timeRes;
      algoPerThread.emplace_back(std::move(*par));
    }
  }
  L_(info) << "--- Configured hitfinder algorithms for BMON.";
}

// ---------------------------------------------------------------------------------------------------------------------
//
Hitfind::Output_t Hitfind::operator()(gsl::span<CbmBmonDigi> digisIn, uint32_t iThread)
{
  Output_t res     = {};
  auto& resHits    = std::get<0>(res);
  auto& resMoni    = std::get<1>(res);
  auto& resDigiIds = std::get<2>(res);

  auto& algoPerThread = fAlgo[iThread];

  // Distribute digis over diamonds, apply cuts on this level (maybe the Calibrator is a more proper place for it)
  size_t nDiamonds  = algoPerThread.size();
  auto vDigiStorage = std::vector<Clusterizer::Input_t>(nDiamonds, Clusterizer::Input_t(0));

  for (int32_t iDigi = 0; iDigi < static_cast<int32_t>(digisIn.size()); ++iDigi) {
    const auto& digi = digisIn[iDigi];
    size_t iDiamond  = GetDiamondIndex(digi.GetAddress());
    if (algoPerThread[iDiamond].SelectDigi(digi)) {
      vDigiStorage[iDiamond].emplace_back(digi, iDigi);
    }
  }

  // NOTE: I see no sense in storing a full channel address for different BMON hits,
  //       so for each hit the diamond address will be assigned: address & ~CbmTofAddress::GetChannelIdBitmask()
  PODVector<Hit> vHitsFlat;                                    // storage for clusters
  PODVector<size_t> vNhitsPerDiamond(fDiamondAddress.size());  // number of hits per diamond

  for (size_t iDiamond = 0; iDiamond < algoPerThread.size(); ++iDiamond) {
    auto [hits, digiIds]       = algoPerThread[iDiamond](vDigiStorage[iDiamond]);
    vNhitsPerDiamond[iDiamond] = hits.size();
    vHitsFlat.insert(vHitsFlat.end(), std::make_move_iterator(hits.begin()), std::make_move_iterator(hits.end()));
    resDigiIds.insert(resDigiIds.end(), std::make_move_iterator(digiIds.begin()),
                      std::make_move_iterator(digiIds.end()));
  }

  resHits = PartitionedVector(std::move(vHitsFlat), vNhitsPerDiamond, fDiamondAddress);
  return res;
}
