/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#pragma once

#include "CbmTrdDigi.h"
#include "Cluster2D.h"
#include "HitFinder2DPars.h"

#include <tuple>
#include <vector>

namespace cbm::algo::trd
{

  //// TO DO: Add monitoring data here!!

  /** @class Clusterizer2D
 ** @brief Algo class for TRD2D cluster building
 ** @author Dominik Smith <d.smith@gsi.de>
 ** @since 05.04.2024
 **
 **/

  class Clusterizer2D {
   public:
    typedef std::tuple<uint16_t, uint16_t, int, int, size_t, const CbmTrdDigi*>
      inputType;  //Tuple: chT, chR, tm, row, id, digi

    /** \brief Default constructor.*/
    Clusterizer2D() = default;

    /** \brief Default constructor.*/
    Clusterizer2D(HitFinder2DModPar par) : fParams(par){};

    /** @brief Destructor **/
    virtual ~Clusterizer2D(){};

    /** @brief Execution
     ** @param  inVec       Digi data for one module 
     ** @param  t0          Start time of timeslice
     ** @return Vector of constructed clusters 
     **/
    std::vector<Cluster2D> operator()(const std::vector<std::pair<CbmTrdDigi, int32_t>>& inVec, uint64_t t0) const;

   protected:
   private:
    HitFinder2DModPar fParams;  ///< Parameter container
  };

}  // namespace cbm::algo::trd
