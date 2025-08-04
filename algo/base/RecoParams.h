/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_RECOPARAMS_H
#define CBM_ALGO_BASE_RECOPARAMS_H

#include "Definitions.h"
#include "util/EnumDict.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <xpu/defines.h>

namespace cbm::algo
{

  /**
   * @brief RecoParams contains all parameters to configure reconstruction
   */
  struct RecoParams {
    enum class SortMode : u8
    {
      BlockSort        = 0,
      CUBSegmentedSort = 1,
    };
    enum class AllocationMode : u8
    {
      Auto,     //< Static on GPU, dynamic on CPU
      Static,   //< Allocate all buffers beforehand
      Dynamic,  //< Allocate buffers per timeslice
    };

    struct STS {
      SortMode digiSortMode;
      SortMode clusterSortMode;

      u8 findClustersMultiKernels;

      f32 timeCutDigiAbs;
      f32 timeCutDigiSig;
      f32 timeCutClusterAbs;
      f32 timeCutClusterSig;

      bool doChargeCorrelation;
      f32 chargeCorrelationDelta;

      struct Memory {
        AllocationMode allocationMode;
        u64 maxNDigisPerTS;
        u64 maxNDigisPerModule;
        f64 clustersPerDigi;
        f64 hitsPerCluster;

        u64 NClustersUpperBound(u64 nDigis) const { return nDigis * clustersPerDigi; }
        u64 NHitsUpperBound(u64 nDigis) const { return NClustersUpperBound(nDigis) * hitsPerCluster; }

        u64 MaxNClustersPerModule() const { return NClustersUpperBound(maxNDigisPerModule); }
        u64 MaxNHitsPerModule() const { return MaxNClustersPerModule() * hitsPerCluster; }

        bool IsDynamic() const { return allocationMode == RecoParams::AllocationMode::Dynamic; }
        bool IsStatic() const { return allocationMode == RecoParams::AllocationMode::Static; }
        bool IsAuto() const { return allocationMode == RecoParams::AllocationMode::Auto; }

        CBM_YAML_PROPERTIES(
          yaml::Property(&Memory::allocationMode, "allocationMode", "Allocation mode (Auto, Static, Dynamic)"),
          yaml::Property(&Memory::maxNDigisPerTS, "maxNDigisPerTS", "Maximal number of digis per time slice"),
          yaml::Property(&Memory::maxNDigisPerModule, "maxNDigisPerModule", "Maximal number of digis per module"),
          yaml::Property(&Memory::clustersPerDigi, "clustersPerDigi", "Number of clusters per digi in a time slice"),
          yaml::Property(&Memory::hitsPerCluster, "hitsPerCluster", "Number of hits per cluster in a time slice"));
      } memory;

      CBM_YAML_PROPERTIES(
        yaml::Property(&STS::digiSortMode, "digiSortMode",
                         "Digi sort mode (0 = block sort, 1 = cub segmented sort))"),
        yaml::Property(&STS::clusterSortMode, "clusterSortMode", "Cluster sort mode"),

        yaml::Property(&STS::findClustersMultiKernels, "findClustersMultiKernels",
                         "Split cluster finding into multiple kernels"),

        yaml::Property(&STS::timeCutDigiAbs, "timeCutDigiAbs",
                         "Time delta for neighboring digis to be considered for the same cluster. [ns]"),
        yaml::Property(
          &STS::timeCutDigiSig, "timeCutDigiSig",
          "Used if timeCutDigiAbs is negative. Time delta must be < 'value * sqrt2 * timeResolution'. [ns]"),
        yaml::Property(&STS::timeCutClusterAbs, "timeCutClusterAbs",
                         "Maximal time difference between two clusters in a hit [ns]."
                         " Setting to a positive value will override timeCutClustersSig."),
        yaml::Property(
          &STS::timeCutClusterSig, "timeCutClusterSig",
          "Time cut for clusters."
          " Two clusters are considered it their time difference is below 'value * sqrt(terr1**2 + terr2*+2)'"),

        yaml::Property(&STS::doChargeCorrelation, "doChargeCorrelation",
                         "Enable charge correlation between front+back clusters during hit finding"),
        yaml::Property(&STS::chargeCorrelationDelta, "chargeCorrelationDelta", "Delta in total charge between front and back clusters to be considered for hit finding"),
        yaml::Property(&STS::memory, "memory", "Memory limits for STS reco"));
    };

    STS sts;

    CBM_YAML_PROPERTIES(yaml::Property(&RecoParams::sts, "sts", "STS reco settings"));
  };

}  // namespace cbm::algo

CBM_YAML_EXTERN_DECL(cbm::algo::RecoParams);

CBM_ENUM_DICT(cbm::algo::RecoParams::SortMode,
  {"BlockSort", RecoParams::SortMode::BlockSort},
  {"CUBSegmentedSort", RecoParams::SortMode::CUBSegmentedSort}
);

CBM_ENUM_DICT(cbm::algo::RecoParams::AllocationMode,
  {"Auto", RecoParams::AllocationMode::Auto},
  {"Static", RecoParams::AllocationMode::Static},
  {"Dynamic", RecoParams::AllocationMode::Dynamic}
);

#endif  // CBM_ALGO_BASE_RECOPARAMS_H
