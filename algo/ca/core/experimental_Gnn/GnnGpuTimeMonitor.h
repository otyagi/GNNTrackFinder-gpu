/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file GnnGpuTimeMonitor.h
///
/// \brief Time monitor for GPU kernels within the GNN algorithm

#pragma once  // include this header only once per compilation unit

#include <xpu/host.h>

#include <iomanip> // for setw

namespace cbm::algo::ca
{

  namespace
  {
    using namespace cbm::algo;
  }

  struct XpuTimings_Gnn {
    xpu::timings MetricLearning_time[4];

    int nIterations;

    void PrintTimings(int iteration) const
    {
      if (iteration < 0 || iteration >= 4) {
        std::cerr << "Error: wrong iteration number: " << iteration << std::endl;
        return;
      }

      auto print_timing = [](const std::string& name, const xpu::timings& t) {
        std::cout << std::left << std::setw(25) << name << "Kernel time: " << std::setw(10) << t.kernel_time()
                  << " H2D copy: " << std::setw(10) << t.copy(xpu::h2d) << " D2H copy: " << std::setw(10)
                  << t.copy(xpu::d2h) << " Memset: " << std::setw(10) << t.memset() << " Total: " << std::setw(10)
                  << t.wall() << std::endl;
      };

      print_timing("MetricLearning_time", MetricLearning_time[iteration]);

    }
  };

  ///
  class GpuTimeMonitor {
   public:
    /// \brief  Default constructor
    GpuTimeMonitor() : nEvents(0) {}

    /// \brief Destructor
    ~GpuTimeMonitor() = default;

    void AddEventInfo(XpuTimings_Gnn& event)
    {
      xpuTimings.push_back(event);
      nEvents++;
    }

   private:
    std::vector<XpuTimings_Gnn> xpuTimings;
    int nEvents;
  };

}  // namespace cbm::algo::ca
