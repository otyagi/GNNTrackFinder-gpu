/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Etienne Bechtel, Florian Uhlig */

#include "HitMerger.h"

namespace cbm::algo::trd
{

  //_______________________________________________________________________________
  HitMerger::HitMerger(HitFinderModPar params) : fParams(params) {}

  HitMerger::outputType HitMerger::operator()(std::vector<inputType>& hitsRow1, std::vector<inputType>& hitsRow2)
  {
    //// TO DO: Implement something here!

    return std::make_pair(std::move(hitsRow1), std::move(hitsRow2));
  }


}  // namespace cbm::algo::trd
