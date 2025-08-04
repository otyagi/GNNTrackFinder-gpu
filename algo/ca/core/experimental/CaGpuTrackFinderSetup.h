/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file CaGpuTrackFinderSetup.h
/// \brief The class is responsible for setting up the environment and the order in which kernels are launched to perform tracking using XPU

#pragma once  // include this header only once per compilation unit

#include "CaBranch.h"
#include "CaGpuParameters.h"
#include "CaGpuTimeMonitor.h"
#include "CaGpuTripletConstructor.h"
#include "CaHit.h"
#include "CaVector.h"
#include "CaWindowData.h"
#include "KfTrackParam.h"

#include <xpu/host.h>

namespace cbm::algo::ca
{
  class GpuTrackFinderSetup {
   public:
    ///                             ------  Constructors and destructor ------

    /// Constructor
    /// \param nThreads  Number of threads for multi-threaded mode
    GpuTrackFinderSetup(ca::WindowData& wData, const ca::Parameters<fvec>& pars);

    /// Copy constructor
    GpuTrackFinderSetup(const GpuTrackFinderSetup&) = delete;

    /// Move constructor
    GpuTrackFinderSetup(GpuTrackFinderSetup&&) = delete;

    /// Copy assignment operator
    GpuTrackFinderSetup& operator=(const GpuTrackFinderSetup&) = delete;

    /// Move assignment operator
    GpuTrackFinderSetup& operator=(GpuTrackFinderSetup&&) = delete;

    /// Destructor
    ~GpuTrackFinderSetup() = default;

    ///                             ------  Public member functions ------

    ///Set the GPU tracking parameters
    void SetupParameters();

    /// Set the input grid data for the GPU tracking
    void SetupGrid();

    /// Set the material map for the GPU tracking
    void SetupMaterialMap();

    /// Set the input data for the GPU tracking
    void SetInputData();

    /// Set the iteration data
    void SetupIterationData(int iter);

    /// Run the track finding algorithm chain
    void RunGpuTrackingSetup();

    /// Get the number of triplets
    unsigned int GetNofTriplets() const { return fNTriplets; }  //fTripletConstructor.fTempData[0].fNTriplets; }

    ca::Triplet& GetTriplet(int itr) { return fTripletConstructor.fvTriplets[itr]; }  //TODO: switch back
    //    ca::Triplet& GetTriplet(int itr) { return fvTriplets[itr]; }//fTripletConstructor.fvTriplets[itr]; }	//TODO: switch back

    /// Get timings
    XpuTimings& GetTimings() { return fEventTimeMonitor; }

    /// Get triplets
    //    const Vector<ca::Triplet>& GetTriplets() const { return fTripletConstructor.fvTriplets; }

    //    static constexpr bool fGPU_time_monitoring = true;  // print debug info

   private:
    const Parameters<fvec>& fParameters;  ///< Object of Framework parameters class
    WindowData& frWData;                  ///< Reference to the window data
    xpu::queue fQueue;                    ///< GPU queue TODO: initialization is ~220 ms. Why and how to avoid?
    ca::GpuTripletConstructor fTripletConstructor;  ///< GPU triplet constructor
    unsigned int fIteration;                        ///< Iteration number
    int fNHits;                                     ///< Number of hits

    int fNTriplets;  ///< Number of triplets

    XpuTimings fEventTimeMonitor;
  };
}  // namespace cbm::algo::ca
