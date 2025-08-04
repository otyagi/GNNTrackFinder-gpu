/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaToolsMCData.cxx
/// \brief  Data structure for internal tracking MC-information (implementation)
/// \since  23.09.2022
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaToolsMCData.h"

#include "CbmL1Hit.h"

#include <iomanip>
#include <sstream>
#include <utility>  // for std::move

using cbm::ca::tools::MCData;

// ---------------------------------------------------------------------------------------------------------------------
//
MCData::MCData() {}

// ---------------------------------------------------------------------------------------------------------------------
//
MCData::MCData(const MCData& other)
  : fvPoints(other.fvPoints)
  , fvTracks(other.fvTracks)
  , fvNofPointsOrig(other.fvNofPointsOrig)
  , fvNofPointsUsed(other.fvNofPointsUsed)
  , fmPointLinkMap(other.fmPointLinkMap)
  , fmTrackLinkMap(other.fmTrackLinkMap)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
MCData::MCData(MCData&& other) noexcept { this->Swap(other); }

// ---------------------------------------------------------------------------------------------------------------------
//
MCData& MCData::operator=(const MCData& other)
{
  if (this != &other) {
    MCData(other).Swap(*this);
  }
  return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
//
MCData& MCData::operator=(MCData&& other) noexcept
{
  if (this != &other) {
    MCData tmp(std::move(other));
    this->Swap(tmp);
  }
  return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCData::Swap(MCData& other) noexcept
{
  std::swap(fvPoints, other.fvPoints);
  std::swap(fvTracks, other.fvTracks);
  std::swap(fvNofPointsOrig, other.fvNofPointsOrig);
  std::swap(fvNofPointsUsed, other.fvNofPointsUsed);
  std::swap(fmPointLinkMap, other.fmPointLinkMap);
  std::swap(fmTrackLinkMap, other.fmTrackLinkMap);
}


// ---------------------------------------------------------------------------------------------------------------------
//
void MCData::Clear()
{
  fvPoints.clear();
  fvTracks.clear();
  fmPointLinkMap.clear();
  fmTrackLinkMap.clear();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCData::InitTrackInfo(const ca::Vector<CbmL1HitDebugInfo>& vHits)
{
  for (auto& aTrk : fvTracks) {
    // Assign hits to tracks
    aTrk.ClearHitIndexes();
    auto& vHitIds = aTrk.GetHitIndexes();
    for (int iP : aTrk.GetPointIndexes()) {
      const auto& point = fvPoints[iP];
      for (int iH : point.GetHitIndexes()) {
        if (std::find(vHitIds.begin(), vHitIds.end(), iH) == vHitIds.end()) {
          aTrk.AddHitIndex(iH);
        }
      }
    }
    // Initialize arrangements of points and hits within stations
    aTrk.InitPointsInfo(fvPoints);
    aTrk.InitHitsInfo(vHits);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string MCData::ToString(int verbose) const
{
  if (verbose < 1) {
    return std::string();
  }
  std::stringstream msg;
  msg << "MCData: " << fvTracks.size() << " tracks, " << fvPoints.size() << " points, ";
  msg << fmTrackLinkMap.size() << " track links, " << fmPointLinkMap.size() << " point links";
  if (verbose > 1) {
    using std::setfill;
    using std::setw;
    constexpr int kMaxLines = 100;
    int nTracks             = std::min(kMaxLines, GetNofTracks());
    int nPoints             = std::min(kMaxLines, GetNofPoints());
    msg << "\n Track sample (first " << nTracks << " tracks):";
    msg << '\n' << setw(10) << setfill(' ') << fvTracks[0].ToString(verbose, true);  // header of the table
    for (int i = 0; i < nTracks; ++i) {
      msg << '\n' << setw(10) << setfill(' ') << fvTracks[i].ToString(verbose);
    }
    msg << "\n Point sample (first " << nPoints << " points):";
    msg << '\n' << setw(10) << setfill(' ') << fvPoints[0].ToString(verbose, true);  // header of the table
    for (int i = 0; i < nPoints; ++i) {
      msg << '\n' << setw(10) << setfill(' ') << fvPoints[i].ToString(verbose);
    }
  }
  return msg.str();
}
