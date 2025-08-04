/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#pragma once

#include "CbmTrdDigi.h"
#include "DigiRec.h"
#include "PODVector.h"
#include "PartitionedVector.h"
#include "trd/Clusterizer.h"
#include "trd/Clusterizer2D.h"
#include "trd/HitFinder.h"
#include "trd/HitFinder2D.h"
#include "trd/HitMerger.h"
#include "trd/HitMerger2D.h"
#include "trd/Hitfind2DSetup.h"
#include "trd/HitfindSetup.h"

#include <gsl/span>
#include <optional>
#include <sstream>
#include <vector>

#include <xpu/host.h>


namespace cbm::algo::trd
{

  /** @struct HitfindMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 17 Apr 2024
   ** @brief Monitoring data for hitfinding
   **/
  struct HitfindMonitorData {
    xpu::timings timeHitfind;
    xpu::timings timeClusterize;
    xpu::timings sortTime;
    size_t numDigis = 0;
    size_t numHits  = 0;

    std::string print() const
    {
      std::stringstream ss;
      ss << "Hitfind stats: num digis " << numDigis << ", num hits " << numHits << std::endl;
      return ss.str();
    }
  };

  /** @class Hitfind
 ** @brief Algo class for hitfinding
 ** @author Dominik Smith <d.smith@gsi.de>
 ** @since 17.04.2024
 **
 **/
  class Hitfind {

   public:
    typedef std::tuple<PartitionedVector<Hit>, HitfindMonitorData> resultType;
    typedef std::pair<Hit, std::vector<DigiRec>> hitDataType;

    /** @brief Algorithm execution
     ** @param fles timeslice to hitfind
     ** @return pair: digi timeslice, monitoring data
     **
     ** @note Modifies input digis for time calibration
     **/
    resultType operator()(gsl::span<CbmTrdDigi> digiIn);

    /** @brief Run all steps row-parallel **/
    resultType RunRowParallel(gsl::span<CbmTrdDigi> digiIn);

    /** @brief Run merge step module-parallel all others row-parallel **/
    resultType RunModuleParallelMerge(gsl::span<CbmTrdDigi> digiIn);

    /** @brief Constructor **/
    explicit Hitfind(trd::HitfindSetup, trd::Hitfind2DSetup);

   private:  // members
    /** @brief Cluster building algorithms per module */
    std::unordered_map<int, std::unique_ptr<cbm::algo::trd::Clusterizer2D>> fClusterBuild2d;
    std::unordered_map<int, std::unique_ptr<cbm::algo::trd::Clusterizer>> fClusterBuild;

    /** @brief Hit finding algorithms per module */
    std::unordered_map<int, std::unique_ptr<cbm::algo::trd::HitFinder2D>> fHitFind2d;
    std::unordered_map<int, std::unique_ptr<cbm::algo::trd::HitFinder>> fHitFind;

    /** @brief Hit merging algorithms per module */
    std::unordered_map<int, std::unique_ptr<cbm::algo::trd::HitMerger2D>> fHitMerge2d;
    std::unordered_map<int, std::unique_ptr<cbm::algo::trd::HitMerger>> fHitMerge;

    /** @brief List of modules (address, type flag (true = 2D), numRows, numCols) */
    std::vector<std::tuple<int, bool, size_t, size_t>> fModList;

    /** @brief List of rows (module address, type flag (true = 2D), row in module) */
    std::vector<std::tuple<int, bool, size_t>> fRowList;

    /** @brief Map from module address to module Id (sequential number) */
    std::unordered_map<int, size_t> fModId;
  };
}  // namespace cbm::algo::trd
