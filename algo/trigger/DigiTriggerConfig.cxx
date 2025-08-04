/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "DigiTriggerConfig.h"


namespace cbm::algo::evbuild
{

  // -----   Constructor from YAML   ------------------------------------------
  DigiTriggerConfig::DigiTriggerConfig(YAML::Node config)
  {
    if (!config) {
      fIsSet = false;
      return;
    }

    auto detector = config["detector"];
    if (!detector) throw std::runtime_error("trigger detector is not specified");
    fDetector = ToCbmModuleIdCaseInsensitive(detector.as<std::string>());

    auto window = config["window"];
    if (!window) throw std::runtime_error("trigger window is not specified");
    fWindow = window.as<double>();

    auto threshold = config["threshold"];
    if (!threshold) throw std::runtime_error("trigger threshold is not specified");
    fThreshold = threshold.as<size_t>();

    auto deadTime = config["deadtime"];
    if (!deadTime) throw std::runtime_error("trigger dead time is not specified");
    fDeadTime = deadTime.as<double>();

    fIsSet = true;
  }
  // --------------------------------------------------------------------------


  // -----   Save to YAML   ---------------------------------------------------
  YAML::Node DigiTriggerConfig::ToYaml() const
  {
    YAML::Node result;
    result["detector"]  = ToString(fDetector);
    result["window"]    = fWindow;
    result["threshold"] = fThreshold;
    result["deadtime"]  = fDeadTime;
    return result;
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::evbuild
