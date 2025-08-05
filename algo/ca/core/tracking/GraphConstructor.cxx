/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GraphConstructor.cxx
/// \brief Functions for running GNN algorithm
/// \author Oddharak Tyagi

#include "GraphConstructor.h"

namespace cbm::algo::ca
{

  GraphConstructor::GraphConstructor(WindowData& wData) : frWData(wData) {}

  // @brief: for debugging save all found edges as tracks
  void GraphConstructor::SaveAllEdgesAsTracks() {}

  // @brief: for debugging. saves triplets as tracks
  void GraphConstructor::SaveAllTripletsAsTracks() {}

  void GraphConstructor::FindFastPrim(const int mode) {}  // FindFastPrim

  /// combine overlapping triplets to form tracks. sort by length and score for competition
  void GraphConstructor::CreateTracksTriplets(const int mode) {}  // CreateTracksTriplets

  void GraphConstructor::FitTriplets() {}  // FitTriplets

  // @brief: add tracks and hits to final containers
  void GraphConstructor::PrepareFinalTracks() {}  // prepareFinalTracks

  void GraphConstructor::CreateMetricLearningDoublets(const int iter) {}

}  // namespace cbm::algo::ca