/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Felix Weiglhofer, P.-A. Loizeau */

/// \file   RecoResults.h
/// \date   22.10.2023
/// \brief  A structure for reconstructed results: digi-events, hits and tracks
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "DigiData.h"
#include "PartitionedSpan.h"
#include "PartitionedVector.h"
#include "bmon/Hit.h"
#include "ca/core/data/CaTrack.h"
#include "ca/core/utils/CaVector.h"
#include "sts/Cluster.h"
#include "sts/Hit.h"
#include "tof/Hit.h"
#include "trd/Hit.h"

#include <vector>

namespace cbm::algo
{
  /// @name RecoResults
  /// @brief  Structure to keep reconstructed results: digi-events, hits and tracks
  struct RecoResults {
    using HitId_t = std::pair<uint32_t, uint32_t>;  // Hit ID by track

    PODVector<CbmBmonDigi> bmonDigis;
    PODVector<CbmStsDigi> stsDigis;
    PODVector<CbmMuchDigi> muchDigis;
    PODVector<CbmTrdDigi> trd2dDigis;
    PODVector<CbmTrdDigi> trdDigis;
    PODVector<CbmTofDigi> tofDigis;
    PODVector<CbmRichDigi> richDigis;

    std::vector<DigiEvent> events;

    PartitionedVector<sts::Cluster> stsClusters;

    PartitionedSpan<sts::Hit> stsHits;
    PartitionedVector<tof::Hit> tofHits;
    PartitionedVector<trd::Hit> trdHits;
    PartitionedVector<bmon::Hit> bmonHits;

    ca::Vector<ca::Track> tracks;
    ca::Vector<std::vector<HitId_t>> trackStsHitIndices;  // [trk][hit][(iPart, iHit)]
    ca::Vector<std::vector<HitId_t>> trackTofHitIndices;  // [trk][hit][(iPart, iHit)]
    ca::Vector<std::vector<HitId_t>> trackTrdHitIndices;  // [trk][hit][(iPart, iHit)]
  };
}  // namespace cbm::algo
