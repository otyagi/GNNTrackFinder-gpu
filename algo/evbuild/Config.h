/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


#ifndef ALGO_EVBUILD_CONFIG_H
#define ALGO_EVBUILD_CONFIG_H 1

#include "DigiEventSelectorConfig.h"
#include "DigiTriggerConfig.h"
#include "EventBuilderConfig.h"
#include "V0Trigger.h"

#include <yaml-cpp/yaml.h>

namespace cbm::algo::evbuild
{

  /** @class Config
   ** @brief Configuration of digi event building
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 10 July 2023
   **/
  class Config {
   public:  // methods
    /** @brief Constructor from YAML **/
    Config(YAML::Node node);

    /** @brief Save to YAML file **/
    YAML::Node ToYaml() const;

   public:                              // data members
    DigiTriggerConfig fDigiTrigger;     ///< Digi trigger configuration
    DigiTriggerConfig fHitMultTrigger;  ///< Hit multiplicity trigger configuration
    V0TriggerConfig fV0Trigger;         ///< V0 trigger configuration
    EventBuilderConfig fBuilder;        ///< Event builder configuration
    DigiEventSelectorConfig fSelector;  ///< Event selector configuration
  };

}  // namespace cbm::algo::evbuild

#endif /* ALGO_EVBUILD_CONFIG_H */
