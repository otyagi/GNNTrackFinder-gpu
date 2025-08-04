/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "Hitfind.h"

#include "AlgoFairloggerCompat.h"
#include "compat/OpenMP.h"
#include "util/TimingsFormat.h"

#include <chrono>

using namespace std;
using fles::Subsystem;

// By default all steps (clusterizing, hit building, hit merging) are parallelized
// by row index. Hit merging is thereby only done between pairs of neighboring rows,
// which can miss some large digi clusters. Enable flag below to instead parallelize
// the last step (hit merging) by module index.
//#define MERGE_BY_MODULE

// If hit merging by module index is enabled, enable the flag below to include an
// additional row-wise merge step before the final module-wise merge step is applied.
// This already catches most of the important merges and hence speeds up the process.
// In addition doing two sweeps through the buffers catches some multi-hit merges which are
// missed even in the module-wise method.
//#define PREPROCESS_BY_ROW

namespace cbm::algo::trd
{
  // -----   Constructor   ------------------------------------------------------
  Hitfind::Hitfind(trd::HitfindSetup setup, trd::Hitfind2DSetup setup2D)
  {

    // Create one algorithm per module for TRD and configure it with parameters
    for (size_t mod = 0; mod < setup.modules.size(); mod++) {

      cbm::algo::trd::HitfindSetup::Mod& module = setup.modules[mod];
      cbm::algo::trd::HitFinderModPar params;
      params.rowPar.resize(module.rowPar.size());

      const double* tra_ptr = module.translation.data();
      const double* rot_ptr = module.rotation.data();
      params.translation    = ROOT::Math::XYZVector(tra_ptr[0], tra_ptr[1], tra_ptr[2]);
      params.rotation       = ROOT::Math::Rotation3D(&rot_ptr[0], &rot_ptr[9]);
      params.address        = module.address;
      params.padSizeX       = module.padSizeX;
      params.padSizeY       = module.padSizeY;
      params.padSizeErrX    = module.padSizeErrX;
      params.padSizeErrY    = module.padSizeErrY;
      params.orientation    = module.orientation;

      for (size_t row = 0; row < module.rowPar.size(); row++) {
        cbm::algo::trd::HitfindSetup::Row& rowPar = module.rowPar[row];
        params.rowPar[row].padPar.resize(rowPar.padPar.size());

        for (size_t col = 0; col < rowPar.padPar.size(); col++) {
          cbm::algo::trd::HitfindSetup::Pad& pad  = rowPar.padPar[col];
          cbm::algo::trd::HitFinderPadPar& padPar = params.rowPar[row].padPar[col];

          const double* pos_ptr    = pad.position.data();
          const double* posErr_ptr = pad.positionError.data();
          padPar.pos               = ROOT::Math::XYZVector(pos_ptr[0], pos_ptr[1], pos_ptr[2]);
          padPar.posErr            = ROOT::Math::XYZVector(posErr_ptr[0], posErr_ptr[1], posErr_ptr[2]);
        }
        fRowList.emplace_back(module.address, false, row);
      }
      fHitFind[module.address]      = std::make_unique<cbm::algo::trd::HitFinder>(params);
      fHitMerge[module.address]     = std::make_unique<cbm::algo::trd::HitMerger>(params);
      fClusterBuild[module.address] = std::make_unique<cbm::algo::trd::Clusterizer>(params);
      fModId[module.address]        = fModList.size();
      fModList.emplace_back(module.address, false, module.rowPar.size(), module.rowPar[0].padPar.size());
    }

    // Create one algorithm per module for TRD2D and configure it with parameters
    for (size_t mod = 0; mod < setup2D.modules.size(); mod++) {

      cbm::algo::trd::Hitfind2DSetup::Mod& module = setup2D.modules[mod];
      cbm::algo::trd::HitFinder2DModPar params;
      params.rowPar.resize(module.rowPar.size());

      const double* tra_ptr = module.translation.data();
      const double* rot_ptr = module.rotation.data();
      params.translation    = ROOT::Math::XYZVector(tra_ptr[0], tra_ptr[1], tra_ptr[2]);
      params.rotation       = ROOT::Math::Rotation3D(&rot_ptr[0], &rot_ptr[9]);
      params.address        = module.address;
      params.padSizeX       = module.padSizeX;
      params.padSizeY       = module.padSizeY;

      for (size_t row = 0; row < module.rowPar.size(); row++) {
        cbm::algo::trd::Hitfind2DSetup::Row& rowPar = module.rowPar[row];
        params.rowPar[row].padPar.resize(rowPar.padPar.size());

        for (size_t col = 0; col < rowPar.padPar.size(); col++) {
          cbm::algo::trd::Hitfind2DSetup::Pad& pad  = rowPar.padPar[col];
          cbm::algo::trd::HitFinder2DPadPar& padPar = params.rowPar[row].padPar[col];

          const double* pos_ptr    = pad.position.data();
          const double* posErr_ptr = pad.positionError.data();
          padPar.pos               = ROOT::Math::XYZVector(pos_ptr[0], pos_ptr[1], pos_ptr[2]);
          padPar.posErr            = ROOT::Math::XYZVector(posErr_ptr[0], posErr_ptr[1], posErr_ptr[2]);
          padPar.chRMasked         = pad.chRMasked;
          padPar.chTMasked         = pad.chTMasked;
        }
        fRowList.emplace_back(module.address, true, row);
      }
      fHitFind2d[module.address]      = std::make_unique<cbm::algo::trd::HitFinder2D>(params);
      fHitMerge2d[module.address]     = std::make_unique<cbm::algo::trd::HitMerger2D>(params);
      fClusterBuild2d[module.address] = std::make_unique<cbm::algo::trd::Clusterizer2D>(params);
      fModId[module.address]          = fModList.size();
      fModList.emplace_back(module.address, true, module.rowPar.size(), module.rowPar[0].padPar.size());
    }

    L_(info) << "--- Configured hitfinder algorithms for TRD.";
  }
  // ----------------------------------------------------------------------------

  // -----   Execution   --------------------------------------------------------
  Hitfind::resultType Hitfind::operator()(gsl::span<CbmTrdDigi> digiIn)
  {
#ifdef MERGE_BY_MODULE
    return RunModuleParallelMerge(digiIn);
#else
    return RunRowParallel(digiIn);
#endif
  }
  // ----------------------------------------------------------------------------

  // -----   Execution fully row parallel  --------------------------------------
  Hitfind::resultType Hitfind::RunRowParallel(gsl::span<CbmTrdDigi> digiIn)
  {
    constexpr bool DebugCheckInput = true;

    // --- Output data
    resultType result = {};
    auto& hitsOut     = std::get<0>(result);
    auto& monitor     = std::get<1>(result);

    // Intermediate digi storage variables (digi, index) per module and row
    std::vector<std::vector<std::vector<std::pair<CbmTrdDigi, int32_t>>>> digiBuffer;  //[modAddress][row]
    digiBuffer.resize(fModList.size());

    // Intermediate hits per module and row
    std::vector<std::vector<hitDataType>> hitBuffer;  //[row]
    hitBuffer.resize(fRowList.size());

    // Initialize storage buffers
    for (size_t mod = 0; mod < fModList.size(); mod++) {
      const size_t numRows = std::get<2>(fModList[mod]);
      digiBuffer[mod].resize(numRows);
    }

    // Loop over the digis array and store the digis in separate vectors for
    // each module and row
    xpu::push_timer("DigiModuleSort");
    for (size_t idigi = 0; idigi < digiIn.size(); idigi++) {
      const CbmTrdDigi* digi = &digiIn[idigi];
      const int address      = digi->GetAddressModule();
      if constexpr (DebugCheckInput) {
        auto modInfo =
          std::find_if(fModList.begin(), fModList.end(), [&](auto m) { return std::get<0>(m) == address; });
        if (modInfo == fModList.end()) {
          L_(error) << "TRD: Unknown module ID";
          continue;
        }
        bool digiIs2D = digi->IsFASP();
        if (std::get<1>(*modInfo) != digiIs2D) {
          L_(error) << "TRD: Module + Digi type mismatch: " << std::get<0>(*modInfo) << ": " << std::get<1>(*modInfo)
                    << " " << digiIs2D;
          continue;
        }
      }
      const size_t modId   = fModId[address];
      const size_t numCols = std::get<3>(fModList[modId]);
      const int row        = digi->GetAddressChannel() / numCols;
      digiBuffer[modId][row].emplace_back(*digi, idigi);
    }
    monitor.sortTime = xpu::pop_timer();

    // Hit finding results
    PODVector<Hit> hitsFlat;       // hit storage
    PODVector<size_t> rowSizes;    // nHits per row
    PODVector<uint> rowAddresses;  // address of row

    // Prefix array for parallelization
    std::vector<size_t> hitsPrefix;
    std::vector<size_t> sizePrefix;
    std::vector<size_t> addrPrefix;

    xpu::push_timer("BuildHits");
    xpu::t_add_bytes(digiIn.size_bytes());

    CBM_PARALLEL()
    {
      const int ithread  = openmp::GetThreadNum();
      const int nthreads = openmp::GetNumThreads();

      CBM_OMP(single)
      {
        hitsPrefix.resize(nthreads + 1);
        sizePrefix.resize(nthreads + 1);
        addrPrefix.resize(nthreads + 1);
      }

      // Cluster and hit building
      CBM_OMP(for schedule(dynamic))
      for (size_t row = 0; row < fRowList.size(); row++) {
        const int address     = std::get<0>(fRowList[row]);
        const bool is2D       = std::get<1>(fRowList[row]);
        const size_t rowInMod = std::get<2>(fRowList[row]);
        const size_t modId    = fModId[address];
        const auto& digiInput = digiBuffer[modId][rowInMod];
        if (is2D) {
          auto clusters  = (*fClusterBuild2d[address])(digiInput, 0.);  // Number is TS start time (T0)
          hitBuffer[row] = (*fHitFind2d[address])(&clusters);
        }
        else {
          auto clusters  = (*fClusterBuild[address])(digiInput);
          hitBuffer[row] = (*fHitFind[address])(&clusters);
        }
      }

      // Row-merging for even rows
      CBM_OMP(for schedule(dynamic))
      for (size_t row = 0; row < fRowList.size() / 2; row++) {
        const size_t row1 = 2 * row;
        const size_t row2 = 2 * row + 1;
        const int address = std::get<0>(fRowList[row1]);
        const bool is2D   = std::get<1>(fRowList[row1]);
        if (row2 >= fRowList.size() || std::get<0>(fRowList[row2]) != address) {
          continue;
        }
        if (is2D) {
          std::tie(hitBuffer[row1], hitBuffer[row2]) = (*fHitMerge2d[address])(hitBuffer[row1], hitBuffer[row2]);
        }
        else {
          std::tie(hitBuffer[row1], hitBuffer[row2]) = (*fHitMerge[address])(hitBuffer[row1], hitBuffer[row2]);
        }
      }

      // Row-merging for odd rows
      CBM_OMP(for schedule(dynamic))
      for (size_t row = 0; row < fRowList.size() / 2; row++) {
        const size_t row1 = 2 * row + 1;
        const size_t row2 = 2 * row + 2;
        if (row2 >= fRowList.size()) {
          continue;
        }
        const int address = std::get<0>(fRowList[row1]);
        const bool is2D   = std::get<1>(fRowList[row1]);
        if (std::get<0>(fRowList[row2]) != address) {
          continue;
        }
        if (is2D) {
          std::tie(hitBuffer[row1], hitBuffer[row2]) = (*fHitMerge2d[address])(hitBuffer[row1], hitBuffer[row2]);
        }
        else {
          std::tie(hitBuffer[row1], hitBuffer[row2]) = (*fHitMerge[address])(hitBuffer[row1], hitBuffer[row2]);
        }
      }

      std::vector<Hit> local_hits;
      std::vector<size_t> local_sizes;
      std::vector<uint> local_addresses;

      // Buffer merging
      CBM_OMP(for schedule(dynamic) nowait)
      for (size_t row = 0; row < fRowList.size(); row++) {
        const int address = std::get<0>(fRowList[row]);
        auto& hits        = hitBuffer[row];
        std::vector<Hit> row_hits;
        std::transform(hits.begin(), hits.end(), std::back_inserter(row_hits), [](const auto& p) { return p.first; });

        // store partition size
        local_sizes.push_back(row_hits.size());

        // store hw address of partition
        local_addresses.push_back(address);

        // Append clusters to output
        local_hits.insert(local_hits.end(), std::make_move_iterator(row_hits.begin()),
                          std::make_move_iterator(row_hits.end()));
      }

      hitsPrefix[ithread + 1] = local_hits.size();
      sizePrefix[ithread + 1] = local_sizes.size();
      addrPrefix[ithread + 1] = local_addresses.size();
      CBM_OMP(barrier)
      CBM_OMP(single)
      {
        for (int i = 1; i < (nthreads + 1); i++) {
          hitsPrefix[i] += hitsPrefix[i - 1];
          sizePrefix[i] += sizePrefix[i - 1];
          addrPrefix[i] += addrPrefix[i - 1];
        }
        hitsFlat.resize(hitsPrefix[nthreads]);
        rowSizes.resize(sizePrefix[nthreads]);
        rowAddresses.resize(addrPrefix[nthreads]);
      }
      std::move(local_hits.begin(), local_hits.end(), hitsFlat.begin() + hitsPrefix[ithread]);
      std::move(local_sizes.begin(), local_sizes.end(), rowSizes.begin() + sizePrefix[ithread]);
      std::move(local_addresses.begin(), local_addresses.end(), rowAddresses.begin() + addrPrefix[ithread]);
    }

    // Monitoring
    monitor.timeHitfind = xpu::pop_timer();
    monitor.numDigis    = digiIn.size();
    monitor.numHits     = hitsFlat.size();

    // Create ouput vector
    hitsOut = PartitionedVector(std::move(hitsFlat), rowSizes, rowAddresses);

    // Ensure hits are time sorted
    CBM_PARALLEL_FOR(schedule(dynamic))
    for (size_t i = 0; i < hitsOut.NPartitions(); i++) {
      auto part = hitsOut[i];
      std::sort(part.begin(), part.end(), [](const auto& h0, const auto& h1) { return h0.Time() < h1.Time(); });
    }

    return result;
  }
  // ----------------------------------------------------------------------------


  // -----   Execution merging module parallel  ---------------------------------
  Hitfind::resultType Hitfind::RunModuleParallelMerge(gsl::span<CbmTrdDigi> digiIn)
  {
    constexpr bool DebugCheckInput = true;

    // --- Output data
    resultType result = {};
    auto& hitsOut     = std::get<0>(result);
    auto& monitor     = std::get<1>(result);

    // Intermediate digi storage variables (digi, index) per module and row
    std::unordered_map<int, std::vector<std::vector<std::pair<CbmTrdDigi, int32_t>>>> digiBuffer;  //[modAddress][row]

    // Intermediate hits per module and row
    std::unordered_map<int, std::vector<std::vector<hitDataType>>> hitBuffer;  //[modAddress][row]

    // Initialize storage buffers
    for (size_t mod = 0; mod < fModList.size(); mod++) {
      const int address    = std::get<0>(fModList[mod]);
      const size_t numRows = std::get<2>(fModList[mod]);
      digiBuffer[address].resize(numRows);
      hitBuffer[address].resize(numRows);
    }

    // Loop over the digis array and store the digis in separate vectors for
    // each module and row
    xpu::push_timer("DigiModuleSort");
    for (size_t idigi = 0; idigi < digiIn.size(); idigi++) {
      const CbmTrdDigi* digi = &digiIn[idigi];
      const int address      = digi->GetAddressModule();
      if constexpr (DebugCheckInput) {
        auto modInfo =
          std::find_if(fModList.begin(), fModList.end(), [&](auto m) { return std::get<0>(m) == address; });
        if (modInfo == fModList.end()) {
          L_(error) << "TRD: Unknown module ID";
          continue;
        }
        bool digiIs2D = digi->IsFASP();
        if (std::get<1>(*modInfo) != digiIs2D) {
          L_(error) << "TRD: Module + Digi type mismatch: " << std::get<0>(*modInfo) << ": " << std::get<1>(*modInfo)
                    << " " << digiIs2D;
          continue;
        }
      }
      const size_t modId   = fModId[address];
      const size_t numCols = std::get<3>(fModList[modId]);
      const int row        = digi->GetAddressChannel() / numCols;
      digiBuffer[address][row].emplace_back(*digi, idigi);
    }
    monitor.sortTime = xpu::pop_timer();

    xpu::push_timer("BuildClusters");
    xpu::t_add_bytes(digiIn.size_bytes());

    // Cluster building and hit finding
    CBM_PARALLEL_FOR(schedule(dynamic))
    for (size_t row = 0; row < fRowList.size(); row++) {
      const int address     = std::get<0>(fRowList[row]);
      const bool is2D       = std::get<1>(fRowList[row]);
      const size_t rowInMod = std::get<2>(fRowList[row]);
      const auto& digiInput = digiBuffer[address][rowInMod];
      if (is2D) {
        auto clusters                = (*fClusterBuild2d[address])(digiInput, 0.);  // Number is TS start time (T0)
        hitBuffer[address][rowInMod] = (*fHitFind2d[address])(&clusters);
      }
      else {
        auto clusters                = (*fClusterBuild[address])(digiInput);
        hitBuffer[address][rowInMod] = (*fHitFind[address])(&clusters);
      }
    }

#ifdef PREPROCESS_BY_ROW
    // Row-merging for even rows
    CBM_PARALLEL_FOR(schedule(dynamic))
    for (size_t row = 0; row < fRowList.size() / 2; row++) {
      const size_t row1      = 2 * row;
      const size_t row2      = 2 * row + 1;
      const int address      = std::get<0>(fRowList[row1]);
      const bool is2D        = std::get<1>(fRowList[row1]);
      const size_t rowInMod1 = std::get<2>(fRowList[row1]);
      const size_t rowInMod2 = std::get<2>(fRowList[row2]);
      auto& buffer           = hitBuffer[address];

      if (row2 >= fRowList.size() || std::get<0>(fRowList[row2]) != address) {
        continue;
      }
      if (is2D) {
        std::tie(buffer[rowInMod1], buffer[rowInMod2]) = (*fHitMerge2d[address])(buffer[rowInMod1], buffer[rowInMod2]);
      }
      else {
        std::tie(buffer[rowInMod1], buffer[rowInMod2]) = (*fHitMerge[address])(buffer[rowInMod1], buffer[rowInMod2]);
      }
    }

    // Row-merging for odd rows
    CBM_PARALLEL_FOR(schedule(dynamic))
    for (size_t row = 0; row < fRowList.size() / 2; row++) {
      const size_t row1 = 2 * row + 1;
      const size_t row2 = 2 * row + 2;
      if (row2 >= fRowList.size()) {
        continue;
      }
      const int address = std::get<0>(fRowList[row1]);
      const bool is2D   = std::get<1>(fRowList[row1]);
      if (std::get<0>(fRowList[row2]) != address) {
        continue;
      }
      const size_t rowInMod1 = std::get<2>(fRowList[row1]);
      const size_t rowInMod2 = std::get<2>(fRowList[row2]);
      auto& buffer           = hitBuffer[address];
      if (is2D) {
        std::tie(buffer[rowInMod1], buffer[rowInMod2]) = (*fHitMerge2d[address])(buffer[rowInMod1], buffer[rowInMod2]);
      }
      else {
        std::tie(buffer[rowInMod1], buffer[rowInMod2]) = (*fHitMerge[address])(buffer[rowInMod1], buffer[rowInMod2]);
      }
    }
#endif

    monitor.timeClusterize = xpu::pop_timer();

    // Result storage
    PODVector<Hit> hitsFlat;       // hit storage
    PODVector<size_t> modSizes;    // nHits per modules
    PODVector<uint> modAddresses;  // address of modules

    // Prefix array for parallelization
    std::vector<size_t> hitsPrefix;
    std::vector<size_t> sizePrefix;
    std::vector<size_t> addrPrefix;

    xpu::push_timer("FindHits");

    // Combine row buffers into module buffers.
    // Then run a final module-wise row-merging iteration and arrange results.
    CBM_PARALLEL()
    {
      const int ithread  = openmp::GetThreadNum();
      const int nthreads = openmp::GetNumThreads();

      CBM_OMP(single)
      {
        hitsPrefix.resize(nthreads + 1);
        sizePrefix.resize(nthreads + 1);
        addrPrefix.resize(nthreads + 1);
      }

      std::vector<Hit> local_hits;
      std::vector<size_t> local_sizes;
      std::vector<uint> local_addresses;

      CBM_OMP(for schedule(dynamic) nowait)
      for (size_t mod = 0; mod < fModList.size(); mod++) {
        const int address = std::get<0>(fModList[mod]);
        const bool is2D   = std::get<1>(fModList[mod]);

        // Lambda expression for vector concatenation
        auto concatVec = [](auto& acc, const auto& innerVec) {
          acc.insert(acc.end(), innerVec.begin(), innerVec.end());
          return std::move(acc);
        };


        // Flatten the input vector of vectors and merge hits
        auto& hitbuffer = hitBuffer[address];
        auto hitData    = std::accumulate(hitbuffer.begin(), hitbuffer.end(), std::vector<hitDataType>(), concatVec);

        std::vector<hitDataType> mod_hitdata;
        std::vector<hitDataType> dummy;
        if (is2D) {
          mod_hitdata = (*fHitMerge2d[address])(hitData, dummy).first;
        }
        else {
          mod_hitdata = (*fHitMerge[address])(hitData, dummy).first;
        }

        // Remove digi data from hits
        std::vector<Hit> mod_hits;
        std::transform(mod_hitdata.begin(), mod_hitdata.end(), std::back_inserter(mod_hits),
                       [](const auto& p) { return p.first; });

        // store partition size
        local_sizes.push_back(mod_hits.size());

        // store hw address of partition
        local_addresses.push_back(address);

        // Append clusters to output
        local_hits.insert(local_hits.end(), std::make_move_iterator(mod_hits.begin()),
                          std::make_move_iterator(mod_hits.end()));
      }

      hitsPrefix[ithread + 1] = local_hits.size();
      sizePrefix[ithread + 1] = local_sizes.size();
      addrPrefix[ithread + 1] = local_addresses.size();
      CBM_OMP(barrier)
      CBM_OMP(single)
      {
        for (int i = 1; i < (nthreads + 1); i++) {
          hitsPrefix[i] += hitsPrefix[i - 1];
          sizePrefix[i] += sizePrefix[i - 1];
          addrPrefix[i] += addrPrefix[i - 1];
        }
        hitsFlat.resize(hitsPrefix[nthreads]);
        modSizes.resize(sizePrefix[nthreads]);
        modAddresses.resize(addrPrefix[nthreads]);
      }
      std::move(local_hits.begin(), local_hits.end(), hitsFlat.begin() + hitsPrefix[ithread]);
      std::move(local_sizes.begin(), local_sizes.end(), modSizes.begin() + sizePrefix[ithread]);
      std::move(local_addresses.begin(), local_addresses.end(), modAddresses.begin() + addrPrefix[ithread]);
    }
    // Monitoring
    monitor.timeHitfind = xpu::pop_timer();
    monitor.numDigis    = digiIn.size();
    monitor.numHits     = hitsFlat.size();

    // Create ouput vector
    hitsOut = PartitionedVector(std::move(hitsFlat), modSizes, modAddresses);

    // Ensure hits are time sorted
    CBM_PARALLEL_FOR(schedule(dynamic))
    for (size_t i = 0; i < hitsOut.NPartitions(); i++) {
      auto part = hitsOut[i];
      std::sort(part.begin(), part.end(), [](const auto& h0, const auto& h1) { return h0.Time() < h1.Time(); });
    }

    return result;
  }
  // ----------------------------------------------------------------------------

}  // namespace cbm::algo::trd
