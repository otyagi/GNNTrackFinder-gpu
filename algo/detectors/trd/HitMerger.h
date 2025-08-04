/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Etienne Bechtel, Florian Uhlig */

#pragma once

#include "DigiRec.h"
#include "Hit.h"
#include "HitFinderPars.h"

#include <tuple>
#include <vector>

namespace cbm::algo::trd
{
  /**
  * \brief Rectangular pad module; Hit merging 
  **/
  class HitMerger {
   public:
    typedef std::pair<Hit, std::vector<DigiRec>> inputType;
    typedef std::pair<std::vector<inputType>, std::vector<inputType>> outputType;

    HitMerger(){};
    /**
  * \brief Constructor with placement
  **/
    HitMerger(HitFinderModPar params);
    virtual ~HitMerger(){};

    /* \brief Steering routine for building hits */
    outputType operator()(std::vector<inputType>& hitsRow1, std::vector<inputType>& hitsRow2);


   protected:
   private:
    HitMerger(const HitMerger& ref);
    const HitMerger& operator=(const HitMerger& ref);

    HitFinderModPar fParams;  ///< Parameter container
  };

}  // namespace cbm::algo::trd
