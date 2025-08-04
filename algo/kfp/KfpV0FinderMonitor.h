/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderMonitor.h
/// \date   03.02.2025
/// \brief  A monitor for the V0Finder
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CaMonitor.h"

namespace cbm::algo::kfp
{
  /// \enum  ECounter
  /// \brief Counter keys for the V0FinderMonitor
  enum class ECounter
  {
    TracksTotal,                ///< Total number of tracks
    TracksSelected,             ///< Tracks, which satisfy topology PID applicability
    TracksInfiniteParam,        ///< Tracks, which have infinite parameters
    TracksWoTofHits,            ///< Tracks, which have no TOF hits
    TracksWNegativeTofHitTime,  ///< Tracks, the last TOF hit of which has a negative time (it's time is less then the t0)
    TracksWoStsHits,            ///< Tracks, which have no STS hits
    TracksWoPid,                ///< Tracks, which has undefined PID
    TracksWoMomentum,           ///< Tracks, which has no momentum
    TracksWUnphysicalBeta,      ///< Tracks with beta > 1
    PionsDca,                   ///< Number of raw pion-candidates
    ProtonsDca,                 ///< Number of raw proton-candidates
    PrimaryDca,                 ///< Number of raw proton-candidates
    Pions,                      ///< Number of pion-candidates
    Protons,                    ///< Number of proton-candidates
    EventsTotal,                ///< Total number of events
    EventsWoTzero,              ///< Number of events with undefined t-zero
    EventsLambdaCand,           ///< Events with at least one pion and one proton candidate
    KfpEventsLambdaCand,        ///< Events with lambda-candidates in KF-particle
    KfpLambdaCandidates,        ///< Number of lambda-candidates
    END
  };

  /// \enum  ETimer
  /// \brief Timer keys for the V0FinderMonitor
  /* clang-format off */
  enum class ETimer
  {
    ProcessEvent,           ///< Processing of a single event
      CollectT0,            ///< Collecting T0s
      CollectDca,           ///< Estimating DCAs
      FindV0Candidates,     ///< V0-finder procedure for a given t0
        PrepareContainers,  ///< Prepare data containers
        PreselectTracks,    ///< Track preselection
        InitKfp,            ///< Init KFParticleFinder inside the event
        ExecKfp,            ///< Run KFParticleFinder inside the event
    END
  };
  /* clang-format on */

  /// \brief Specification of ca::MonitorData for the V0Finder
  using V0FinderMonitorData_t = ca::MonitorData<ECounter, ETimer>;

  /// \class Monitor
  /// \brief A monitor for the V0Finder
  class V0FinderMonitor : public ca::Monitor<ECounter, ETimer> {
   public:
    /// \brief Default constructor
    V0FinderMonitor() : ca::Monitor<ECounter, ETimer>("V0 finder monitor")
    {
      SetCounterName(ECounter::TracksTotal, "all tracks");
      SetCounterName(ECounter::TracksSelected, "pre-selected tracks");
      SetCounterName(ECounter::TracksInfiniteParam, "tracks satisfying PID selection");
      SetCounterName(ECounter::TracksWoTofHits, "tracks w/o TOF hits");
      SetCounterName(ECounter::TracksWNegativeTofHitTime, "tracks w/ negative TOF time");
      SetCounterName(ECounter::TracksWoStsHits, "tracks w/o STS hits");
      SetCounterName(ECounter::TracksWoPid, "tracks w/o PID");
      SetCounterName(ECounter::TracksWoMomentum, "tracks w/o momentum");
      SetCounterName(ECounter::TracksWUnphysicalBeta, "tracks w/ beta > 1");
      SetCounterName(ECounter::PionsDca, "raw pion candidates");
      SetCounterName(ECounter::ProtonsDca, "raw proton candidates");
      SetCounterName(ECounter::PrimaryDca, "number of primary particles");
      SetCounterName(ECounter::Pions, "pion candidates");
      SetCounterName(ECounter::Protons, "proton candidates");
      SetCounterName(ECounter::EventsTotal, "all events");
      SetCounterName(ECounter::EventsWoTzero, "events w/o t0");
      SetCounterName(ECounter::EventsLambdaCand, "events passed to KFP");
      SetCounterName(ECounter::KfpEventsLambdaCand, "events w/ lambda candidates");
      SetCounterName(ECounter::KfpLambdaCandidates, "lambda candidates");

      SetTimerName(ETimer::ProcessEvent, "event processing");
      SetTimerName(ETimer::CollectT0, "t0 container preparation");
      SetTimerName(ETimer::CollectDca, "DCA container preparation");
      SetTimerName(ETimer::FindV0Candidates, "V0-candidates finding");
      SetTimerName(ETimer::PrepareContainers, "Container initialization");
      SetTimerName(ETimer::PreselectTracks, "Track preselection");
      SetTimerName(ETimer::InitKfp, "KFParticleFinder initialization");
      SetTimerName(ETimer::ExecKfp, "KFParticleFinder execution");

      SetRatioKeys({ECounter::EventsTotal, ECounter::TracksTotal, ECounter::TracksSelected});
    }

   private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<ca::Monitor<ECounter, ETimer>>(*this);
    }
  };

}  // namespace cbm::algo::kfp
