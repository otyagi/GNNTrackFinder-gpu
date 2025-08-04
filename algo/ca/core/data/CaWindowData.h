/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file    CaWindowData.h
/// \author  Sergei Zharko <s.zharko@gsi.de>
/// \brief   Container for all data, which are processed within a single sub-timeslice
/// \since   29.01.2024

#pragma once

#include "CaDefs.h"
#include "CaGrid.h"
#include "CaHit.h"
#include "CaIteration.h"
#include "CaTrack.h"
#include "KfFieldRegion.h"
#include "KfMeasurementXy.h"

#include <array>

namespace cbm::algo::ca
{
  /// \class WindowData
  /// \brief Container for internal data, processed on a single time window
  class WindowData {
   public:
    /// \brief  Constructor
    WindowData() = default;

    /// \brief  Destructor
    ~WindowData() = default;

    /// \brief Gets grid for station index
    /// \param iStation  Index of the tracking station
    [[gnu::always_inline]] ca::Grid& Grid(int iStation) { return fvGrid[iStation]; }

    /// \brief Gets grid for station index
    /// \param iStation  Index of the tracking station
    [[gnu::always_inline]] const ca::Grid& Grid(int iStation) const { return fvGrid[iStation]; }

    /// \brief Gets hit by index
    /// \param iHit  Hit index
    [[gnu::always_inline]] ca::Hit& Hit(int iHit) { return fvHits[iHit]; }

    /// \brief Gets hit by index
    /// \param iHit  Hit index
    [[gnu::always_inline]] const ca::Hit& Hit(int iHit) const { return fvHits[iHit]; }

    /// \brief Gets hit vector
    [[gnu::always_inline]] Vector<ca::Hit>& Hits() { return fvHits; }

    /// \brief Gets hit vector
    [[gnu::always_inline]] const Vector<ca::Hit>& Hits() const { return fvHits; }

    /// \brief Gets hit suppression flag
    /// \param iHit  Hit index
    [[gnu::always_inline]] uint8_t IsHitSuppressed(int iHit) const { return fvbHitSuppressed[iHit]; }

    /// \brief Resets hit data
    /// \param nHits  Number of hits in the sample
    void ResetHitData(int nHits);

    /// \brief Reset suppressed hit flags
    [[gnu::always_inline]] void ResetHitSuppressionFlags() { fvbHitSuppressed.reset(fvHits.size(), 0); }

    /// \brief Set hit suppression flag
    [[gnu::always_inline]] void SuppressHit(int iHit) { fvbHitSuppressed[iHit] = 1; }

    /// \brief Index of the first hit on the station
    /// \param iStation  Index of the tracking station
    [[gnu::always_inline]] HitIndex_t& HitStartIndexOnStation(int iStation)
    {
      return fvHitStartIndexOnStation[iStation];
    }

    /// \brief Hit key flag: if this hit or cluster was already used
    /// \param iKey  Index of the key (index of the hit or the cluster)
    [[gnu::always_inline]] unsigned char IsHitKeyUsed(HitKeyIndex_t iKey) const { return fvbHitKeyFlags[iKey]; }

    /// \brief Hit key flag: if this hit or cluster was already used
    /// \param iKey  Index of the key (index of the hit or the cluster)
    [[gnu::always_inline]] unsigned char& IsHitKeyUsed(HitKeyIndex_t iKey) { return fvbHitKeyFlags[iKey]; }

    /// \brief Access to the hit key flags container
    [[gnu::always_inline]] Vector<unsigned char>& HitKeyFlags() { return fvbHitKeyFlags; }

    /// \brief Index of the first hit on the station
    /// \param iStation  Index of the tracking station
    [[gnu::always_inline]] HitIndex_t HitStartIndexOnStation(int iStation) const
    {
      return fvHitStartIndexOnStation[iStation];
    }

    /// \brief Number of hits on station
    /// \param iStation  Index of the tracking station
    [[gnu::always_inline]] HitIndex_t& NofHitsOnStation(int iStation) { return fvNofHitsOnStation[iStation]; }

    /// \brief Number of hits on station
    /// \param iStation  Index of the tracking station
    [[gnu::always_inline]] HitIndex_t NofHitsOnStation(int iStation) const { return fvNofHitsOnStation[iStation]; }

    /// \brief Accesses indices of hits, used by reconstructed tracks
    [[gnu::always_inline]] Vector<HitIndex_t>& RecoHitIndices() { return fvRecoHitIndices; }

    /// \brief Accesses indices of hits
    [[gnu::always_inline]] const Vector<HitIndex_t>& RecoHitIndices() const { return fvRecoHitIndices; }

    /// \brief Accesses index of hit in the input data
    /// \param iHit  Index of reconstructed hit, used in reconstructed tracks
    /// \return Index of reconstructed hit in the algorithm input data object
    [[gnu::always_inline]] HitIndex_t& RecoHitIndex(int iHit) { return fvRecoHitIndices[iHit]; }

    /// \brief Accesses index of hit in the input data
    /// \param iHit  Index of reconstructed hit, used in reconstructed tracks
    /// \return Index of reconstructed hit in the algorithm input data object
    [[gnu::always_inline]] HitIndex_t RecoHitIndex(int iHit) const { return fvRecoHitIndices[iHit]; }

    /// \brief Accesses reconstructed track by index
    /// \param iTrack  Track index
    [[gnu::always_inline]] Track& RecoTrack(int iTrack) { return fvRecoTracks[iTrack]; }

    /// \brief Accesses reconstructed track by index
    /// \param iTrack  Track index
    [[gnu::always_inline]] const Track& RecoTrack(int iTrack) const { return fvRecoTracks[iTrack]; }

    /// \brief Accesses reconstructed track container
    [[gnu::always_inline]] Vector<Track>& RecoTracks() { return fvRecoTracks; }

    /// \brief Accesses reconstructed track container
    [[gnu::always_inline]] const Vector<Track>& RecoTracks() const { return fvRecoTracks; }

    /// \brief Maps hit index from the time window to the time slice
    /// \param iSt   Index of tracking station
    /// \param iHit  Index of hit in the window
    [[gnu::always_inline]] HitIndex_t TsHitIndex(int iSt, int iHit) const { return fvTsHitIndices[iSt][iHit]; }

    /// \brief Accesses container of hit index map from the time window to the time slice
    /// \param iSt  Index of tracking station
    [[gnu::always_inline]] Vector<HitIndex_t>& TsHitIndices(int iSt) { return fvTsHitIndices[iSt]; }

    /// \brief Accesses container of hit index map from the time window to the time slice
    /// \param iSt  Index of tracking station
    [[gnu::always_inline]] const Vector<HitIndex_t>& TsHitIndices(int iSt) const { return fvTsHitIndices[iSt]; }

    /// \brief Accesses current iteration
    [[gnu::always_inline]] void SetCurrentIteration(const Iteration* ptr) { fpCurrentIteration = ptr; }

    /// \brief Accesses current iteration
    [[gnu::always_inline]] const Iteration* CurrentIteration() const { return fpCurrentIteration; }

    /// \brief Accesses magnetic field in starting point (target or first station)
    [[gnu::always_inline]] kf::FieldValue<fvec>& TargB() { return fTargB; }

    /// \brief Accesses magnetic field in starting point (target or first station)
    [[gnu::always_inline]] const kf::FieldValue<fvec>& TargB() const { return fTargB; }

    /// \brief Measurement of the target with the uncertainty
    [[gnu::always_inline]] kf::MeasurementXy<fvec>& TargetMeasurement() { return fTargetMeasurement; }

    /// \brief Measurement of the target with the uncertainty
    [[gnu::always_inline]] const kf::MeasurementXy<fvec>& TargetMeasurement() const { return fTargetMeasurement; }

   private:
    static constexpr int kMaxNofStations = constants::size::MaxNstations;  ///< Alias to max number of stations

    /// \brief Grid vs. station index
    std::array<ca::Grid, kMaxNofStations> fvGrid;

    /// \brief Hits of the current time window
    ///
    /// It is a portion of fInputData to process in the current time window
    /// hit.Id is replaced by the hit index in fInputData
    Vector<ca::Hit> fvHits{"WindowData::fHits"};

    /// \brief List of used hit keys
    /// \note  Global for all the time windows within one thread
    Vector<unsigned char> fvbHitKeyFlags{"WindowData::fvbHitKeyFlags"};

    /// \brief Flag, if the hit is suppressed for tracking
    Vector<unsigned char> fvbHitSuppressed{"WindowData::fvbHitSuppressed"};

    /// \brief First hit index of the station
    std::array<HitIndex_t, kMaxNofStations + 1> fvHitStartIndexOnStation = {0};

    /// \brief Number of hits on the station
    std::array<HitIndex_t, kMaxNofStations + 1> fvNofHitsOnStation = {0};

    /// \brief Sample of reconstructed tracks
    Vector<Track> fvRecoTracks{"WindowData::fvRecoTracks"};

    /// \brief Sample of reconstructed hit indices
    Vector<HitIndex_t> fvRecoHitIndices{"WindowData::fvRecoHitIndices"};

    /// \brief Map of hit indices from the time window to the time slice
    std::array<Vector<HitIndex_t>, kMaxNofStations> fvTsHitIndices{"WindowData::fvFullDSHitIndex"};

    /// \brief Current track-finder iteration
    const Iteration* fpCurrentIteration = nullptr;

    kf::FieldValue<fvec> fTargB{};                 ///< field in the target point (modifiable, do not touch!!)
    kf::MeasurementXy<fvec> fTargetMeasurement{};  ///< target constraint

  } _fvecalignment;
}  // namespace cbm::algo::ca
