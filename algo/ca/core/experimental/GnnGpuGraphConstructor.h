/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnGpuGraphConstructor.h
/// \brief The class contains data and kernels for running GNN tracking on CPU and GPU using XPU libraries

#pragma once  // include this header only once per compilation unit

#include "CaDeviceImage.h"
#include "CaGpuGrid.h"
#include "CaGpuGridArea.h"
#include "CaGpuMaterialMap.h"
#include "CaGpuParameters.h"
#include "CaHit.h"
#include "CaMeasurementXy.h"
#include "CaTriplet.h"
#include "KfMeasurementU.h"

#include <xpu/device.h>
#include <xpu/host.h>

namespace cbm::algo
{
  //  static constexpr int kMaxNofStations = 24;
  // Block sizes / other compile time constants that need tuning
  enum GnnGpuConstants {
#if XPU_IS_CUDA
    kSingletConstructorBlockSize      = 512,
    kSingletConstructorItemsPerThread = 8,
#else  // HIP, values ignored on CPU
    kEmbedHitsBlockSize               = 64,
#endif
  };
}  // namespace cbm::algo

namespace cbm::algo::ca
{
  //  class Framework;
  class GnnGpuGraphConstructor;

  // Declare Constant Memory
  struct strGnnGpuGraphConstructor : xpu::constant<GPUReco, GnnGpuGraphConstructor> {};

  // Declare Kernels
  struct EmbedHits : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;
    XPU_D void operator()(context&);
  };

  struct GnnIterationData {
    int fNHits;              ///< Number of hits in grid
    int fIteration;          ///< Iteration number
    int fNDoublets;          ///< Number of doublets
    int fNDoublets_counter;  ///< Number of doublets counter
    int fNTriplets;          ///< Number of triplets
    int fNTriplets_counter;  ///< Number of triplets counter
  };

  class GnnGpuGraphConstructor {
   public:
    ///                             ------  FUNCTIONAL PART ------
    XPU_D void EmbedHits(EmbedHits::context&) const;

   private:
    // XPU_D void CollectHits(int iThread, int iSta, int iHit, int maxNHits, int* collectedHits) const;

    ///                          ------  DATA MEMBERS ------
   public:
    xpu::buffer<ca::GpuGrid> fvGpuGrid;  ///< Grid

    xpu::buffer<unsigned int> fgridFirstBinEntryIndex;  ///< Index of the first entry in the grid

    xpu::buffer<unsigned int> fgridEntries;  ///< Entries in the grid	//TODO: check if it should be ca::HitIndex_t

    ///Material map
    xpu::buffer<ca::GpuMaterialMap> fMaterialMap;  ///< Material map base objects
    xpu::buffer<float> fMaterialMapTables;         ///< Material map tables

    xpu::buffer<GnnIterationData> fIterationData;  ///< Temporary data

    /// \brief Hits of the current time window
    /// It is a portion of fInputData to process in the current time window
    /// hit.Id is replaced by the hit index in fInputData
    xpu::buffer<ca::Hit> fvHits;

    xpu::buffer<kf::TrackParamBase<float>> fvTrackParams;  ///< Track parameters

    xpu::buffer<ca::Triplet> fvTriplets;  ///< Triplets

    //    GpuParameters fParams;
    xpu::buffer<GpuParameters> fParams;

    int fIteration;

    GpuParameters fParams_const[4];

    ca::GpuStation fStations_const[constants::gpu::MaxNofStations];
  };

}  // namespace cbm::algo::ca
