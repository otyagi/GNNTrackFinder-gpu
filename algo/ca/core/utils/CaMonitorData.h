/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaMonitorData.h
/// \brief  A block of data for cbm::algo::ca::Monitor
/// \since  19.10.2023
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "CaEnumArray.h"
#include "CaTimer.h"

#include <boost/serialization/access.hpp>

#include <algorithm>

namespace cbm::algo::ca
{
  /// \class MonitorData
  /// \brief Monitor data block
  /// \tparam ECounterKey  A enum class, containing keys for monitorables
  /// \tparam ETimerKey    A enum class, containing keys for timers
  ///
  template<class ECounterKey, class ETimerKey>
  class MonitorData {
   public:
    /// \brief Alias to array, indexed by the counter enumeration
    template<typename T>
    using CounterArray = EnumArray<ECounterKey, T>;

    /// \brief Alias to array, indexed by the timer enumeration
    template<typename T>
    using TimerArray = EnumArray<ETimerKey, T>;

    /// \brief Default constructor
    MonitorData() = default;

    /// \brief Copy constructor
    MonitorData(const MonitorData&) = default;

    /// \brief Move constructor
    MonitorData(MonitorData&&) = default;

    /// \brief Destructor
    ~MonitorData() = default;

    /// \brief Copy assignment operator
    MonitorData& operator=(const MonitorData&) = default;

    /// \brief Move assignment operator
    MonitorData& operator=(MonitorData&&) = default;

    /// \brief Adds the other monitor data to this
    /// \param other    Reference to the other MonitorData object to add
    /// \param parallel If the monitors were filled in parallel (See CaTimer::AddTimer)
    void AddMonitorData(const MonitorData& other, bool parallel = false);

    /// \brief Gets counter value
    /// \param key
    int GetCounterValue(ECounterKey key) const { return faCounters[key]; }

    /// \brief Gets number of counters
    int GetNofCounters() const { return static_cast<int>(ECounterKey::END); }

    /// \brief Gets number of timers
    int GetNofTimers() const { return static_cast<int>(ETimerKey::END); }

    /// \brief Gets timer
    const Timer& GetTimer(ETimerKey key) const { return faTimers[key]; }

    /// \brief Increments key counter by 1
    /// \param key Counter key
    void IncrementCounter(ECounterKey key) { ++faCounters[key]; };

    /// \brief Increments key counter by a number
    /// \param key  Counter key
    /// \param num  Number to add
    void IncrementCounter(ECounterKey key, int num) { faCounters[key] += num; }

    /// \brief Resets all the counters and timers
    void Reset();

    /// \brief Resets a particular counter
    /// \param key  Counter key
    void ResetCounter(ECounterKey key) { faCounters[key] = 0; }

    /// \brief Starts timer
    /// \param key  Timer key
    void StartTimer(ETimerKey key) { faTimers[key].Start(); }

    /// \brief Stops timer
    /// \param key  Timer key
    void StopTimer(ETimerKey key) { faTimers[key].Stop(); }

   private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& faTimers;
      ar& faCounters;
    }

    TimerArray<Timer> faTimers{};    ///< Array of timers
    CounterArray<int> faCounters{};  ///< Array of counters
  };


  // *****************************************
  // **  Template function implementations  **
  // *****************************************

  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<class ECounterKey, class ETimerKey>
  inline void MonitorData<ECounterKey, ETimerKey>::AddMonitorData(const MonitorData<ECounterKey, ETimerKey>& other,
                                                                  bool parallel)
  {
    for (size_t iCounter = 0; iCounter < faCounters.size(); ++iCounter) {
      faCounters[iCounter] += other.faCounters[iCounter];
    }
    for (size_t iTimer = 0; iTimer < faTimers.size(); ++iTimer) {
      faTimers[iTimer].AddTimer(other.faTimers[iTimer], parallel);
    }
  }

  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<class ECounterKey, class ETimerKey>
  inline void MonitorData<ECounterKey, ETimerKey>::Reset()
  {
    faCounters.fill(0);
    std::for_each(faTimers.begin(), faTimers.end(), [](auto& timer) { timer.Reset(); });
  }
}  // namespace cbm::algo::ca
