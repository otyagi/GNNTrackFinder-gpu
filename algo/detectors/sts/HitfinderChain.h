/* Copyright (C) 2022 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Kilian Hunold */

#ifndef CBM_ALGO_STS_HITFINDER_CHAIN_H
#define CBM_ALGO_STS_HITFINDER_CHAIN_H

#include "CbmStsDigi.h"
#include "PartitionedSpan.h"
#include "PartitionedVector.h"
#include "SubChain.h"
#include "sts/Hitfinder.h"
#include "sts/HitfinderPars.h"
#include "sts/LandauTable.h"

#include <array>
#include <cstdint>
#include <gsl/span>
#include <map>
#include <optional>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::sts
{

  struct HitfinderMon : HitfinderMonDevice {
    u64 nClusterTotal;
    u64 nHitsTotal;

    void SetDeviceMon(const HitfinderMonDevice& devMon) { HitfinderMonDevice::operator=(devMon); }
  };

  struct HitfinderChainPars {
    HitfinderPars setup;  // TODO: HitfinderPars should renamed to HitfinderSetup
    RecoParams::STS::Memory memory;
  };

  /**
   * Sts Hitfinder Chain. This class is responsible for all memory allocations
   * on the GPU required by the hitfinder. It executes the hitfinder kernels and
   * handles memory transfer for input / output data and conversion to the
   * regular CBM STS types.
   */
  class HitfinderChain : public SubChain {

   public:
    struct Result {
      PartitionedSpan<sts::Hit> hits;
      PartitionedVector<sts::Cluster> clusters;
      HitfinderMon monitor;
    };

    void SetParameters(const HitfinderChainPars& parameters);
    const HitfinderChainPars& GetParameters() const { return *fPars; }

    /**
     * Teardown chain.
     */
    void Finalize();

    Result operator()(gsl::span<const CbmStsDigi>, bool storeClusters = false);

   private:
    static constexpr u16 InvalidModule = 0xFFFF;

    struct DigiMap {
      //< Map module address to index in digis array. Only works for 17 bit packed addresses.
      std::vector<u16> addrToIndex;
      // Map modules to number of Digis, 2 * NModules entries, first half is front, second half is back
      std::vector<size_t> nDigisPerModule;
      std::vector<std::vector<size_t>> nDigisPerThread;  //< Number of digis per module per thread [thread][module]
      size_t maxNDigisPerModule = 0;                     //< Upper bound on number of digis per module

      u16 ModuleIndex(const CbmStsDigi& digi) const
      {
        i32 moduleAddr  = digi.GetAddressPacked();
        u16 moduleIndex = addrToIndex[moduleAddr];
        return moduleIndex;
      }
    };

    /**
       * Ensure parameters were set. Raises log(fatal) otherwise.
       */
    void EnsureParameters();

    /**
       * Allocate memory that doesn't depend on input
       * Data stays on GPU for the entire duration the hitfinder is alive
       */
    void AllocateStatic();

    /**
       * Allocate memory that depends on input
       * Data stays on GPU until next timeslice is processed
       */
    void AllocateDynamic(size_t, size_t);

    /**
      * Count Digis per module.
      */
    DigiMap CountDigisPerModules(gsl::span<const CbmStsDigi> digis);

    /**
      * Copy Digis into flat array that can be copied to the GPU.
      */
    void FlattenDigis(gsl::span<const CbmStsDigi> digis, DigiMap& digiMap);

    /**
     * Copy Hits back to host into a flat array.
     */
    PartitionedSpan<sts::Hit> FlattenHits(xpu::queue);

    /**
     * Copy Clusters back to host into a flat array.
     */
    PartitionedVector<sts::Cluster> FlattenClusters(xpu::queue);

    /**
     * @brief Returns the number of hits of a module.
     *
     * @note: Wrapper method required as buckets might overflow. This corrects for that.
     **/
    size_t GetNHits(xpu::h_view<PaddedToCacheLine<int>> nHitsPerModule, int module);

    /**
     * Divide Hits into streams.
     *
     * @param hits: Hits to sort. Expects one partition per sensor.
     * @param nstreamsMax: max number of streams to create.
     *
     * @note: Creates 'nstreamsMax / nSensors' streams per sensor. Rounds down if nstreamsMax isn't divisble by nSensors.
     * So fewer streams in total could be created.
     **/
    PartitionedSpan<sts::Hit> SplitHitsIntoStreams(PartitionedSpan<sts::Hit> hits, int nstreamsMax);

    /**
     * Sort Hits by time in each partition.
     */
    void SortHitsWithinPartition(PartitionedSpan<sts::Hit> hits);

    // Debug functions, ensure reco produces sane results
    void EnsureDigiOffsets(DigiMap&);
    void EnsureDigisSorted();
    void EnsureChannelOffsets(gsl::span<u32>);
    void EnsureClustersSane(gsl::span<ClusterIdx>, gsl::span<PaddedToCacheLine<int>>);
    void EnsureClustersSorted();
    void EnsureHitsSorted(PartitionedSpan<sts::Hit>);

    std::optional<sts::HitfinderChainPars> fPars;

    Hitfinder fHitfinder;

    // Output buffer, used by the returned PartitionedSpan
    std::vector<u32> fAddresses;
    std::vector<size_t> fHitOffsets;
    std::vector<u32> fStreamAddresses;
    std::vector<size_t> fHitStreamOffsets;
  };

}  // namespace cbm::algo::sts

#endif  // #ifndef CBM_ALGO_STS_HITFINDER_CHAIN_H
