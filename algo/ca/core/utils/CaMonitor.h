/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaMonitor.h
/// \brief  CA Tracking monitor class
/// \since  25.08.2023
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaEnumArray.h"
#include "CaMonitorData.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>


namespace cbm::algo::ca
{
  /// \class  Monitor
  /// \brief  Monitor class for the CA tracking
  /// \tparam ECounterKey  A enum class, containing keys for monitorables
  /// \tparam ETimerKey    A enum class, containing keys for timers
  ///
  template<class ECounterKey, class ETimerKey = EDummy>
  class Monitor {
   public:
    /// \brief Alias to array, indexed by the monitorable enumeration
    template<typename T>
    using CounterArray = EnumArray<ECounterKey, T>;

    /// \brief Alias to array, indexed by the timer enumeration
    template<typename T>
    using TimerArray = EnumArray<ETimerKey, T>;

    /// \brief Constructor
    /// \param name Name of monitor
    Monitor(std::string_view name) : fsName(name) {}

    /// \brief Default constructor
    Monitor() = default;

    /// \brief Destructor
    ~Monitor() = default;

    /// \brief Copy constructor
    Monitor(const Monitor&) = delete;

    /// \brief Move constructor
    Monitor(Monitor&&) = delete;

    /// \brief Copy assignment operator
    Monitor& operator=(const Monitor&) = delete;

    /// \brief Move assignment operator
    Monitor& operator=(Monitor&&) = delete;

    /// \brief Adds the other monitor data to this
    /// \param data     Reference to the other MonitorData object to add
    /// \param parallel True, if the monitor data were obtained in parallel (See Timer::AddMonitorData)
    void AddMonitorData(const MonitorData<ECounterKey, ETimerKey>& data, bool parallel = false)
    {
      fMonitorData.AddMonitorData(data, parallel);
    }

    /// \brief Gets counter name
    /// \param key Counter key
    const std::string& GetCounterName(ECounterKey key) const { return faCounterNames[key]; }

    /// \brief Gets counter value
    /// \param key
    int GetCounterValue(ECounterKey key) const { return fMonitorData.GetCounterValue(key); }

    /// \brief Gets monitor data
    const MonitorData<ECounterKey, ETimerKey>& GetMonitorData() const { return fMonitorData; }

    /// \brief Gets name of the monitor
    const std::string& GetName() const { return fsName; }

    /// \brief Gets timer
    /// \param key Timer key
    const Timer& GetTimer(ETimerKey key) const { return fMonitorData.GetTimer(key); }

    /// \brief Gets timer name
    /// \param key Timer key
    const std::string& GetTimerName(ETimerKey key) const { return faTimerNames[key]; }

    /// \brief Increments key counter by 1
    /// \param key Counter key
    void IncrementCounter(ECounterKey key) { fMonitorData.IncrementCounter(key); };

    /// \brief Increments key counter by a number
    /// \param key  Counter key
    /// \param num  Number to add
    void IncrementCounter(ECounterKey key, int num) { fMonitorData.IncrementCounter(key, num); }

    /// \brief Resets the counters
    void Reset() { fMonitorData.Reset(); }

    /// \brief Sets keys of counters, which are used as denominators for ratios
    /// \param vKeys Vector of keys
    void SetRatioKeys(const std::vector<ECounterKey>& vKeys) { fvCounterRatioKeys = vKeys; }

    /// \brief Sets name of counter
    /// \param name  Name of counter
    void SetCounterName(ECounterKey key, std::string_view name) { faCounterNames[key] = name; }

    /// \brief Sets monitor data
    /// \param data  The MonitorData class objet
    void SetMonitorData(const MonitorData<ECounterKey, ETimerKey>& data) { fMonitorData = data; }

    /// \brief Sets name of the monitor
    /// \param name  Name of the monitor
    void SetName(std::string_view name) { fsName = name; }

    /// \brief Sets name of the timer
    /// \param name  Name of the timer
    void SetTimerName(ETimerKey key, std::string_view name) { faTimerNames[key] = name; }

    /// \brief Starts timer
    /// \param key  Timer key
    void StartTimer(ETimerKey key) { fMonitorData.StartTimer(key); }

    /// \brief Stops timer
    /// \param key  Timer key
    void StopTimer(ETimerKey key) { fMonitorData.StopTimer(key); }

    /// \brief Prints counters summary to string
    std::string ToString() const;

    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fMonitorData;
      ar& faTimerNames;
      ar& faCounterNames;
      ar& fvCounterRatioKeys;
      ar& fsName;
    }

   private:
    MonitorData<ECounterKey, ETimerKey> fMonitorData;  ///< Monitor data
    TimerArray<std::string> faTimerNames{};            ///< Array of timer names
    CounterArray<std::string> faCounterNames{};        ///< Array of counter names
    std::vector<ECounterKey> fvCounterRatioKeys{};     ///< List of keys, which are used as denominators in ratios
    std::string fsName;                                ///< Name of the monitor
  };


  // *****************************************
  // **  Template function implementations  **
  // *****************************************

  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<class ECounterKey, class ETimerKey>
  std::string Monitor<ECounterKey, ETimerKey>::ToString() const
  {
    using std::left;
    using std::right;
    using std::setfill;
    using std::setw;
    std::stringstream msg;
    constexpr size_t width    = 24;
    auto Cmp                  = [](const std::string& l, const std::string& r) { return l.size() < r.size(); };
    msg << "\n===== Monitor: " << fsName << "\n";
    if constexpr (!std::is_same_v<ETimerKey, EDummy>) {
      size_t widthKeyTimer{std::max_element(faTimerNames.begin(), faTimerNames.end(), Cmp)->size() + 1};
      msg << "\n----- Timers:\n";
      msg << setw(widthKeyTimer) << left << "Key" << ' ';
      msg << setw(width) << left << "N Calls" << ' ';
      msg << setw(width) << left << "Average [s]" << ' ';
      msg << setw(width) << left << "Min [s]" << ' ';
      msg << setw(width) << left << "Max [s]" << ' ';
      msg << setw(width) << left << "Total [s]" << '\n';
      msg << right;
      for (int iKey = 0; iKey < fMonitorData.GetNofTimers(); ++iKey) {
        const Timer& timer = fMonitorData.GetTimer(static_cast<ETimerKey>(iKey));
        msg << setw(widthKeyTimer) << faTimerNames[iKey] << ' ';
        msg << setw(width) << timer.GetNofCalls() << ' ';
        msg << setw(width) << timer.GetAverage() << ' ';
        msg << setw(width) << timer.GetMin() << ' ';
        msg << setw(width) << timer.GetMax() << ' ';
        msg << setw(width) << timer.GetTotal() << '\n';
      }
    }

    msg << "\n----- Counters:\n";
    size_t widthKeyCounter{std::max_element(faCounterNames.begin(), faCounterNames.end(), Cmp)->size() + 1};
    msg << setw(widthKeyCounter) << left << "Key" << ' ';
    msg << setw(width) << left << "Total" << ' ';
    for (auto key : fvCounterRatioKeys) {
      msg << setw(width) << left << std::string("per ") + faCounterNames[key] << ' ';
    }
    msg << '\n';
    for (int iKey = 0; iKey < fMonitorData.GetNofCounters(); ++iKey) {
      auto counterValue = fMonitorData.GetCounterValue(static_cast<ECounterKey>(iKey));
      msg << setw(widthKeyCounter) << left << faCounterNames[iKey] << ' ';
      msg << setw(width) << right << counterValue << ' ';
      for (auto keyDen : fvCounterRatioKeys) {
        auto ratio = static_cast<double>(counterValue) / fMonitorData.GetCounterValue(keyDen);
        msg << setw(width) << right << ratio << ' ';
      }
      msg << '\n';
    }
    return msg.str();
  }
}  // namespace cbm::algo::ca
