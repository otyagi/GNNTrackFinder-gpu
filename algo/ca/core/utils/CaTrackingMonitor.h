/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaTrackingMonitor.h
/// \brief  Monitor specialization for the tracking algorithm
/// \since  19.10.2023
/// \author S.Zharko <s.zharko@gsi.de>

// NOTE: SZh: #prama once does not work properly in ROOT macros, so to use there enums from the header one
//            have to use old approach to protect from multiple includes
//#pragma once

#ifndef CaTrackingMonitor_h
#define CaTrackingMonitor_h 1

#include "CaMonitor.h"

#include <boost/serialization/base_object.hpp>

namespace cbm::algo::ca
{
  /// \enum  ECounter
  /// \brief Counter keys for the CA algo monitor
  enum class ECounter {
    TrackingCall,  ///< number of the routine calls
    SubTS,         ///< number of sub time-slices
    RecoTrack,     ///< number of reconstructed tracks
    RecoHit,       ///< number of reconstructed hits
    RecoHitUsed,   ///< number of used reconstructed hits
    Triplet,       ///< number of triplets
    // TODO: Provide counters vs. detector ID
    RecoMvdHit,        ///< number of MVD hits in tracks
    RecoStsHit,        ///< number of STS hits in tracks
    RecoMuchHit,       ///< number of MUCH hits in tracks
    RecoTrdHit,        ///< number of TRD hits in tracks
    RecoTofHit,        ///< number of TOF hits in tracks
    UndefinedMvdHit,   ///< number of undefined MVD hits
    UndefinedStsHit,   ///< number of undefined STS hits
    UndefinedMuchHit,  ///< number of undefined MuCh hits
    UndefinedTrdHit,   ///< number of undefined TRD hits
    UndefinedTofHit,   ///< number of undefined TOF hits
    END
  };

  /// \enum  ETimer
  /// \brief Timer keys for the CA algo monitor
  /* clang-format off */
  // NOTE: SZh 21.03.2024: Disabling clang-format to indicate the scope of timers using the indent
  enum class ETimer
  {
    TrackingChain,
      PrepareInputData,
        PrepareStsHits,
        PrepareTrdHits,
        PrepareTofHits,
        InputDataTransmission,
        CaHitCreation,
      Tracking,
        PrepareTimeslice,
        TrackingThread,
          PrepareThread,
          PrepareWindow,
          TrackingWindow,
            InitWindow,
            PrepareGrid,
            FindTracks, /// (iterations loop)
              PrepareIteration,
              ConstructTriplets,
              SearchNeighbours,
              CreateTracks,
              SuppressHitKeys,
            FitTracks,
            MergeClones,
          StoreTracksWindow,
        StoreTracksFinal,
      Qa,
    END,
    GNNTracking,
    MetricLearning,
    NearestNeighbours,
    TripletConstruction,
    TripletFit,
    TrackCandidate,
    TrackCompetition
  };
  /* clang-format on */

  using TrackingMonitorData = MonitorData<ECounter, ETimer>;

  /// \class cbm::algo::ca::TrackingMonitor
  /// \brief A monitor specialization for cbm::algo::ca::Framework class
  class TrackingMonitor : public Monitor<ECounter, ETimer> {
   public:
    /// \brief Default constructor
    TrackingMonitor() : Monitor<ECounter, ETimer>("CA Framework Monitor")
    {
      SetCounterName(ECounter::TrackingCall, "full routine calls");
      SetCounterName(ECounter::RecoTrack, "reco tracks");
      SetCounterName(ECounter::RecoHit, "reco hits");
      SetCounterName(ECounter::Triplet, "triplets");
      SetCounterName(ECounter::RecoHitUsed, "used reco hits");
      SetCounterName(ECounter::SubTS, "sub-timeslices");
      SetCounterName(ECounter::RecoMvdHit, "MVD hits in tracks");
      SetCounterName(ECounter::RecoStsHit, "STS hits in tracks");
      SetCounterName(ECounter::RecoMuchHit, "MUCH hits in tracks");
      SetCounterName(ECounter::RecoTrdHit, "TRD hits in tracks");
      SetCounterName(ECounter::RecoTofHit, "TOF hits in tracks");
      SetCounterName(ECounter::UndefinedMvdHit, "undefined MVD hits");
      SetCounterName(ECounter::UndefinedStsHit, "undefined STS hits");
      SetCounterName(ECounter::UndefinedMuchHit, "undefined MuCh hits");
      SetCounterName(ECounter::UndefinedTrdHit, "undefined TRD hits");
      SetCounterName(ECounter::UndefinedTofHit, "undefined TOF hits");

      SetTimerName(ETimer::TrackingChain, "tracking chain");
      SetTimerName(ETimer::PrepareInputData, "input data preparation");
      SetTimerName(ETimer::PrepareStsHits, "STS hits preparation");
      SetTimerName(ETimer::PrepareTrdHits, "TRD hits preparation");
      SetTimerName(ETimer::PrepareTofHits, "TOF hits preparation");
      SetTimerName(ETimer::InputDataTransmission, "input data transmission");
      SetTimerName(ETimer::CaHitCreation, "CA hit creation");
      SetTimerName(ETimer::Tracking, "algorithm execution");
      SetTimerName(ETimer::PrepareTimeslice, "timeslice preparation");
      SetTimerName(ETimer::TrackingThread, "tracking on one thread");
      SetTimerName(ETimer::PrepareThread, "thread preparation");
      SetTimerName(ETimer::PrepareWindow, "window preparation");
      SetTimerName(ETimer::TrackingWindow, "tracking in one window");
      SetTimerName(ETimer::InitWindow, "window initialization");
      SetTimerName(ETimer::PrepareGrid, "grid preparation");
      SetTimerName(ETimer::FindTracks, "track finding");
      SetTimerName(ETimer::PrepareIteration, "iteration preparation");
      SetTimerName(ETimer::ConstructTriplets, "triplet constrcution");
      SetTimerName(ETimer::SearchNeighbours, "neighbors search");
      SetTimerName(ETimer::CreateTracks, "track creation");
      SetTimerName(ETimer::SuppressHitKeys, "used hit key suppression");
      SetTimerName(ETimer::FitTracks, "track fitter");
      SetTimerName(ETimer::MergeClones, "clone merger");
      SetTimerName(ETimer::StoreTracksWindow, "track storing in window");
      SetTimerName(ETimer::StoreTracksFinal, "final track storing");
      SetTimerName(ETimer::Qa, "QA");
      SetTimerName(ETimer::GNNTracking, "GNN Tracking");
      SetTimerName(ETimer::MetricLearning, "Metric Learning");
      SetTimerName(ETimer::NearestNeighbours, "kNN Nearest Neighbours");
      SetTimerName(ETimer::TripletConstruction, "Triplet Construction");
      SetTimerName(ETimer::TripletFit, "Triplet Fit");
      SetTimerName(ETimer::TrackCandidate, "Track Candidate Construction");
      SetTimerName(ETimer::TrackCompetition, "Track Competition-Altruistic");
      SetRatioKeys({ECounter::TrackingCall, ECounter::SubTS, ECounter::RecoTrack});
    }

   private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<Monitor<ECounter, ETimer>>(*this);
    }
  };

}  // namespace cbm::algo::ca

#endif