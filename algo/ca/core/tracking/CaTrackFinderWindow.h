/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer], Maksym Zyzak */

/// \file    CaTrackFinderWindow.h
/// \authors S.Zharko <s.zharko@gsi.de> (interface), M.Zyzak (original algorithm)
/// \brief   A class wrapper over clones merger algorithm for the CA track finder (declaration)
/// \since   22.07.2022

#pragma once  // include this header only once per compilation unit

#include "CaBranch.h"
#include "CaCloneMerger.h"
#include "CaGpuTrackFinderSetup.h"
#include "CaGrid.h"
#include "CaHit.h"
#include "CaSimd.h"
#include "CaTrackExtender.h"
#include "CaTrackFitter.h"
#include "CaTrackingMonitor.h"
#include "CaVector.h"
#include "CaWindowData.h"

namespace cbm::algo::ca
{

  class Track;
  class Triplet;
  class Framework;

  /// Class implements a clones merger algorithm for the CA track finder
  ///
  class TrackFinderWindow {
   public:
    /// Default constructor
    TrackFinderWindow(const ca::Parameters<fvec>& pars, const fscal mass, const ca::TrackingMode& mode,
                      ca::TrackingMonitorData& monitorData);
    /// Destructor
    ~TrackFinderWindow() = default;

    /// Copy constructor
    TrackFinderWindow(const TrackFinderWindow&) = default;

    /// Move constructor
    TrackFinderWindow(TrackFinderWindow&&) = default;

    /// Copy assignment operator
    TrackFinderWindow& operator=(const TrackFinderWindow&) = delete;

    /// Move assignment operator
    TrackFinderWindow& operator=(TrackFinderWindow&&) = delete;

    void CaTrackFinderSlice(const ca::InputData& input, WindowData& wData);

    /// \note The function initializes global arrays for a given thread
    void InitTimeslice(size_t nHitKeys) { fvHitKeyToTrack.reset(nHitKeys, -1); }

   private:
    ///-------------------------------
    /// Private methods

    typedef std::array<Vector<ca::Triplet>, constants::size::MaxNstations> TripletArray_t;

    // TripletData_t:
    // 1 -> vHitFirstTriplet   /// link hit -> first triplet { hit, *, *}
    // 2 -> vHitNofTriplets    /// link hit ->n triplets { hit, *, *}
    // 3 -> vTriplets;
    typedef std::tuple<Vector<int>, Vector<int>, TripletArray_t> TripletData_t;

    void ConstructTriplets(WindowData& wData);

    void ReadWindowData(const Vector<Hit>& hits, WindowData& wData);

    void PrepareGrid(const Vector<Hit>& hits, WindowData& wData);

    void PrepareCAIteration(const ca::Iteration& caIteration, WindowData& wData, const bool isFirst);

    void CreateTracks(WindowData& wData, const ca::Iteration& caIteration, TripletArray_t& vTriplets);

    void CreateTrackCandidates(WindowData& wData, TripletArray_t& vTriplets, const int min_level,
                               const int firstTripletLevel);

    void DoCompetitionLoop(const WindowData& wData);

    void SelectTracks(WindowData& wData);

    void SearchNeighbors(WindowData& wData);

    void CAFindTrack(int ista, ca::Branch& best_tr, const ca::Triplet* curr_trip, ca::Branch& curr_tr,
                     unsigned char min_best_l, ca::Branch* new_tr, WindowData& wData, TripletArray_t& vTriplets);

    bool checkTripletMatch(const ca::Triplet& l, const ca::Triplet& r, fscal& dchi2, WindowData& wData) const;

    void SetupGpuTrackFinder(GpuTrackFinderSetup& gpuTrackFinderSetup);

    void ConstructTripletsGPU(WindowData& wData, GpuTrackFinderSetup& gpuTrackFinderSetup, int iteration);

    // ** Functions, which pack and unpack indexes of station and triplet **

    // TODO: move to ca::Triplet class (S.Zharko)
    /// Packs station and triplet indices to an unique triplet ID
    /// \param  iStation  Index of station in the active stations array
    /// \param  iTriplet  Index of triplet
    static unsigned int PackTripletId(unsigned int iStation, unsigned int iTriplet);

    /// Unpacks the triplet ID to its station index
    /// \param  id  Unique triplet ID
    static unsigned int TripletId2Station(unsigned int id);

    /// Unpacks the triplet ID to its triplet index
    /// \param  id  Unique triplet ID
    static unsigned int TripletId2Triplet(unsigned int id);


   private:
    ///-------------------------------
    /// Data members

    static constexpr bool fDebug = false;  // print debug info

    const Parameters<fvec>& fParameters;            ///< Object of Framework parameters class
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
    ca::TrackingMode fTrackingMode;

    TrackingMonitorData& frMonitorData;  ///< Reference to monitor data
    TrackExtender fTrackExtender;        ///< Object of the track extender algorithm
    CloneMerger fCloneMerger;            ///< Object of  the clone merger algorithm
    TrackFitter fTrackFitter;            ///< Object of the track extender algorithm

    /// \note Global array for a given thread
    Vector<int> fvHitKeyToTrack{"TrackFinderWindow::fvHitKeyToTrack"};

    // Persistent to avoid memory allocations.
    // Only used in CreateTrackCandidates().
    ca::Branch fNewTr[constants::size::MaxNstations];

    // Track candidates created out of adjacent triplets before the final track selection.
    // The candidates may share any amount of hits.
    Vector<ca::Branch> fvTrackCandidates{"TrackFinderWindow::fvTrackCandidates"};

    // Triplets and related meta data
    TripletData_t fTripletData;

    // Triplet temporary storage. Only used in ConstructTriplets().
    Vector<ca::Triplet> fvTriplets;
  };

  // ********************************************
  // ** Inline member functions implementation **
  // ********************************************

  // ---------------------------------------------------------------------------------------------------------------------
  //
  [[gnu::always_inline]] inline unsigned int TrackFinderWindow::PackTripletId(unsigned int iStation,
                                                                              unsigned int iTriplet)
  {
    assert(iStation < constants::size::MaxNstations);
    assert(iTriplet < constants::size::MaxNtriplets);
    constexpr unsigned int kMoveStation = constants::size::TripletBits;
    return (iStation << kMoveStation) + iTriplet;
  }

  // ---------------------------------------------------------------------------------------------------------------------
  //
  [[gnu::always_inline]] inline unsigned int TrackFinderWindow::TripletId2Station(unsigned int id)
  {
    constexpr unsigned int kMoveStation = constants::size::TripletBits;
    return id >> kMoveStation;
  }

  // ---------------------------------------------------------------------------------------------------------------------
  //
  [[gnu::always_inline]] inline unsigned int TrackFinderWindow::TripletId2Triplet(unsigned int id)
  {
    constexpr unsigned int kTripletMask = (1u << constants::size::TripletBits) - 1u;
    return id & kTripletMask;
  }


}  // namespace cbm::algo::ca
