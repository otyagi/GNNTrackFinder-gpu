/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Maksym Zyzak */

/// \file    CaTrackFinder.h

#pragma once  // include this header only once per compilation unit

#include "CaSimd.h"
#include "CaTimesliceHeader.h"
#include "CaTrackExtender.h"
#include "CaTrackFinderWindow.h"
#include "CaTrackFitter.h"
#include "CaVector.h"
#include "KfTrackParam.h"

#include <vector>

namespace cbm::algo::ca
{

  class Track;
  class Triplet;
  class Framework;

  /// Class implements a clones merger algorithm for the CA track finder
  ///
  class TrackFinder {
   public:
    typedef std::pair<Vector<Track>, Vector<ca::HitIndex_t>> Output_t;

    /// Default constructora
    TrackFinder(const ca::Parameters<fvec>& pars, const fscal mass, const ca::TrackingMode& mode,
                TrackingMonitorData& monitorData, int nThreads, double& recoTime);
    /// Destructor
    ~TrackFinder() = default;

    /// Copy constructor
    TrackFinder(const TrackFinder&) = delete;

    /// Move constructor
    TrackFinder(TrackFinder&&) = delete;

    /// Copy assignment operator
    TrackFinder& operator=(const TrackFinder&) = delete;

    /// Move assignment operator
    TrackFinder& operator=(TrackFinder&&) = delete;

    Output_t FindTracks(const InputData& input, TimesliceHeader& tsHeader);

    const auto& GetRecoTracksContainer(int iThread) const { return fvRecoTracks[iThread]; }
    const auto& GetRecoHitIndicesContainer(int iThread) const { return fvRecoHitIndices[iThread]; }
    TrackingMode GetTrackingMode() const { return fTrackingMode; }
    const std::vector<ca::WindowData>& GetWData() const { return fvWData; }

   private:
    // -------------------------------
    // Private methods
    void FindTracksThread(const InputData& input, int iThread, std::pair<fscal, fscal>& windowRange, int& statNwindows,
                          int& statNhitsProcessed);
    //   bool checkTripletMatch(const ca::Triplet& l, const ca::Triplet& r, fscal& dchi2) const;

    // -------------------------------
    // Data members

    Vector<CaHitTimeInfo> fHitTimeInfo;

    const Parameters<fvec>& fParameters;            ///< Object of Framework parameters class
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
    ca::TrackingMode fTrackingMode;

    TrackingMonitorData& fMonitorData;                     ///< Tracking monitor data (statistics per call)
    std::vector<TrackingMonitorData> fvMonitorDataThread;  ///< Tracking monitor data per thread

    std::vector<ca::WindowData> fvWData;  ///< Intrnal data processed in a time-window

    int fNofThreads;      ///< Number of threads to execute the track-finder
    double& fCaRecoTime;  // time of the track finder + fitter

    std::vector<Vector<Track>> fvRecoTracks;           ///< reconstructed tracks
    std::vector<Vector<HitIndex_t>> fvRecoHitIndices;  ///< packed hits of reconstructed tracks

    fscal fWindowLength  = 0.;   ///< Time window length [ns]
    fscal fStatTsStart  = 0.;
    fscal fStatTsEnd    = 0.;
    int fStatNhitsTotal = 0;
  };

}  // namespace cbm::algo::ca
