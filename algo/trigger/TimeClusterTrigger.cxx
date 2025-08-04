/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Dominik Smith */

#include "TimeClusterTrigger.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>
#include <vector>

#include <xpu/host.h>

using std::string;
using std::stringstream;
using std::vector;

namespace cbm::algo::evbuild
{

  TimeClusterTrigger::resultType TimeClusterTrigger::operator()(const vector<double>& dataVec) const
  {

    if (!std::is_sorted(dataVec.begin(), dataVec.end())) throw std::runtime_error("TimeClusterTrigger: unsorted input");

    xpu::push_timer("TimeClusterTrigger");
    xpu::t_add_bytes(dataVec.size() * sizeof(double));

    // --- Output data
    resultType result                      = {};
    vector<double>& triggerVec             = result.first;
    TimeClusterTriggerMonitorData& monitor = result.second;

    auto winStart = dataVec.begin();
    auto current  = dataVec.begin();

    while (current != dataVec.end()) {

      // If window size is exceeded, adjust window start
      while (*current - *winStart > fWinSize)
        winStart++;

      // Create trigger if threshold is reached
      if (std::distance(winStart, current) >= fMinNumData - 1) {
        triggerVec.push_back(0.5 * (*current + *winStart));

        // Increment monitoring counter with number of digis used
        monitor.numInTrigger += std::distance(winStart, current) + 1;

        // Start new window after dead time
        winStart = current + 1;
        while (winStart != dataVec.end() && *winStart - *current <= fDeadTime)
          winStart++;
        current = winStart;
      }

      // If threshold is not reached, check with next element
      else
        current++;
    }
    // Store number of input data for monitoring
    monitor.num += dataVec.size();
    monitor.nTriggers = triggerVec.size();

    monitor.time = xpu::pop_timer();

    return result;
  }


  string TimeClusterTrigger::ToString() const
  {
    stringstream out;
    out << "--- Using TimeClusterTrigger with trigger window " << fWinSize << " ns";
    out << ", threshold " << fMinNumData;
    out << ", dead time " << fDeadTime << " ns";
    return out.str();
  }

}  // namespace cbm::algo::evbuild
