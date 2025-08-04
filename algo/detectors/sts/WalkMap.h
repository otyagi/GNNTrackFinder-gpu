/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#pragma once

#include "Definitions.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <map>
#include <vector>

namespace cbm::algo::sts
{

  class WalkMap {

   public:
    /**
     * @brief Construct emtpy mapping
     */
    WalkMap()  = default;
    ~WalkMap() = default;

    /** @brief API: Mapping from module address and ASIC number to walk coefficients.
     ** @param modAddress     module address
     ** @param asic           ASIC number within module
     ** @return std::vector<double> of walk coefficients
     */
    std::vector<double> Get(int32_t modAddress, uint16_t asic);

    /** @brief Get system time offset. **/
    i32 GetSystemTimeOffset() { return timeOffset; }

   private:
    // Move ADC into seperate class for easier YAML formatting
    struct AdcMap {
      // Map index: (ADC value), map value: (walk coefficient)
      std::vector<double> walkCoefficients;

      AdcMap() = default;
      AdcMap(std::initializer_list<double> coeffs) : walkCoefficients(coeffs) {}

      CBM_YAML_FORMAT(YAML::Flow);
      CBM_YAML_MERGE_PROPERTY();
      CBM_YAML_PROPERTIES(yaml::Property(&AdcMap::walkCoefficients, "walkCoefficients", "Walk coefficients"));
    };

    // --- STS system time offset
    i32 timeOffset;

    // --- STS walk correction map
    // --- Map index: (module address, ASIC number in module, ADC value), map value: (walk coefficient)
    std::map<i32, std::vector<AdcMap>> fWalkMap;

    CBM_YAML_MERGE_PROPERTY();
    CBM_YAML_PROPERTIES( yaml::Property(&WalkMap::timeOffset, "timeOffset", "Time offset for STS", YAML::Hex),
		    yaml::Property(&WalkMap::fWalkMap, "WalkMap", "Walk correction map", YAML::Hex, YAML::Flow));
  };

}  // namespace cbm::algo::sts

CBM_YAML_EXTERN_DECL(cbm::algo::sts::WalkMap);
