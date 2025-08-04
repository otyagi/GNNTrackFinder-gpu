/* Copyright (C) 2022-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Shreya Roy. Pierre-Alain Loizeau, Volker Friese [committer], Dominik Smith, Sergei Zharko */

#ifndef CBM_ALGO_EVBUILD_DIGIEVENTSELECTORCONFIG_H
#define CBM_ALGO_EVBUILD_DIGIEVENTSELECTORCONFIG_H 1

#include "CbmDigiEvent.h"

#include <cstdint>
#include <map>
#include <unordered_set>

#include <yaml-cpp/yaml.h>


namespace cbm::algo::evbuild
{

  class EventbuildChain;


  /** @class DigiEventSelectorConfig
   ** @brief Configuration of the DigiEventSelector class
   ** @author Dominik Smith <d.smith@ gsi.de>
   ** @author Shreya Roy <s.roy@gsi.de>
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 26.01.2023
   **
   ** Holds minimum values for number of digis per event and detector system and for the
   ** number of active layers/stations (containing at least one digi).
   **
   ** None of the parameters are mandatory.
   **/
  class DigiEventSelectorConfig {

    friend class DigiEventSelector;

   public:
    /** @brief Constructor from YAML **/
    DigiEventSelectorConfig(YAML::Node config);

    /** @brief Presence of selection criteria **/
    bool IsEmpty() const { return fMinNumDigis.empty() && fMinNumLayers.empty(); }

    /** @brief Save to YAML **/
    YAML::Node ToYaml() const;

   private:
    std::map<ECbmModuleId, size_t> fMinNumDigis;   ///< Key: detector, value: minimal number of digis
    std::map<ECbmModuleId, size_t> fMinNumLayers;  ///< Key: detector, value: Minimal number of layers

    /** @brief A map of masked digi addresses, which should not participate in the event building **/
    std::map<ECbmModuleId, std::unordered_set<uint32_t>> fMaskedChannels;
  };


}  // namespace cbm::algo::evbuild

#endif /* CBM_ALGO_EVBUILD_DIGIEVENTSELECTORCONFIG_H */
