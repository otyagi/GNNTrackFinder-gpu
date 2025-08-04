/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Clusterizer.cxx
/// \brief  A clusterizer algorithm for BMON (implementation)
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>


#include "bmon/Clusterizer.h"

#include "AlgoFairloggerCompat.h"
#include "CbmBmonDigi.h"
#include "CbmTofAddress.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

using cbm::algo::bmon::Clusterizer;

// ---------------------------------------------------------------------------------------------------------------------
//
Clusterizer::Output_t Clusterizer::operator()(const Clusterizer::Input_t& digis)
{
  // Description:
  // The input array of digis is traced through until the last element. If the current digi and the next digi are
  // close in time and have neighboring channels, a single hit is produced. Otherwise a hit is produced from a
  // single digi. The complexity of the algorithm is O(nDigis). Requirements: digis must be sorted in time
  Output_t res;
  if (digis.empty()) {
    return res;
  }

  auto& hits        = std::get<0>(res);
  auto& digiIndices = std::get<1>(res);
  hits.reserve(digis.size());
  digiIndices.reserve(digis.size());
  auto itLast = std::prev(digis.end());  // iterator pointing to the last digi
  bool bUsedWithPrevious{false};         // A flag: if the current digi was used together with the previous one
  for (auto it = digis.begin(), in = it; it != itLast; ++it) {
    if (bUsedWithPrevious) {
      // skip a digi, if it was already used
      bUsedWithPrevious = false;
      continue;
    }
    in                = std::next(it);
    const auto& digiT = it->first;
    const auto& digiN = in->first;
    if (digiN.GetTime() - digiT.GetTime() < fParams.fdMaxTimeDist
        && abs(digiN.GetChannel() - digiT.GetChannel()) == 1) {
      // A hit consisting from two digis is found
      hits.emplace_back(fParams.fAddress, digiT, digiN);
      digiIndices.emplace_back(it->second);
      bUsedWithPrevious = true;
    }
    else {
      // A hit consisting from a single digi
      hits.emplace_back(fParams.fAddress, digiT);
      digiIndices.emplace_back(it->second);
    }
  }
  if (!bUsedWithPrevious) {
    // Create a hit from the last digi
    hits.emplace_back(fParams.fAddress, itLast->first);
    digiIndices.emplace_back(itLast->second);
  }

  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Clusterizer::SelectDigi(const CbmBmonDigi& digi) const
{
  // Dead channel cut
  if (fParams.fDeadStrips & (1 << CbmTofAddress::GetChannelId(digi.GetAddress()))) {
    return false;
  }

  // ??? Other cuts ??? Charge threshold? Also dead strips can be accounted already on the calibrator level, together
  //     with the cuts

  return true;
}
