/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#pragma once  // include this header only once per compilation unit

#include "DigiTriggerConfig.h"
#include "RecoResults.h"
#include "TimeClusterTrigger.h"

#include <utility>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::evbuild
{

  /** @class HitMultTrigger
   ** @brief Trigger class for finding time clusters of hit data
   ** @author Volker Friese <v.friese@gsi.de>
   ** @date 30 April 2024
   **
   ** The class takes as input an array of detector hits. It finds clusters of hits in time, using the generic TimeClusterTrigger algorithm.
   **/
  class HitMultTrigger {

   public:
    typedef std::pair<std::vector<double>, TimeClusterTriggerMonitorData> Result;

    /** @brief Constructor
     ** @param config Trigger configuration
     **/
    HitMultTrigger(const DigiTriggerConfig& config)
      : fConfig(config)
      , fAlgo(config.Window(), config.Threshold(), config.DeadTime()){};

    /** @brief Execution
     ** @param  recoData    Container of reco data
     ** @return Vector of trigger times and monitoring data
     **/
    Result operator()(const RecoResults& recoData) const;

    /** @brief Info to string **/
    std::string ToString() const;

   private:  // methods
    /** @brief Extract the hit time stamps for the selected trigger detector 
     ** @param recoData  Container of reco results
     ** @param system    Trigger Detector
     ** @return Sorted vector of hit time stamps
     **/
    std::vector<double> GetHitTimes(const RecoResults& recoData, ECbmModuleId system) const;

    /** @brief Get vector of time stamps from a data container [template]
     ** @param data Data container
     ** @return Sorted vector of time stamps
     **
     ** The template argument class must implement the method Time().
     **/
    template<class T>
    std::vector<double> GetTimeStamps(const gsl::span<const T> data) const
    {
      std::vector<double> result;
      result.resize(data.size());
      std::transform(data.begin(), data.end(), result.begin(), [](const T& obj) { return obj.Time(); });
      std::sort(result.begin(), result.end());
      return result;
    }


   private:                     // members
    DigiTriggerConfig fConfig;  ///< Configuration
    TimeClusterTrigger fAlgo;   ///< Algorithm
  };

}  // namespace cbm::algo::evbuild
