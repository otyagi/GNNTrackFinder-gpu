/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#include "WalkMap.h"

using namespace cbm::algo::sts;

CBM_YAML_INSTANTIATE(WalkMap);

std::vector<double> WalkMap::Get(int32_t modAddress, uint16_t asic)
{
  std::vector<double> result;
  auto modIter = fWalkMap.find(modAddress);
  if (modIter != fWalkMap.end()) {
    if (asic < modIter->second.size()) {
      result = modIter->second.at(asic).walkCoefficients;
    }
  }
  return result;
}
