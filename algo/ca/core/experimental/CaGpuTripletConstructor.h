/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file CaGpuTripletConstructor.h
/// \brief The class contains data and kernels for running CA tracking on CPU and GPU using XPU libraries


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
  enum CaGpuConstants
  {
#if XPU_IS_CUDA
    kSingletConstructorBlockSize      = 512,
    kSingletConstructorItemsPerThread = 8,
#else  // HIP, values ignored on CPU
    kSingletConstructorBlockSize      = 64,  //1024,	//128 is optimal (preliminary tests)
    kSingletConstructorItemsPerThread = 6,
    kSortTripletsBlockSize            = 64,
    kSortTripletsItemsPerThread       = 6,
#endif
  };
}  // namespace cbm::algo

namespace cbm::algo::ca
{
  //  class Framework;
  class GpuTripletConstructor;

  struct TrackingParams {
  };

  struct Params : xpu::constant<ca::GPUReco, TrackingParams> {
  };

  // Declare Constant Memory
  struct strGpuTripletConstructor : xpu::constant<GPUReco, GpuTripletConstructor> {
  };

  struct hit_doublets {
    ca::HitIndex_t hit1;
    ca::HitIndex_t hit2;
  };

  struct hit_triplets {
    ca::HitIndex_t hit1;
    ca::HitIndex_t hit2;
    ca::HitIndex_t hit3;
  };

  struct sort_triplets {
    unsigned int tr_id;
    //    unsigned int lsta;
    unsigned int lhit;
  };

  struct shared_counters {
    int counter;
  };

  struct shared_data {
    //    ca::GpuStation fStations_shared[kMaxNofStations];
    ca::GpuFieldRegion fld0_shared[kSingletConstructorBlockSize];
    ca::GpuFieldRegion fld1_shared[kSingletConstructorBlockSize];
  };

  struct shared_data_fit_triplets {
    //    ca::GpuStation fStations_shared[kMaxNofStations];
    kf::TrackParamBase<float> fit_params_shared[kSingletConstructorBlockSize];
  };

  //  struct TestFunc : xpu::kernel<GPUReco> {
  //    using block_size    = xpu::block_size<kSingletConstructorBlockSize>;
  //    using constants     = xpu::cmem<strGpuTripletConstructor, Params>;
  //    using context       = xpu::kernel_context<shared_memory, constants>;
  //    XPU_D void operator()(context&);
  //  };

  // Declare Kernels
  struct MakeSinglets : xpu::kernel<GPUReco> {
    using block_size    = xpu::block_size<kSingletConstructorBlockSize>;
    using shared_memory = shared_data;
    using constants     = xpu::cmem<strGpuTripletConstructor, Params>;
    using context       = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct MakeDoublets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSingletConstructorBlockSize>;
    //    using shared_memory = shared_counters;
    using constants = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context   = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct CompressDoublets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSingletConstructorBlockSize>;
    using constants  = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context    = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct FitDoublets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSingletConstructorBlockSize>;
    using constants  = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context    = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct MakeTriplets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSingletConstructorBlockSize>;
    //    using shared_memory = shared_counters;
    using constants = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context   = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct CompressTriplets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSingletConstructorBlockSize>;
    using constants  = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context    = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct FitTriplets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSingletConstructorBlockSize>;
    //    using shared_memory = shared_data_fit_triplets;
    using constants = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context   = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct SortTriplets : xpu::kernel<GPUReco> {
    using block_size = xpu::block_size<kSortTripletsBlockSize>;
    using sort_t = xpu::block_sort<unsigned int, sort_triplets, kSortTripletsBlockSize, kSortTripletsItemsPerThread>;
    using shared_memory = sort_t::storage_t;
    using constants     = xpu::cmem<strGpuTripletConstructor>;  //, Params>;
    using context       = xpu::kernel_context<shared_memory, constants>;
    XPU_D void operator()(context&);
  };

  struct IterationData {
    int fNHits;              ///< Number of hits in grid
    int fIteration;          ///< Iteration number
    int fNDoublets;          ///< Number of doublets
    int fNDoublets_counter;  ///< Number of doublets counter
    int fNTriplets;          ///< Number of triplets
    int fNTriplets_counter;  ///< Number of triplets counter
  };

  class GpuTripletConstructor {
   public:
    ///                             ------  FUNCTIONAL PART ------

    //    XPU_D void TestFunc(TestFunc::context&) const;

    XPU_D void MakeSinglets(MakeSinglets::context&) const;

    XPU_D void MakeDoublets(MakeDoublets::context&) const;

    XPU_D void CompressDoublets(CompressDoublets::context&) const;

    XPU_D void FitDoublets(FitDoublets::context&) const;

    XPU_D void MakeTriplets(MakeTriplets::context&) const;

    XPU_D void CompressTriplets(CompressTriplets::context&) const;

    XPU_D void FitTriplets(FitTriplets::context&) const;

    XPU_D void SortTripletsFunc(SortTriplets::context&) const;

   private:
    XPU_D void CollectHits(int iThread, int iSta, int iHit, int maxNHits, int* collectedHits) const;

    XPU_D void CollectHitsTriplets(int iThread, int iSta, int maxNHits,
                                   int* collectedHits) const;  //TODO: merge with CollectHits


    ///                             ------  FITTING FINCTIONALITY ------ //TODO: check and switch to the standard CaTrackFit if possible
    XPU_D void FilterWithTargetAtLine(kf::TrackParamBase<float>* tr_par, const ca::GpuFieldRegion* F0) const;

    /// extrapolate track as a line, return the extrapolated X, Y and the Jacobians
    XPU_D void GetExtrapolatedXYline(kf::TrackParamBase<float>* tr_par, float z, const ca::GpuFieldRegion* F,
                                     float* extrXY, float* Jx, float* Jy) const;

    XPU_D void MultipleScattering(kf::TrackParamBase<float>* tr_par, float radThick, float qp0) const;

    /// extrapolate the track to the given Z using the field F
    /// it does extrapolation in one step
    XPU_D void ExtrapolateStep(kf::TrackParamBase<float>* tr_par, float z_out, float qp0,
                               const ca::GpuFieldRegion* Field) const;

    /// filter the track with the XY measurement placed at different Z
    /// \param m - measurement
    /// \param extrX - extrapolated X of the track
    /// \param extrY - extrapolated Y of the track
    /// \param Jx - Jacobian of the extrapolated X
    /// \param Jy - Jacobian of the extrapolated Y
    XPU_D void FilterExtrapolatedXY(
      kf::TrackParamBase<float>* tr_par, const ca::MeasurementXy<float>* m, float* extrXY,  // fvec extrY,
      //			      std::array<float, ca::TrackParamBase<float>::kNtrackParam>* Jx,
      //			      std::array<float, ca::TrackParamBase<float>::kNtrackParam>* Jy) const;
      float* Jx, float* Jy) const;

    XPU_D void ExtrapolateLineNoField(kf::TrackParamBase<float>* tr_par, float zOut) const;

    XPU_D void FilterXY(kf::TrackParamBase<float>* tr_par, const ca::MeasurementXy<float>& mxy) const;

    XPU_D void Filter1d(kf::TrackParamBase<float>* tr_par, const kf::MeasurementU<float>& m) const;

    XPU_D void FilterTime(kf::TrackParamBase<float>* tr_par, float t, float dt2, float timeInfo) const;

    XPU_D void EnergyLossCorrection(kf::TrackParamBase<float>* tr_par, float radThick, float upstreamDirection) const;

    ///                          ------  DATA MEMBERS ------
   public:
    xpu::buffer<ca::GpuGrid> fvGpuGrid;  ///< Grid

    xpu::buffer<unsigned int> fgridFirstBinEntryIndex;  ///< Index of the first entry in the grid

    xpu::buffer<unsigned int> fgridEntries;  ///< Entries in the grid	//TODO: check if it should be ca::HitIndex_t

    ///Material map
    xpu::buffer<ca::GpuMaterialMap> fMaterialMap;  ///< Material map base objects

    xpu::buffer<float> fMaterialMapTables;  ///< Material map tables

    xpu::buffer<IterationData> fIterationData;  ///< Temporary data

    //    xpu::buffer<unsigned char> fvbHitSuppressed;  ///< Hit suppression flags

    xpu::buffer<hit_doublets> fHitDoublets;  ///< Hit doublets

    xpu::buffer<hit_doublets> fHitDoubletsCompressed;  ///< Hit doublets compressed

    xpu::buffer<hit_triplets> fHitTriplets;  ///< Hit triplets

    xpu::buffer<hit_triplets> fHitTripletsCompressed;  ///< Hit triplets compressed

    /// \brief Hits of the current time window
    ///
    /// It is a portion of fInputData to process in the current time window
    /// hit.Id is replaced by the hit index in fInputData
    xpu::buffer<ca::Hit> fvHits;

    xpu::buffer<kf::TrackParamBase<float>> fvTrackParams;          ///< Track parameters
    xpu::buffer<kf::TrackParamBase<float>> fvTrackParamsDoublets;  ///< Track parameters for doublets
    //    xpu::buffer<ca::TrackParamBase<float>> fvTrackParamsTriplets;  ///< Track parameters for triplets

    xpu::buffer<ca::Triplet> fvTriplets;  ///< Triplets

    xpu::buffer<sort_triplets> fTripletSortHelper;  ///< Helper for sorting triplets
                                                    //    xpu::buffer<sort_triplets> fTripletSortHelperTmp;
                                                    //    xpu::buffer<sort_triplets*> dst;

    //    GpuParameters fParams;
    xpu::buffer<GpuParameters> fParams;

    int fIteration;

    GpuParameters fParams_const[4];

    ca::GpuStation fStations_const[constants::gpu::MaxNofStations];
  };

}  // namespace cbm::algo::ca
