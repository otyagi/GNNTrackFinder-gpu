/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "Config.h"

#include <fstream>

#include <yaml-cpp/yaml.h>

namespace cbm::algo::evbuild
{

  // -----   Constructor from YAML   --------------------------------------------
  Config::Config(YAML::Node node)
    : fDigiTrigger(node["trigger"])
    , fHitMultTrigger(node["hit_mult_trigger"])
    , fV0Trigger(node["v0trigger"])
    , fBuilder(node["eventbuilder"])
    , fSelector(node["selector"])
  {
    if (!node) throw std::runtime_error("no configuration node for event building");
  }
  // ----------------------------------------------------------------------------


  // -----   Save configuration to YAML file   ----------------------------------
  YAML::Node Config::ToYaml() const
  {
    YAML::Node result;
    result["trigger"]          = fDigiTrigger.ToYaml();     // Digi trigger config
    result["hit_mult_trigger"] = fHitMultTrigger.ToYaml();  // Hit multiplicity trigger config
    result["v0trigger"]        = fV0Trigger.ToYaml();       // V0 trigger config
    result["eventbuilder"]     = fBuilder.ToYaml();         // Event builder config
    result["selector"]         = fSelector.ToYaml();        // Event selector config
    return result;
  }
  // ----------------------------------------------------------------------------

}  // namespace cbm::algo::evbuild
