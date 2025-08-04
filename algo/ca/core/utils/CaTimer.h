/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaTimer.h
/// \brief  Timer class for CA tracking (header)
/// \since  18.10.2023
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include <boost/serialization/serialization.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace cbm::algo::ca
{
  /// \class  Timer
  /// \brief  A timer class for the monitor
  ///
  class Timer {
   public:
    using Clock         = std::chrono::high_resolution_clock;
    using Duration      = std::chrono::nanoseconds;
    using DurationCount = Duration::rep;
    using TimePoint     = std::chrono::time_point<Clock, Duration>;

    /// \brief Default constructor
    Timer() = default;

    /// \brief Destructor
    ~Timer() = default;

    /// \brief Copy constructor
    Timer(const Timer&) = default;

    /// \brief Move constructor
    Timer(Timer&&) = default;

    /// \brief Copy assignment operator
    Timer& operator=(const Timer&) = default;

    /// \brief Move assignment operator
    Timer& operator=(Timer&&) = default;

    /// \brief Adds another timer
    /// \param other    Reference to the other Timer object to add
    /// \param parallel Bool: if the timers were executed in parallel
    ///
    /// If the parallel flag is true then the resulting fTotal time is taken as a maximum of each total time of
    /// the appended timers. If the parallel flag is false, the resulting fTotal is a sum of all timers.
    void AddTimer(const Timer& other, bool parallel);

    /// \brief Gets average time [s]
    double GetAverage() const { return static_cast<double>(fTotal) / fNofCalls * 1.e-9; }

    /// \brief Gets time of the longest call [s]
    double GetMax() const { return static_cast<double>(fMax) * 1.e-9; }

    /// \brief Gets time of the shortest call [s]
    double GetMin() const { return static_cast<double>(fMin) * 1.e-9; }

    /// \brief Gets number of calls
    int GetNofCalls() const { return fNofCalls; }

    /// \brief Gets index of the longest call
    int GetMaxCallIndex() const { return fMaxCallIndex; }

    /// \brief Gets index of the longest call
    int GetMinCallIndex() const { return fMinCallIndex; }

    /// \brief Gets total time [s]
    double GetTotal() const { return static_cast<double>(fTotal) * 1.e-9; }

    /// \brief Gets total time [ms]
    double GetTotalMs() const { return GetTotal() * 1.e3; }

    /// \brief Resets the timer
    void Reset();

    /// \brief Starts the timer
    void Start() { fStart = Clock::now(); }

    /// \brief Stops the timer
    void Stop();

   private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fMin;
      ar& fMax;
      ar& fTotal;
      ar& fNofCalls;
      ar& fMinCallIndex;
      ar& fMaxCallIndex;
    }

    TimePoint fStart     = TimePoint();
    DurationCount fMin   = std::numeric_limits<DurationCount>::max();  ///< Minimal time period
    DurationCount fMax   = std::numeric_limits<DurationCount>::min();  ///< Maximal time period
    DurationCount fTotal = DurationCount(0);                           ///< Total measured time period [ns]
    int fNofCalls        = 0;                                          ///< Number of timer calls [ns]
    int fMinCallIndex    = -1;                                         ///< Index of the shortest call [ns]
    int fMaxCallIndex    = -1;                                         ///< Index of the longest call [ns]
  };                                                                   // class Timer


  // ************************************
  // **   Inline function definition   **
  // ************************************

  // -------------------------------------------------------------------------------------------------------------------
  //
  inline void Timer::AddTimer(const Timer& other, bool parallel)
  {
    if (other.fMin < fMin) {
      fMin          = other.fMin;
      fMinCallIndex = other.fMinCallIndex + fNofCalls;
    }
    if (other.fMax > fMax) {
      fMax          = other.fMax;
      fMaxCallIndex = other.fMinCallIndex + fNofCalls;
    }
    if (parallel) {
      if (fTotal < other.fTotal) {
        fTotal = other.fTotal;
      }
    }
    else {
      fTotal += other.fTotal;
    }
    fNofCalls += other.fNofCalls;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  inline void Timer::Reset()
  {
    fStart        = TimePoint();
    fMin          = std::numeric_limits<DurationCount>::max();
    fMax          = std::numeric_limits<DurationCount>::min();
    fMinCallIndex = -1;
    fMaxCallIndex = -1;
    fTotal        = DurationCount(0);
    fNofCalls     = 0;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  inline void Timer::Stop()
  {
    auto stop = Clock::now();
    auto time = std::chrono::duration_cast<Duration>(stop - fStart).count();
    if (fMin > time) {
      fMin          = time;
      fMinCallIndex = fNofCalls;
    }
    if (fMax < time) {
      fMax          = time;
      fMaxCallIndex = fNofCalls;
    }
    fTotal += time;
    ++fNofCalls;
  }
}  // namespace cbm::algo::ca
