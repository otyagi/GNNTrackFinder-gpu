/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "MainConfig.h"

#include <fstream>

#include <yaml-cpp/yaml.h>

namespace cbm::algo
{

  // -----   Load configuration from YAML file   --------------------------------
  void MainConfig::LoadYaml(const std::string& filename)
  {
    YAML::Node config = YAML::LoadFile(filename);

    // --- Digi trigger
    fTriggerDet       = ToCbmModuleIdCaseInsensitive(config["trigger"]["detector"].as<std::string>());
    fTriggerWin       = config["trigger"]["window"].as<double>();
    fTriggerThreshold = config["trigger"]["threshold"].as<size_t>();
    fTriggerDeadTime  = config["trigger"]["deadtime"].as<double>();

    // --- Event builder: (detector -> (tMin, tMax))
    if (auto eventbuilder = config["eventbuilder"]) {
      if (auto windows = eventbuilder["windows"]) {
        for (YAML::const_iterator it = windows.begin(); it != windows.end(); ++it) {
          auto det              = ToCbmModuleIdCaseInsensitive(it->first.as<std::string>());
          auto lower            = it->second[0].as<double>();
          auto upper            = it->second[1].as<double>();
          fEvtbuildWindows[det] = std::make_pair(lower, upper);
        }
      }
    }

    // --- Event selector parameters
    fSelectMinStationsSts = config["selector"]["minStationsSts"].as<size_t>();
    fSelectMinStationsTof = config["selector"]["minStationsTof"].as<size_t>();
    fSelectMinDigisBmon   = config["selector"]["minDigisBmon"].as<size_t>();

    // --- Branch persistence in output file
    fStoreTimeslice = config["store"]["timeslice"].as<bool>();
    fStoreTrigger   = config["store"]["triggers"].as<bool>();
    fStoreEvents    = config["store"]["events"].as<bool>();

    // --- QA publishing
    fHttpServerRefreshRate = config["qa"]["refreshrate"].as<int32_t>(fHttpServerRefreshRate);
  }
  // ----------------------------------------------------------------------------


  // -----   Save configuration to YAML file   ----------------------------------
  void MainConfig::SaveYaml(const std::string& filename)
  {
    YAML::Node config;

    // --- Digi trigger
    config["trigger"]["detector"]  = ToString(fTriggerDet);
    config["trigger"]["window"]    = fTriggerWin;
    config["trigger"]["threshold"] = fTriggerThreshold;
    config["trigger"]["deadtime"]  = fTriggerDeadTime;

    // --- Event builder: (detector -> (tMin, tMax))
    for (const auto& [key, value] : fEvtbuildWindows) {
      auto det = ToString(key);
      config["eventbuilder"]["windows"][det].push_back(value.first);
      config["eventbuilder"]["windows"][det].push_back(value.second);
    };

    // --- Event selector
    config["selector"]["minStationsSts"] = fSelectMinStationsSts;
    config["selector"]["minStationsTof"] = fSelectMinStationsTof;
    config["selector"]["minDigisBmon"]   = fSelectMinDigisBmon;


    // --- Branch persistence in output file
    config["store"]["timeslice"] = fStoreTimeslice;
    config["store"]["triggers"]  = fStoreTrigger;
    config["store"]["events"]    = fStoreEvents;
    // --- QA publishing
    config["qa"]["refreshrate"] = fHttpServerRefreshRate;
    // ---
    std::ofstream fout(filename);
    fout << config;
  }
  // ----------------------------------------------------------------------------

} /* namespace cbm::algo */
