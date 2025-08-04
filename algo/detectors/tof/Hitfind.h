/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#ifndef TOFHITFIND_H
#define TOFHITFIND_H 1

#include "CbmTofDigi.h"
#include "PODVector.h"
#include "PartitionedVector.h"
#include "tof/Clusterizer.h"
#include "tof/HitfindSetup.h"

#include <gsl/span>
#include <optional>
#include <sstream>
#include <vector>

#include <xpu/host.h>


namespace cbm::algo::tof
{

  /** @struct HitfindMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 16 Oct 2023
   ** @brief Monitoring data for hitfinding
   **/
  struct HitfindMonitorData {
    //std::vector<tof::ClusterizerMonitorData> fMonitor;   //Per RPC monitoring data, to be implemented
    xpu::timings fTime;
    xpu::timings fSortTime;
    size_t fNumDigis = 0;
    size_t fNumHits  = 0;

    std::string print() const
    {
      std::stringstream ss;
      ss << "Hitfind stats: num digis " << fNumDigis << ", time " << fTime.wall() << " ms ( " << fTime.throughput()
         << " GB/s ), sort time " << fSortTime.wall() << " ms, num hits " << fNumHits << std::endl;
      return ss.str();
    }
  };

  /** @class Hitfind
 ** @brief Algo class for hitfinding
 ** @author Dominik Smith <d.smith@gsi.de>
 ** @since 16.10.2023
 **
 **/
  class Hitfind {

   public:
    typedef std::tuple<PartitionedVector<Hit>, HitfindMonitorData, PODVector<i32>> resultType;

    /** @brief Algorithm execution
     ** @param fles timeslice to hitfind
     ** @return pair: digi timeslice, monitoring data
     **
     ** @note Modifies input digis for time calibration
     **/
    resultType operator()(gsl::span<CbmTofDigi> digiIn);

    /** @brief Constructor **/
    explicit Hitfind(tof::HitfindSetup);

   private:  // members
    /** @brief TOF hitfinders (with unique RPC index for OpenMP) **/
    std::vector<tof::Clusterizer> fAlgo;  //[rpcUnique]

    /** @brief Number of SMs per super module type **/
    std::vector<int32_t> fNbSm;

    /** @brief Number of RPCs per super module type **/
    std::vector<int32_t> fNbRpc;

    /** @brief Intermediate storage variables (digi, index) **/
    std::vector<std::vector<std::vector<std::pair<CbmTofDigi, int32_t>>>> fStorDigi;  //[nbType][nbSm*nbRpc][nDigis]

    /** @brief Pointer to storage variables with unique RPC index (for OpenMP) **/
    std::vector<std::vector<std::pair<CbmTofDigi, int32_t>>*> fStorDigiPtr;  //[rpcUnique][nDigis]
  };
}  // namespace cbm::algo::tof

#endif /* TOFHITFIND_H */
