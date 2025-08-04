/* Copyright (C) 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingChain.h
/// \date   13.09.2023
/// \brief  A chain class to execute CA tracking algorithm in online reconstruction (header)
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "CaDataManager.h"
#include "CaFramework.h"
#include "CaQa.h"
#include "CaTrack.h"
#include "CaTrackingMonitor.h"
#include "CaVector.h"
#include "PartitionedSpan.h"
#include "RecoResults.h"
#include "SubChain.h"
#include "TrackingChainConfig.h"
#include "TrackingDefs.h"
#include "TrackingSetup.h"
#include "sts/Hit.h"
#include "tof/Hit.h"

#include <memory>
#include <vector>

namespace cbm::algo::qa
{
  class Manager;
}

namespace cbm::algo
{
  /// \class cbm::algo::TrackingChain
  /// \brief A chain for tracking algorithm
  ///
  /// The class executes a tracking algorithm in the online data reconstruction chain.
  class TrackingChain : public SubChain {
   public:
    /// \brief Constructor from parameters
    /// \param recoMode     Reconstruction mode
    /// \param pManager a QA-manager
    /// \param name A name of the task (histograms directory)
    TrackingChain(ECbmRecoMode recoMode, const std::unique_ptr<cbm::algo::qa::Manager>& qaManager = nullptr,
                  std::string_view name = "");

    /// \struct Input_t
    /// \brief  Input to the TrackingChain
    struct Input_t {
      PartitionedSpan<sts::Hit> stsHits;
      PartitionedSpan<tof::Hit> tofHits;
      PartitionedSpan<trd::Hit> trdHits;
    };

    /// \struct Output_t
    /// \brief  Output from the TrackingChain
    struct Output_t {
      /// \brief Reconstructed tracks
      ca::Vector<ca::Track> tracks;

      /// \brief STS hit indices
      /// \note  Indexing: [trackID][localHit], value: (partitionID, hitIDinPartition)
      ca::Vector<std::vector<std::pair<uint32_t, uint32_t>>> stsHitIndices;

      /// \brief TOF hit indices
      /// \note  Indexing: [trackID][localHit], value: (partitionID, hitIDinPartition)
      ca::Vector<std::vector<std::pair<uint32_t, uint32_t>>> tofHitIndices;

      /// \brief TRD hit indices
      /// \note  Indexing: [trackID][localHit], value: (partitionID, hitIDinPartition)
      ca::Vector<std::vector<std::pair<uint32_t, uint32_t>>> trdHitIndices;

      /// \brief Monitor data
      ca::TrackingMonitorData monitorData;
    };

    /// \brief  Gets internal monitor
    const ca::TrackingMonitor& GetCaMonitor() const { return fCaMonitor; }

    /// \brief  Provides action in the initialization of the run
    void Init();

    /// \brief  Registers tracking setup
    void RegisterSetup(std::shared_ptr<TrackingSetup> pSetup) { fpSetup = pSetup; }

    /// \brief  Provides action for a given time-slice
    /// \param  recoResults  Structure of reconstruction results
    /// \return A pair (vector of tracks, tracking monitor)
    Output_t Run(Input_t recoResults);

    /// \brief  Provides action in the end of the run
    void Finalize();


   private:
    // *********************
    // **  Utility functions

    /// \brief  Prepares input data
    /// \param  recoResults  Structure of reconstruction results
    void PrepareInput(Input_t recoResults);

    /// \brief  Prepares output data
    Output_t PrepareOutput();

    /// \brief  Reads from different detector subsystems
    /// \tparam DetID Detector ID
    /// \param  hits  Hits vector
    template<ca::EDetectorID DetID>
    void ReadHits(PartitionedSpan<const ca::HitTypes_t::at<DetID>> hits);

    // *************************
    // **  Framework variables

    ca::TrackingMonitor fCaMonitor{};              ///< CA internal monitor (debug purposes)
    ca::TrackingMonitorData fCaMonitorData{};      ///< CA monitor data object
    ca::Framework fCaFramework{};                  ///< CA framework instance
    ca::DataManager fCaDataManager{};              ///< CA data manager
    ca::Qa fQa;                                    ///< CA QA builder

    std::shared_ptr<TrackingSetup> fpSetup = nullptr;  ///< setup interface

    ca::DetIdArray_t<bool> fbDetUsed;  ///< Flags of detector subsystems used in tracking

    // ************************
    // **  Auxilary variables

    TrackingChainConfig fConfig;        ///< Tracking config
    ca::HitKeyIndex_t fNofHitKeys = 0;  ///< Current number of hit keys (aux)

    /// \brief External indices of used hits
    /// \note  Indexing: [globalHitID], value: (DetID, partitionID, hitID)
    ca::Vector<std::tuple<ca::EDetectorID, uint32_t, uint32_t>> faHitExternalIndices{"faHitExternalIndices"};

    ECbmRecoMode fRecoMode{ECbmRecoMode::Undefined};  ///< Reconstruction mode

    static constexpr bool kDEBUG = false;  ///< Debug mode
  };


}  // namespace cbm::algo
