/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file GpuTimeMonitor.h
///
/// \brief Time monitor for GPU kernels within the CA algorithm

#pragma once  // include this header only once per compilation unit

#include <xpu/host.h>

namespace cbm::algo::ca
{

  namespace
  {
    using namespace cbm::algo;
  }

  struct XpuTimings {
    // CA time monitors
    xpu::timings PrepareData_time[4];
    xpu::timings MakeSinglets_time[4];
    xpu::timings MakeDoublets_time[4];
    xpu::timings CompressDoublets_time[4];
    xpu::timings ResetDoublets_time[4];
    xpu::timings FitDoublets_time[4];
    xpu::timings MakeTriplets_time[4];
    xpu::timings CompressTriplets_time[4];
    xpu::timings ResetTriplets_time[4];
    xpu::timings FitTriplets_time[4];
    xpu::timings SortTriplets_time[4];
    xpu::timings Total_time[4];

    // Gnn time monitors
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

      print_timing("PrepareData_time", PrepareData_time[iteration]);
      print_timing("MakeSinglets_time", MakeSinglets_time[iteration]);
      print_timing("MakeDoublets_time", MakeDoublets_time[iteration]);
      print_timing("CompressDoublets_time", CompressDoublets_time[iteration]);
      print_timing("ResetDoublets_time", ResetDoublets_time[iteration]);
      print_timing("FitDoublets_time", FitDoublets_time[iteration]);
      print_timing("MakeTriplets_time", MakeTriplets_time[iteration]);
      print_timing("CompressTriplets_time", CompressTriplets_time[iteration]);
      print_timing("ResetTriplets_time", ResetTriplets_time[iteration]);
      print_timing("FitTriplets_time", FitTriplets_time[iteration]);
      print_timing("SortTriplets_time", SortTriplets_time[iteration]);
      print_timing("Total_time", Total_time[iteration]);

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

    void AddEventInfo(XpuTimings& event)
    {
      xpuTimings.push_back(event);
      nEvents++;
    }

    void SetNEvents(int nEv) { this->nEvents = nEv; }

    int GetNEvents() { return nEvents; }

    xpu::timings GetMakeSingletsTime(int ev, int it) { return xpuTimings[ev].MakeSinglets_time[it]; }

    xpu::timings GetMakeDoubletsTime(int ev, int it) { return xpuTimings[ev].MakeDoublets_time[it]; }

    xpu::timings GetCompressDoubletsTime(int ev, int it) { return xpuTimings[ev].CompressDoublets_time[it]; }

    xpu::timings GetFitDoubletsTime(int ev, int it) { return xpuTimings[ev].FitDoublets_time[it]; }

    xpu::timings GetMakeTripletsTime(int ev, int it) { return xpuTimings[ev].MakeTriplets_time[it]; }

    xpu::timings GetCompressTripletsTime(int ev, int it) { return xpuTimings[ev].CompressTriplets_time[it]; }

    xpu::timings GetFitTripletsTime(int ev, int it) { return xpuTimings[ev].FitTriplets_time[it]; }

    xpu::timings GetTotalTime(int ev, int it) { return xpuTimings[ev].Total_time[it]; }

    xpu::timings GetMetricLearningTime(int ev, int it) { return xpuTimings[ev].MetricLearning_time[it]; }

   private:
    std::vector<XpuTimings> xpuTimings;
    int nEvents;
  };

}  // namespace cbm::algo::ca
