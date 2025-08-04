/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Dominik Smith */

#ifndef CBM_ALGO_TIMECLUSTERTRIGGER_H
#define CBM_ALGO_TIMECLUSTERTRIGGER_H 1

#include "Definitions.h"
#include "DigiTriggerConfig.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::evbuild
{

  /** @struct TimeClusterTriggerMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 23 Jun 2023
   ** @brief Monitoring data for time cluster trigger algorithm
   **/
  struct TimeClusterTriggerMonitorData {
    size_t numInTrigger = 0;  ///< Time stamps used for trigger building
    size_t num          = 0;  ///< Total time stamps in input
    size_t nTriggers    = 0;  ///< Number of triggers
    xpu::timings time;        ///< Time for trigger building
  };

  /** @class TimeClusterTrigger
   ** @author Volker Friese <v.friese@gsi.de>
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 21.11.2021
   ** @brief Finds clusters in time-series data
   **
   ** A trigger is generated when the number density of data exceeds a given threshold. Each datum
   ** can contribute to only one trigger. Consecutive triggers are separated by at least the dead time.
   **
   ** The input vector must be sorted, otherwise the behaviour is undefined.
   **/
  class TimeClusterTrigger {

   public:
    typedef std::pair<std::vector<double>, TimeClusterTriggerMonitorData> resultType;

    /** @brief Constructor
     ** @param  winSize     Size of trigger window
     ** @param  minNumData  Threshold on number of data within the trigger window
     ** @param  deadTime    Minimum time between two triggers
     **/
    TimeClusterTrigger(double winSize, u32 minNumData, double deadTime)
      : fWinSize(winSize)
      , fMinNumData(minNumData)
      , fDeadTime(deadTime)
    {
    }

    /** @brief Execution
     ** @param  dataVec     Source data vector
     ** @return Vector of trigger times and monitoring data
     **/
    resultType operator()(const std::vector<double>& dataVec) const;

    /** @brief Info to string **/
    std::string ToString() const;


   private:
    double fWinSize     = 0.;
    int32_t fMinNumData = 0;
    double fDeadTime    = 0.;
  };


}  // namespace cbm::algo::evbuild

#endif /* CBM_ALGO_TIMECLUSTERTRIGGER_H */
