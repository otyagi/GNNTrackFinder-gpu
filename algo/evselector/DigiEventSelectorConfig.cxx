/* Copyright (C) 2022-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Shreya Roy. Pierre-Alain Loizeau, Volker Friese [committer], Dominik Smith, Sergei Zharko */

#include "DigiEventSelectorConfig.h"

#include "AlgoFairloggerCompat.h"

#include <iomanip>

namespace cbm::algo::evbuild
{

  // -----   Constructor from YAML   ------------------------------------------
  DigiEventSelectorConfig::DigiEventSelectorConfig(YAML::Node config)
  {
    if (!config) return;
    if (auto numDigis = config["minDigis"]) {
      for (YAML::const_iterator it = numDigis.begin(); it != numDigis.end(); it++) {
        auto det   = ToCbmModuleIdCaseInsensitive(it->first.as<std::string>());
        auto value = it->second.as<size_t>();
        if (value > 0)
          fMinNumDigis[det] = value;
        else
          L_(warning) << "DigiEventSelectorConfig: Ignoring minimum 0 for digis in " << ::ToString(det);
      }
    }
    if (auto numLayers = config["minLayers"]) {
      for (YAML::const_iterator it = numLayers.begin(); it != numLayers.end(); it++) {
        auto det   = ToCbmModuleIdCaseInsensitive(it->first.as<std::string>());
        auto value = it->second.as<size_t>();
        if (value > 0)
          fMinNumLayers[det] = value;
        else
          L_(warning) << "DigiEventSelectorConfig: Ignoring minimum 0 for layers in " << ::ToString(det);
      }
    }
    if (auto maskedChannels = config["maskedChannels"]) {
      for (YAML::const_iterator it = maskedChannels.begin(); it != maskedChannels.end(); ++it) {
        auto det   = ToCbmModuleIdCaseInsensitive(it->first.as<std::string>());
        auto value = it->second.as<std::vector<uint32_t>>();
        if (value.size() > 0) {
          fMaskedChannels[det] = std::unordered_set<uint32_t>(value.begin(), value.end());
          L_(info) << "Masking following " << ToString(det) << " channels from event selecting: ";
          for (auto address : fMaskedChannels[det]) {
            L_(info) << " - 0x" << std::hex << std::setw(8) << std::setfill('0') << address << std::dec;
          }
        }
      }
    }
  }
  // --------------------------------------------------------------------------


  // -----   Save to YAML   ---------------------------------------------------
  YAML::Node DigiEventSelectorConfig::ToYaml() const
  {
    YAML::Node result;
    for (const auto& entry : fMinNumDigis) {
      auto det                = ToString(entry.first);
      result["minDigis"][det] = entry.second;
    }
    for (const auto& entry : fMinNumLayers) {
      auto det                 = ToString(entry.first);
      result["minLayers"][det] = entry.second;
    }
    // FIXME: implement masked channels storage
    return result;
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::evbuild
