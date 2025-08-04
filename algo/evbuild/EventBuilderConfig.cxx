/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "EventBuilderConfig.h"


namespace cbm::algo::evbuild
{

  // -----   Constructor from YAML   ------------------------------------------
  EventBuilderConfig::EventBuilderConfig(YAML::Node config)
  {
    if (!config) throw std::runtime_error("EventBuilderConfig: no configuration node");
    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
      auto det      = ToCbmModuleIdCaseInsensitive(it->first.as<std::string>());
      auto lower    = it->second[0].as<double>();
      auto upper    = it->second[1].as<double>();
      fWindows[det] = std::make_pair(lower, upper);
    }
    // TODO: Check if complete (should the parameters be mandatory?)
  }
  // --------------------------------------------------------------------------


  // -----   Save to YAML   ---------------------------------------------------
  YAML::Node EventBuilderConfig::ToYaml() const
  {
    YAML::Node result;
    for (const auto& entry : fWindows) {
      auto det    = ToString(entry.first);
      result[det] = entry.second;
    }
    return result;
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::evbuild
