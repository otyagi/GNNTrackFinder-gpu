/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Etienne Bechtel, Florian Uhlig */

#pragma once

#include "CbmTrdDigi.h"
#include "Cluster.h"
#include "HitFinderPars.h"

#include <tuple>
#include <vector>

namespace cbm::algo::trd
{

  //// TO DO: Add monitoring data here!!

  /** @class Clusterizer
 ** @brief Algo class for TRD cluster building
 ** @author Dominik Smith <d.smith@gsi.de>
 ** @since 09.04.2024
 **
 **/

  class Clusterizer {
   public:
    typedef std::tuple<int, const CbmTrdDigi*, double>
      inputType;  //digi index, pointer to digi (null if processed), digi time

    /** \brief Default constructor.*/
    Clusterizer() = default;

    /** \brief Default constructor.*/
    Clusterizer(HitFinderModPar par) : fParams(par){};

    /** @brief Destructor **/
    virtual ~Clusterizer(){};

    /** @brief Execution
     ** @param  inVec       Digi data for one module 
     ** @return Vector of constructed clusters 
     **/
    std::vector<Cluster> operator()(const std::vector<std::pair<CbmTrdDigi, int32_t>>& inVec) const;

   protected:
   private:
    HitFinderModPar fParams;  ///< Parameter container

    bool TryAddDigi(std::vector<std::vector<inputType*>>* digimap, int chan,
                    std::vector<std::vector<inputType*>::iterator>* lastPos,
                    std::vector<std::pair<int, const CbmTrdDigi*>>* cluster, const double digiTime) const;

    void addClusters(std::vector<std::pair<int, const CbmTrdDigi*>> cluster, std::vector<Cluster>* clustersOut) const;
  };

}  // namespace cbm::algo::trd
