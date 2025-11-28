/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnGpuGraphConstructor.h
/// \brief The class contains data and kernels for running GNN tracking on CPU and GPU using XPU libraries

#pragma once  // include this header only once per compilation unit

#include "CaDeviceImage.h"
#include "CaGpuField.h"
#include "CaGpuGrid.h"
#include "CaGpuGridArea.h"
#include "CaGpuMaterialMap.h"
#include "CaGpuParameters.h"
#include "CaHit.h"
#include "CaMeasurementXy.h"
#include "CaTriplet.h"
#include "GnnGpuEmbedNet.h"
#include "KfGpuTrackKalmanFilter.h"
#include "KfMeasurementU.h"

#include <xpu/device.h>
#include <xpu/host.h>

namespace cbm::algo
{
  // Block sizes / other compile time constants that need tuning
  enum GnnGpuConstants {
#if XPU_IS_CUDA
    kSingletConstructorBlockSize      = 512,
    kSingletConstructorItemsPerThread = 8,
#else  // HIP, values ignored on CPU
    kEmbedHitsBlockSize   = 64,
    kScanBlockSize        = 1024,
    kCompressionBlockSize = 64,
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
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx);
  };

  struct NearestNeighbours_FastPrim : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx);
  };

  struct NearestNeighbours_Other : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx, int);
  };

  struct MakeTripletsOT_FastPrim : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx);
  };

  struct MakeTripletsOT_Other : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx, int);
  };

  struct FitTripletsOT_FastPrim : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx);
  };

  struct FitTripletsOT_Other : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx);
  };

  struct ConstructCandidates : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kEmbedHitsBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<xpu::no_smem, constants>;  // shared memory argument required
    XPU_D void operator()(context& ctx);
  };

  struct ExclusiveScan : xpu::kernel<GPUReco> {
    using block_size    = xpu::block_size<kScanBlockSize>;
    using scan_t        = xpu::block_scan<int, kScanBlockSize>;
    using constants     = xpu::cmem<strGnnGpuGraphConstructor>;
    using shared_memory = scan_t::storage_t;
    using context       = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct AddBlockSums : xpu::kernel<GPUReco> {
    using block_size    = xpu::block_size<kScanBlockSize>;
    using scan_t        = xpu::block_scan<int, kScanBlockSize>;
    using constants     = xpu::cmem<strGnnGpuGraphConstructor>;
    using shared_memory = scan_t::storage_t;
    using context       = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&, int);
  };

  struct AddOffsets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kScanBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct CompressAllTripletsOrdered : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kCompressionBlockSize>;
    using constants  = xpu::cmem<strGnnGpuGraphConstructor>;
    using context    = xpu::kernel_context<shared_memory, constants>;
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

    XPU_D void NearestNeighbours_FastPrim(NearestNeighbours_FastPrim::context&) const;

    XPU_D void NearestNeighbours_Other(NearestNeighbours_Other::context&, const int iteration) const;

    XPU_D void MakeTripletsOT_FastPrim(MakeTripletsOT_FastPrim::context&) const;

    XPU_D void MakeTripletsOT_Other(MakeTripletsOT_Other::context&, const int iteration) const;

    XPU_D void FitTripletsOT_FastPrim(FitTripletsOT_FastPrim::context&) const;

    XPU_D void FitTripletsOT_Other(FitTripletsOT_Other::context&) const;

    XPU_D void ConstructCandidates(ConstructCandidates::context&) const;

    XPU_D void ExclusiveScan(ExclusiveScan::context&) const;

    XPU_D void AddBlockSums(AddBlockSums::context&, int nblocks) const;

    XPU_D void AddOffsets(AddOffsets::context&) const;

    XPU_D void CompressAllTripletsOrdered(CompressAllTripletsOrdered::context&) const;


   private:
    XPU_D void EmbedSingleHit(std::array<float, 3>& input, std::array<float, 6>& result) const;

    // XPU_D void affine_3to16(std::array<std::array<float, 3>, 16>& weight, std::array<float, 3>& input,
    //                         std::array<float, 16>& bias, std::array<float, 16>& result) const;

    template<int Rows, int Cols>
    XPU_D void affine(const std::array<std::array<float, Cols>, Rows>& weight, const std::array<float, Cols>& input,
                      const std::array<float, Rows>& bias, std::array<float, Rows>& result) const;

    template<std::size_t N>
    XPU_D void applyTanH(std::array<float, N>& vec) const;

    XPU_D float hitDistanceSq(std::array<float, 6>& a, std::array<float, 6>& b) const;

    ///                          ------  DATA MEMBERS ------
   public:
    xpu::buffer<ca::GpuGrid> fvGpuGrid;                 ///< Grid for every station
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

    // xpu::buffer<ca::Triplet> fvTriplets;  ///< Triplets

    xpu::buffer<GpuParameters> fParams;

    int fIteration;

    GpuParameters fParams_const[4];

    ca::GpuStation fStations_const[constants::gpu::MaxNofStations];

    // General
    xpu::buffer<unsigned int> fIndexFirstHitStation;  // index (in fvHits) of first hit on station

    // Metric learning
    xpu::buffer<std::array<float, 6>> fEmbedCoord;
    xpu::buffer<GnnGpuEmbedNet> fEmbedParameters;

    // Doublets
    xpu::buffer<unsigned int> fNNeighbours;  // num doublets for each hit, max kNNOrder

    // FastPrim
    constexpr static const int kNN_FastPrim = 20;
    xpu::buffer<std::array<unsigned int, kNN_FastPrim>>
      fDoublets_FastPrim;  // neighbours of every hit from kNN. Hit index in fvHits

    // Other
    constexpr static const float margin_allPrim = 5.0; // def - 5
    constexpr static const float margin_allSec = 100.0; // def - 100
    constexpr static const int kNN_Other = 25 + 10;
    xpu::buffer<std::array<unsigned int, kNN_Other>> fDoublets_Other;

    // triplet construction
    xpu::buffer<unsigned int> fNTriplets;  // num triplets from hit

    // FastPrim
    xpu::buffer<std::array<std::array<unsigned int, 2>, kNN_FastPrim * kNN_FastPrim>>
      fTriplets_FastPrim;  // Triplets from hit. Can be max kNNOrder**2. Hit index in fvHits

    // AllPrim
    xpu::buffer<std::array<std::array<unsigned int, 2>, kNN_Other * kNN_Other>> fTriplets_Other;

    // triplet fitting
    // FastPrim
    xpu::buffer<std::array<bool, kNN_FastPrim * kNN_FastPrim>>
      fTripletsSelected_FastPrim;  // true where triplet passed KF fit check
    xpu::buffer<std::array<std::array<float, 7>, kNN_FastPrim * kNN_FastPrim>> fvTripletParams_FastPrim;  ///< Triplet parameters

    // AllPrim
    xpu::buffer<std::array<bool, kNN_Other * kNN_Other>> fTripletsSelected_Other;
    xpu::buffer<std::array<std::array<float, 7>, kNN_Other * kNN_Other>> fvTripletParams_Other;

    /// -- TEST --
    /// flatten triplets
    // Scan buffers
    xpu::buffer<unsigned int> fOffsets;           // size: fIterationData[0].fNHits
    xpu::buffer<unsigned int> fBlockOffsets;      // size: numBlocks used by scan
    xpu::buffer<unsigned int> fBlockOffsetsLast;  // size: ceil(numBlocks / kScanBlockSize)

    // Output
    xpu::buffer<std::array<unsigned int, 2>> fTripletsFlat;  // size: total (or maximal) capacity
    xpu::buffer<unsigned int> fTotalTriplets;                // size: 1 (optional)
  };

}  // namespace cbm::algo::ca
