/* Copyright (C) 2006-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel,  Sergey Gorbunov, Denis Bertini [committer], Igor Kulakov, Maksym Zyzak, Sergei Zharko */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction 
 *  
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de 
 *
 *====================================================================
 *
 *  L1 track class
 *
 *====================================================================
 */

// TODO: SZh 06.06.2024: Rework this class (inherit the new CbmTrack class with additional functionality)

#ifndef CbmL1Track_H
#define CbmL1Track_H

#include "CaVector.h"
#include "CbmL1Constants.h"
#include "CbmL1MCTrack.h"
#include "KfTrackParam.h"
#include "TMath.h"

#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace
{
  namespace ca = cbm::algo::ca;
}


class CbmL1MCTrack;

class CbmL1Track : public cbm::algo::kf::TrackParamD {
 public:
  CbmL1Track() = default;

  /// Adds an index of MC track index
  void AddMCTrackIndex(int iMT) { fvMcTrackIndexes.push_back_no_warning(iMT); }

  /// Clears the contents of matched MC track indexes (and pointers)
  void ClearMatchedMCTracks() { fvMcTrackIndexes.clear(); }

  bool IsGhost() const { return (maxPurity < CbmL1Constants::MinPurity); }

  /// @brief Gets first hit index
  int GetFirstHitIndex() const { return Hits.front(); }

  /// @brief Gets last hit index
  int GetLastHitIndex() const { return Hits.back(); }

  /// Gets hit indexes
  const auto& GetHitIndexes() const { return Hits; }


  /// Gets a reference to MC track indexes
  const auto& GetMCTrackIndexes() const { return fvMcTrackIndexes; }

  /// Gets max purity
  double GetMaxPurity() const { return maxPurity; }

  /// @brief Gets index of matched MC track
  /// @note  If two tracks are matched (should not happen, if purity > 50%), the first
  ///
  int GetMatchedMCTrackIndex() const { return fvMcTrackIndexes.size() ? fvMcTrackIndexes[0] : -1; }

  /// Gets number of hits of the track
  int GetNofHits() const { return Hits.size(); }

  /// Gets number of associated MC tracks
  int GetNofMCTracks() const { return fvMcTrackIndexes.size(); }

  /// Gets number of stations
  int GetNofStations() const { return nStations; }


  /// Sets max purity
  /// NOTE: max purity is calculated as a ratio of max number of hits left by an actual track and the
  ///       total number of hits in the track
  void SetMaxPurity(double maxPurity_) { maxPurity = maxPurity_; }

  /// Gets probability of track fit model
  double GetProb() const { return TMath::Prob(GetChiSq(), GetNdf()); }

  /// @brief Provides a string representation of object
  /// @param verbose  Verbosity level
  /// @param header   If true, header will be printed
  std::string ToString(int verbose = 10, bool header = false) const;

  cbm::algo::kf::TrackParamD Tpv;    ///< Track parameters at primary vertex
  cbm::algo::kf::TrackParamD TLast;  ///< Track parameters in the end of the track

  std::vector<int> Hits;  ///< Indexes of hits of this track
  int nStations{0};       ///< Number of stations with hits of this track
  int index{0};           ///< Index of this track (TODO: it seems to be not initialized)

  double fTrackTime{0.};  ///< Time of the track [ns] ???

  std::map<int, int> hitMap;  // N hits (second) from each mcTrack (first is a MC track ID) belong to current recoTrack
                              // FIXME: SZh 14.12.2022:  map => unordered_map

 private:
  ca::Vector<int> fvMcTrackIndexes = {"CbmL1Track::fvMcTrackIndexes"};  // global indexes of MC tracks
  // NOTE: mcTracks should be replaced with fvMcTrackIndexes

  double maxPurity{-1.};  ///< Maximum persent of hits, which belong to one mcTrack.
};

#endif
