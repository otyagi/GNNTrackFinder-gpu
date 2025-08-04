/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau */

/*
   This algo was based on CbmTofEventClusterizer
*/

#ifndef CLUSTERIZERTOF_H
#define CLUSTERIZERTOF_H

// TOF Classes and includes
class CbmTofDigi;

#include "ClusterizerPars.h"
#include "Hit.h"

// C++ Classes and includes
#include <cmath>
#include <memory>
#include <vector>

namespace cbm::algo::tof
{
  class Clusterizer {
   public:
    typedef std::tuple<std::vector<Hit>, std::vector<size_t>, std::vector<u32>, std::vector<int32_t>> resultType;

    /**
       ** @brief Constructor.
       **/
    Clusterizer(ClusterizerRpcPar params) : fParams(params){};

    /**
       ** @brief Default constructor.
       **/
    Clusterizer() = default;

    /**
       ** @brief Build clusters out of ToF Digis and store the resulting info in a TofHit.
       **/
    resultType operator()(const std::vector<std::pair<CbmTofDigi, int32_t>>& digisIn);

   private:
    typedef std::vector<std::pair<const CbmTofDigi*, int32_t>> inputType;

    ClusterizerRpcPar fParams;  ///< Parameter container

    std::vector<inputType> chanSortDigis(const std::vector<std::pair<CbmTofDigi, int32_t>>& digisIn);

    resultType buildClusters(std::vector<inputType>& input);

    bool AddNextChan(std::vector<inputType>& input, int32_t iLastChan, Hit& cluster, std::vector<Hit>& clustersOut,
                     std::vector<int32_t>& digiIndRef, std::vector<inputType::iterator>* lastChanPos = nullptr);
  };

}  // namespace cbm::algo::tof

#endif  // CLUSTERIZERTOF_H
