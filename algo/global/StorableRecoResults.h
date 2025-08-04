/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], P.-A. Loizeau */
#ifndef CBM_ALGO_GLOBAL_STORABLE_RECO_RESULTS_H
#define CBM_ALGO_GLOBAL_STORABLE_RECO_RESULTS_H

#include "CbmDigiEvent.h"
#include "PartitionedVector.h"
#include "ca/core/data/CaTrack.h"
#include "ca/core/utils/CaVector.h"
#include "sts/Cluster.h"
#include "sts/Hit.h"
#include "tof/Hit.h"
#include "trd/Hit.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

#include <cstdint>

namespace cbm::algo
{

  class StorableRecoResults {

   public:
    using TrackHitIndexContainer_t = ca::Vector<std::vector<std::pair<uint32_t, uint32_t>>>;

    /**
     * @brief Default constructor (required by boost::serialization)
     */
    StorableRecoResults() = default;

    StorableRecoResults(uint64_t tsIndex, uint64_t tsStartTime) : fTsIndex(tsIndex), fTsStartTime(tsStartTime) {}

    /**
     * @brief Index of the timeslice during the run
     */
    uint64_t TsIndex() const { return fTsIndex; }

    /**
     * @brief Start time of the timeslice
     */
    uint64_t TsStartTime() const { return fTsStartTime; }

    /**
     * @brief Total size in bytes
     */
    size_t SizeBytes() const;

    std::vector<CbmBmonDigi>& BmonDigis() { return fBmonDigis; }
    const std::vector<CbmBmonDigi>& BmonDigis() const { return fBmonDigis; }

    std::vector<CbmStsDigi>& StsDigis() { return fStsDigis; }
    const std::vector<CbmStsDigi>& StsDigis() const { return fStsDigis; }

    std::vector<CbmMuchDigi>& MuchDigis() { return fMuchDigis; }
    const std::vector<CbmMuchDigi>& MuchDigis() const { return fMuchDigis; }

    std::vector<CbmTrdDigi>& Trd2dDigis() { return fTrd2dDigis; }
    const std::vector<CbmTrdDigi>& Trd2dDigis() const { return fTrd2dDigis; }

    std::vector<CbmTrdDigi>& TrdDigis() { return fTrdDigis; }
    const std::vector<CbmTrdDigi>& TrdDigis() const { return fTrdDigis; }

    std::vector<CbmTofDigi>& TofDigis() { return fTofDigis; }
    const std::vector<CbmTofDigi>& TofDigis() const { return fTofDigis; }

    std::vector<CbmRichDigi>& RichDigis() { return fRichDigis; }
    const std::vector<CbmRichDigi>& RichDigis() const { return fRichDigis; }

    std::vector<CbmDigiEvent>& DigiEvents() { return fDigiEvents; }
    const std::vector<CbmDigiEvent>& DigiEvents() const { return fDigiEvents; }

    PartitionedVector<sts::Cluster>& StsClusters() { return fStsClusters; }
    const PartitionedVector<sts::Cluster>& StsClusters() const { return fStsClusters; }

    PartitionedVector<sts::Hit>& StsHits() { return fStsHits; }
    const PartitionedVector<sts::Hit>& StsHits() const { return fStsHits; }

    PartitionedVector<tof::Hit>& TofHits() { return fTofHits; }
    const PartitionedVector<tof::Hit>& TofHits() const { return fTofHits; }

    PartitionedVector<trd::Hit>& TrdHits() { return fTrdHits; }
    const PartitionedVector<trd::Hit>& TrdHits() const { return fTrdHits; }

    ca::Vector<ca::Track>& Tracks() { return fTracks; }
    const ca::Vector<ca::Track>& Tracks() const { return fTracks; }

    TrackHitIndexContainer_t& TrackStsHitIndices() { return fTrackStsHitIndices; }
    const TrackHitIndexContainer_t& TrackStsHitIndices() const { return fTrackStsHitIndices; }

    TrackHitIndexContainer_t& TrackTofHitIndices() { return fTrackTofHitIndices; }
    const TrackHitIndexContainer_t& TrackTofHitIndices() const { return fTrackTofHitIndices; }

   private:
    uint64_t fTsIndex     = UINT64_MAX;
    uint64_t fTsStartTime = UINT64_MAX;

    // Unpackers output
    std::vector<CbmBmonDigi> fBmonDigis;
    std::vector<CbmStsDigi> fStsDigis;
    std::vector<CbmMuchDigi> fMuchDigis;
    std::vector<CbmTrdDigi> fTrd2dDigis;
    std::vector<CbmTrdDigi> fTrdDigis;
    std::vector<CbmTofDigi> fTofDigis;
    std::vector<CbmRichDigi> fRichDigis;

    // Event builder/filter output
    std::vector<CbmDigiEvent> fDigiEvents;

    // Local Reconstruction output
    PartitionedVector<sts::Cluster> fStsClusters;
    PartitionedVector<sts::Hit> fStsHits;
    PartitionedVector<tof::Hit> fTofHits;
    PartitionedVector<trd::Hit> fTrdHits;

    // Tracking output
    ca::Vector<ca::Track> fTracks;

    /// \brief STS hit indices of tracks
    /// \note  index: [trkID][hitID], value: pair(partitionID, hitPartitionID)
    TrackHitIndexContainer_t fTrackStsHitIndices;

    /// \brief TOF hit indices of tracks
    /// \note  index: [trkID][hitID], value: pair(partitionID, hitPartitionID)
    TrackHitIndexContainer_t fTrackTofHitIndices;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar& fTsIndex;
      ar& fTsStartTime;

      ar& fBmonDigis;
      ar& fStsDigis;
      ar& fMuchDigis;
      ar& fTrd2dDigis;
      ar& fTrdDigis;
      ar& fTofDigis;
      ar& fRichDigis;

      ar& fDigiEvents;

      ar& fStsClusters;
      ar& fStsHits;
      ar& fTofHits;
      ar& fTrdHits;

      ar& fTracks;
      ar& fTrackStsHitIndices;
      ar& fTrackTofHitIndices;
    }
  };

}  // namespace cbm::algo

#endif  // CBM_ALGO_GLOBAL_STORABLE_RECO_RESULTS_H
