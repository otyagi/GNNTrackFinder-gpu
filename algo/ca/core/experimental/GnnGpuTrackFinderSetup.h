/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnGpuTrackFinderSetup.h
/// \brief The class is responsible for setting up the environment and the order in which kernels are launched to perform GNN tracking using XPU

#pragma once  // include this header only once per compilation unit

#include "CaGpuParameters.h"
#include "CaGpuTimeMonitor.h"
#include "CaHit.h"
#include "CaTrackFitter.h"
#include "CaVector.h"
#include "CaWindowData.h"
#include "CandClassifier.h"
#include "EmbedNet.h"
#include "GnnGpuGraphConstructor.h"
#include "KfTrackParam.h"
#include "MLPutil.h"

#include <xpu/host.h>

namespace cbm::algo::ca
{
  class GnnGpuTrackFinderSetup {
   public:
    ///                             ------  Constructors and destructor ------

    /// Constructor
    /// \param nThreads  Number of threads for multi-threaded mode
    GnnGpuTrackFinderSetup(ca::WindowData& wData, const ca::Parameters<fvec>& pars, const ca::InputData& input,
                           TrackFitter& trackFitter);

    /// Copy constructor
    GnnGpuTrackFinderSetup(const GnnGpuTrackFinderSetup&) = delete;

    /// Move constructor
    GnnGpuTrackFinderSetup(GnnGpuTrackFinderSetup&&) = delete;

    /// Copy assignment operator
    GnnGpuTrackFinderSetup& operator=(const GnnGpuTrackFinderSetup&) = delete;

    /// Move assignment operator
    GnnGpuTrackFinderSetup& operator=(GnnGpuTrackFinderSetup&&) = delete;

    /// Destructor
    ~GnnGpuTrackFinderSetup() = default;

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
    void RunGpuTracking();

    /// Load embed weights and set up for embedding hits
    void SetupGNN(const int iteration);

    /// Save doublets as tracks for debugging
    void SaveDoubletsAsTracks();

    /// Save triplets as tracks for debugging before fitting
    void SaveTripletsAsTracks();

    /// Save triplets as tracks for debugging after KF fitting
    void SaveFittedTripletsAsTracks();

    void CooperativeCompetitionCPU(std::vector<std::pair<std::vector<int>, float>>& trackAndScores);

    void CooperativeCompetitionGPU(std::vector<std::pair<std::vector<int>, float>>& trackAndScores);

    void FitTracklets(std::vector<std::vector<int>>& tracklets, std::vector<float>& trackletScores,
                      std::vector<std::vector<float>>& trackletFitParams);

    void FindTracks(const int iteration, const bool doCompetition);

    /// Get the number of triplets
    unsigned int GetNofTriplets() const { return fNTriplets; }

    /// Get timings
    XpuTimings& GetTimings() { return fEventTimeMonitor; }

   private:
    const Parameters<fvec>& fParameters;           ///< Object of Framework parameters class
    WindowData& frWData;                           ///< Reference to the window data
    xpu::queue fQueue;                             ///< GPU queue TODO: initialization is ~220 ms. Why and how to avoid?
    ca::GnnGpuGraphConstructor fGraphConstructor;  ///< GPU graph constructor
    TrackFitter& frTrackFitter;
    const ca::InputData& frInput;
    unsigned int fIteration;  ///< Iteration number

    int fNHits;                             ///< Number of active hits
    std::vector<ca::Hit> activeHits;        ///< active hits in this iteration
    std::vector<int> activeToWDataMapping;  ///< index of activeHit in window data

    int fNTriplets;  ///< Number of triplets

    const bool useCandClassifier_        = true;
    const float CandClassifierThreshold_ = 0.5;

    XpuTimings fEventTimeMonitor;
  };
}  // namespace cbm::algo::ca
