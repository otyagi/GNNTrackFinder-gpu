/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file .cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 30.10.2023
 **/


#include "Config.h"

#include <Logger.h>

#include <fstream>

#include <yaml-cpp/yaml.h>

using std::string;

namespace cbm::atconverter
{


  // -----   Load settings from YAML file   -------------------------------------
  void Config::LoadYaml(const string& fileName)
  {

    LOG(info) << "Config: Reading configuration from " << fileName;
    YAML::Node settings = YAML::LoadFile(fileName);

    // --- Global settings
    f_glb_logLevel      = settings["global"]["log_level"].as<string>();
    f_glb_logVerbose    = settings["global"]["log_verbose"].as<string>();
    f_glb_logColor      = settings["global"]["log_color"].as<string>();
    f_glb_mode          = ToCbmRecoMode(settings["global"]["mode"].as<string>());
    f_glb_numTs         = settings["global"]["nTimeslices"].as<int>();
    f_glb_firstTs       = settings["global"]["firstTimeslice"].as<int>();
    f_glb_tslength      = settings["global"]["timeslicelength"].as<float>();
    f_glb_system        = settings["global"]["collisionSystem"].as<string>();
    f_glb_beamMom       = settings["global"]["beamMomentum"].as<float>();
    f_glb_trackMatching = settings["global"]["trackMatching"].as<bool>();

    // --- Fsd hits converter settings
    f_fsd_minChi2match = settings["fsdhits"]["gtrackmatch_minChi2"].as<double>();
    f_fsd_maxChi2match = settings["fsdhits"]["gtrackmatch_maxChi2"].as<double>();
  }
  // ----------------------------------------------------------------------------


  // ------   String to ECbmRecoMode   ------------------------------------------
  ECbmRecoMode Config::ToCbmRecoMode(string choice)
  {
    string temp = choice;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "timeslice") return ECbmRecoMode::Timeslice;
    else if (temp == "event")
      return ECbmRecoMode::EventByEvent;
    else
      return ECbmRecoMode::Undefined;
  }
  // ----------------------------------------------------------------------------


  // -----   ECbmRecoMode to string   -------------------------------------------
  string Config::ToString(ECbmRecoMode mode)
  {
    if (mode == ECbmRecoMode::Timeslice) return "timeslice";
    else if (mode == ECbmRecoMode::EventByEvent)
      return "event";
    else
      return "undefined";
  }
  // ----------------------------------------------------------------------------


  // -----   Save settings to YAML node   ---------------------------------------
  YAML::Node Config::ToYaml()
  {
    YAML::Node settings;

    // --- Global settings
    settings["global"]["log_level"]       = f_glb_logLevel;
    settings["global"]["log_verbose"]     = f_glb_logVerbose;
    settings["global"]["log_color"]       = f_glb_logColor;
    settings["global"]["mode"]            = ToString(f_glb_mode);
    settings["global"]["nTimeslices"]     = f_glb_numTs;
    settings["global"]["firstTimeslice"]  = f_glb_firstTs;
    settings["global"]["timeslicelength"] = f_glb_tslength;
    settings["global"]["collisionSystem"] = f_glb_system;
    settings["global"]["beamMomentum"]    = f_glb_beamMom;
    settings["global"]["trackMatching"]   = f_glb_trackMatching;

    // --- Fsd hits converter settings
    settings["fsdhits"]["gtrackmatch_minChi2"] = f_fsd_minChi2match;
    settings["fsdhits"]["gtrackmatch_maxChi2"] = f_fsd_maxChi2match;

    return settings;
  }
  // ----------------------------------------------------------------------------

}  // namespace cbm::atconverter
