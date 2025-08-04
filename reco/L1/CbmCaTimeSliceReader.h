/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaTimeSliceReader.h
/// @brief  Time-slice/event reader for CA tracker in CBM (header)
/// @since  24.02.2023
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CaDataManager.h"
#include "CaDefs.h"
#include "CaToolsHitRecord.h"
#include "CaVector.h"
#include "CbmCaMCModule.h"
#include "CbmEvent.h"
#include "CbmL1DetectorID.h"
#include "CbmL1Hit.h"
#include "CbmL1Track.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdHit.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmPixelHit.h"
#include "CbmStsHit.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofHit.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdHit.h"
#include "CbmTrdTrackingInterface.h"
#include "TClonesArray.h"

class CbmTimeSlice;
namespace cbm::algo::ca
{
  class DataManager;
  template<typename DataT>
  class Parameters;
}  // namespace cbm::algo::ca

namespace cbm::ca
{
  /// @brief A reader of time slice for CA tracker
  ///
  /// The class reads reconstructed hits and reconstructed tracks (optionally) and fills the CA tracking internal
  /// data structures.
  ///
  class TimeSliceReader {
   public:
    /// @brief Constructor from parameters
    /// @param mode  Tracking mode
    TimeSliceReader() = default;

    /// @brief Destructor
    ~TimeSliceReader() = default;

    /// @brief Copy constructor
    TimeSliceReader(const TimeSliceReader&) = delete;

    /// @brief Move constructor
    TimeSliceReader(TimeSliceReader&&) = delete;

    /// @brief Copy assignment operator
    TimeSliceReader& operator=(const TimeSliceReader&) = delete;

    /// @brief Move assignment operator
    TimeSliceReader& operator=(TimeSliceReader&&) = delete;

    /// @brief Clears class content
    void Clear();

    /// @brief Gets reference to container of first hit indexes in a detector subsystem
    /// @return Ref. to the container
    const auto& GetHitFirstIndexDet() const { return fvHitFirstIndexDet; }

    /// @brief Gets number of hits stored for a given detector
    /// @param iDet  Detector ID
    /// @return Number of hits
    int GetNofHits(ca::EDetectorID iDet) const
    {
      return fvHitFirstIndexDet[int(iDet) + 1] - fvHitFirstIndexDet[int(iDet)];
    }

    /// @brief Gets CBM tracking mode
    ECbmCaTrackingMode GetTrackingMode() const { return fTrackingMode; }

    /// @brief Run initializer function
    /// @return Success flag
    ///
    /// Initializes data branches and provides necessary checks in the beginning of the run
    bool InitRun();

    /// @brief Reads time slice
    /// @param pEvent  A pointer to CbmEvent
    ///
    /// Reads hits and tracks (optionally) from time slice
    void ReadEvent(CbmEvent* pEvent = nullptr);

    /// @brief Registers hit debug info container
    /// @param vQaHits  Reference to Qa hit container
    /// @note If no container is registered, all related routines are omitted
    void RegisterQaHitContainer(ca::Vector<CbmL1HitDebugInfo>& vQaHits) { fpvQaHits = &vQaHits; }

    /// @brief Registers hit index container
    /// @param vHitIds  Reference to hits indexes container
    /// @note If no container is registered, all related routines are omitted
    void RegisterHitIndexContainer(ca::Vector<CbmL1HitId>& vHitIds) { fpvHitIds = &vHitIds; }

    /// @brief Registers CA parameters object
    /// @param pParameters  A shared pointer to the parameters object
    void RegisterParameters(std::shared_ptr<ca::Parameters<float>>& pParameters) { fpParameters = pParameters; }

    /// @brief Registers the CA IO data manager instance
    /// @param pIODataManager  Shared pointer to the IO data manager instance
    /// @note If no container is registered, all related routines are omitted
    void RegisterIODataManager(std::shared_ptr<ca::DataManager>& ioDataManager);

    /// @brief Register the reconstructed tracks container
    /// @param vTracks  Reference to reconstructed tracks container
    /// @note If no container is registered, all related routines are omitted
    void RegisterTracksContainer(ca::Vector<CbmL1Track>& vTracks) { fpvTracks = &vTracks; }

    /// @brief Sets used detector subsystems
    /// @param  detID  Id of detector
    /// @param  flag   Flag: true - detector is used
    /// @note Should be called before this->Init()
    void SetDetector(ca::EDetectorID detID, bool flag = true) { fvbUseDet[detID] = flag; }

    /// @brief  Sets the tracking mode
    /// @param  mode Tracking mode (from ECbmTrackingMode)
    void SetTrackingMode(ECbmCaTrackingMode mode) { fTrackingMode = mode; }

    /// @brief  Sets flag to additionally sort QA hits by stations
    /// @param  doSortQaHits  true: hits are sorted
    void SetSortQaHits(bool doSortQaHits) { fbSortQaHits = doSortQaHits; }

   private:
    /// @brief Check class initialization
    /// @note The function throws std::logic_error, if initialization is incomplete
    void CheckInit() const;

    /// @brief Reads hits
    void ReadHits();

    /// @brief Reads reconstructed tracks
    void ReadRecoTracks();

    /// @brief Reads hits for a given detector subsystem
    /// @tparam Detector ID
    /// @return  Number of stored hits
    /// @note The function modifies fNofHitKey and fFirstHitKey counters
    template<ca::EDetectorID DetID>
    int ReadHitsForDetector();

    /// @brief Sorts QA hit objects by stations
    void SortQaHits();

    /// @brief Saves hit to data structures
    /// @param hitRecord  Filled hit record
    ///
    /// Stores recorded hit information into registered hit containers
    void StoreHitRecord(const tools::HitRecord& hitRecord);

    /// @brief Pointers to the tracking detector interfaces for each subsystem
    DetIdArr_t<const CbmTrackingDetectorInterfaceBase*> fvpDetInterface = {{nullptr}};

    // Input data branches
    // CbmTimeSlice* fpBrTimeSlice         = nullptr;      ///< Pointer to the TS object
    CbmEvent* fpEvent                   = nullptr;      ///< Pointer to the event object
    DetIdArr_t<TClonesArray*> fvpBrHits = {{nullptr}};  ///< Input branch for hits

    // Branches for reconstructed tracks. The input at the moment (as for 27.02.2023) depends on the selected
    // tracking mode. For simulations in CBM, the CA tracking is used only in STS + MVD detectors. In this case
    // the reconstructed tracks are saved to the "StsTrack" branch as CbmStsTrack objects. For mCBM, the tracks from
    // CA are saved as global tracks, and the local ones are used to keep indexes of hits in different subsystems
    TClonesArray* fpBrRecoTracks = nullptr;  ///< Input branch for reconstructed tracks ("GlobalTrack", "StsTrack")
    TClonesArray* fpBrStsTracks  = nullptr;  ///< Input branch for reconstructed STS tracks ("StsTrack")
    TClonesArray* fpBrMuchTracks = nullptr;  ///< Input branch for reconstructed MuCh tracks ("MuchTrack")
    TClonesArray* fpBrTrdTracks  = nullptr;  ///< Input branch for reconstructed TRD tracks ("TrdTrack")
    TClonesArray* fpBrTofTracks  = nullptr;  ///< Input branch for reconstructed TOF tracks ("TofTrack")

    // Pointers to output data containers
    ca::Vector<CbmL1HitId>* fpvHitIds                   = nullptr;  ///< Pointer to array of hit index objects
    ca::Vector<CbmL1HitDebugInfo>* fpvQaHits            = nullptr;  ///< Pointer to array of debug hits
    ca::Vector<CbmL1Track>* fpvTracks                   = nullptr;  ///< Pointer to array of reconstructed tracks
    std::shared_ptr<ca::DataManager> fpIODataManager    = nullptr;  ///< Pointer to input data manager
    std::shared_ptr<ca::Parameters<float>> fpParameters = nullptr;  ///< Pointer to tracking parameters object

    // Maps of hit indexes: ext -> int
    DetIdArr_t<std::unordered_map<int, int>> fvmHitExtToIntIndexMap;  ///< Hit index map ext -> int

    DetIdArr_t<int> fvNofHitsTotal = {{0}};      ///< Total hit number in detector
    DetIdArr_t<int> fvNofHitsUsed  = {{0}};      ///< Number of used hits in detector
    DetIdArr_t<bool> fvbUseDet     = {{false}};  ///< Flag: is detector subsystem used

    ECbmCaTrackingMode fTrackingMode = ECbmCaTrackingMode::kSTS;  ///< Tracking mode

    bool fbSortQaHits = false;  ///< Flag, if the QA hits must be sorted after reading

    // Variables for storing cache
    int fNofHits     = 0;  ///< Stored number of hits
    int fNofHitKeys  = 0;  ///< Recorded number of hit keys
    int fFirstHitKey = 0;  ///< First index of hit key for the detector subsystem

    std::array<int, constants::size::MaxNdetectors + 1> fvHitFirstIndexDet = {{0}};  ///< First hit index in detector
  };
}  // namespace cbm::ca
