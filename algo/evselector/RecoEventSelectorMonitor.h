/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   RecoEventSelectorMonitor.h
/// \date   28.01.2025
/// \brief  A monitor for reco event selector
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CaMonitor.h"

#include <boost/serialization/base_object.hpp>

namespace cbm::algo::evselect
{
  /// \enum  ECounter
  /// \brief Counter keys for the event selector monitor
  enum class ECounter
  {
    Timeslices,        ///< number of processed timeslices
    EventsTotal,       ///< Total number of events processed
    EventsNeStsHits,   ///< Events with not enough STS hits
    EventsNeTofHits,   ///< Events with enough STS hits, but not enough TOF hits
    EventsNeBmonHits,  ///< Events with not enough BMon hits
    EventsNeTracks,    ///< Events with enough hits, but not enough tracks
    EventsSelected,    ///< Number of selected events
    LambdaCandidates,  ///< Number of lambda-candidates, returned by KFParticleFinder
    END
  };

  /// \enum  ETimer
  /// \brief Timer keys for the event selector monitor
  /* clang-format off */
  enum class ETimer {
    EventReconstruction,
      BmonHitFinder,
      StsHitFinder,
      TofHitFinder,
      TrdHitFinder,
      TrackFinder,
      V0Finder,
    END
  };
  /* clang-format on */

  /// \brief Specification of ca::MonitorData for the event selector
  using MonitorData_t = ca::MonitorData<ECounter, ETimer>;

  /// \class Monitor
  /// \brief A monitor for the event selector
  class Monitor : public ca::Monitor<ECounter, ETimer> {
   public:
    /// \brief Default constructor
    Monitor() : ca::Monitor<ECounter, ETimer>("Event-selector Monitor")
    {
      SetCounterName(ECounter::Timeslices, "processed timeslices");
      SetCounterName(ECounter::EventsTotal, "total events");
      SetCounterName(ECounter::EventsNeStsHits, "events discarded by N STS hits");
      SetCounterName(ECounter::EventsNeTofHits, "events discarded by N TOF hits");
      SetCounterName(ECounter::EventsNeBmonHits, "events discarded by N BMon hits");
      SetCounterName(ECounter::EventsNeTracks, "events discarded by N tracks");
      SetCounterName(ECounter::LambdaCandidates, "potential lambda candidates");
      SetCounterName(ECounter::EventsSelected, "selected events");

      SetTimerName(ETimer::EventReconstruction, "event reconstruction");
      SetTimerName(ETimer::BmonHitFinder, "hit finding in Bmon");
      SetTimerName(ETimer::StsHitFinder, "hit finding in STS");
      SetTimerName(ETimer::TofHitFinder, "hit finding in TOF");
      SetTimerName(ETimer::TrdHitFinder, "hit finding in TRD");
      SetTimerName(ETimer::TrackFinder, "track finding");
      SetTimerName(ETimer::V0Finder, "V0 finding");

      SetRatioKeys({ECounter::Timeslices});
    }

   private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<ca::Monitor<ECounter, ETimer>>(*this);
    }
  };

}  // namespace cbm::algo::evselect
