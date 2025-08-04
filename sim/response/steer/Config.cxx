/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file .cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 10.01.2024
 **/


#include "Config.h"

#include <Logger.h>

#include <fstream>

#include <yaml-cpp/yaml.h>

using std::string;

namespace cbm::sim::digitization
{


  // -----   Load settings from YAML file   -------------------------------------
  void Config::LoadYaml(const string& fileName)
  {

    LOG(info) << "Config: Reading configuration from " << fileName;
    YAML::Node settings = YAML::LoadFile(fileName);

    // --- Global settings
    f_glb_logLevel   = settings["global"]["log_level"].as<string>();
    f_glb_logVerbose = settings["global"]["log_verbose"].as<string>();
    f_glb_logColor   = settings["global"]["log_color"].as<string>();
    f_glb_mode       = ToCbmDigiMode(settings["global"]["mode"].as<string>());
    f_glb_numTs      = settings["global"]["nTimeslices"].as<int>();
    f_glb_firstTs    = settings["global"]["firstTimeslice"].as<int>();

    // --- Detector choice
    f_det_deactivateAllBut = ToECbmModuleId(settings["detectors"]["deactivateAllBut"].as<string>());

    YAML::Node deactivatedSystems = settings["detectors"]["deactivate"];
    for (auto det : deactivatedSystems)
      f_det_deactivate.push_back(ToECbmModuleId(det.as<string>()));

    // --- Timeslice settings
    f_ts_tslength   = settings["timeslice"]["timeslicelength"].as<float>();
    f_ts_storeAllTS = settings["timeslice"]["storeAllTimeslices"].as<bool>();
    f_ts_startTime  = settings["timeslice"]["startTime"].as<float>();
    f_ts_timeDist   = ToCbmSimTimeDist(settings["timeslice"]["timeDist"].as<string>());

    // --- Background settings
    f_bg_produceNoise = settings["background"]["produceNoise"].as<bool>();

    // --- Input sources
    YAML::Node sources = settings["sources"];

    for (auto source : sources) {
      f_src_id.push_back(source["id"].as<int>());
      f_src_rate.push_back(source["rate"].as<float>());
      f_src_treeAccessMode.push_back(ToECbmTreeAccess(source["treeAccessMode"].as<string>()));
      f_src_embedToId.push_back(source["embedToId"].as<int>());
    }
  }
  // ----------------------------------------------------------------------------


  // ------   String to ECbmRecoMode   ------------------------------------------
  cbm::sim::Mode Config::ToCbmDigiMode(string choice)
  {
    string temp = choice;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "timeslice")
      return cbm::sim::Mode::Timebased;
    else if (temp == "event")
      return cbm::sim::Mode::EventByEvent;
    else
      return cbm::sim::Mode::Undefined;
  }
  // ----------------------------------------------------------------------------


  // -----   ECbmRecoMode to string   -------------------------------------------
  string Config::ToString(cbm::sim::Mode mode)
  {
    if (mode == cbm::sim::Mode::Timebased)
      return "timeslice";
    else if (mode == cbm::sim::Mode::EventByEvent)
      return "event";
    else
      return "undefined";
  }
  // ----------------------------------------------------------------------------


  // -----   String to cbm::sim::TimeDist   -------------------------------------
  cbm::sim::TimeDist Config::ToCbmSimTimeDist(string choice)
  {
    string temp = choice;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "poisson")
      return cbm::sim::TimeDist::Poisson;
    else if (temp == "uniform")
      return cbm::sim::TimeDist::Uniform;
    else
      return cbm::sim::TimeDist::Undefined;
  }
  // ----------------------------------------------------------------------------


  // -----   cbm::sim::TimeDist to string   -------------------------------------
  string Config::ToString(cbm::sim::TimeDist dist)
  {
    if (dist == cbm::sim::TimeDist::Poisson)
      return "poisson";
    else if (dist == cbm::sim::TimeDist::Uniform)
      return "uniform";
    else
      return "undefined";
  }
  // ----------------------------------------------------------------------------


  // -----  Convert std::string to ECbmModuleId  --------------------------------
  ECbmModuleId Config::ToECbmModuleId(std::string moduleString)
  {
    std::string temp = moduleString;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "mvd")
      return ECbmModuleId::kMvd;
    else if (temp == "sts")
      return ECbmModuleId::kSts;
    else if (temp == "rich")
      return ECbmModuleId::kRich;
    else if (temp == "much")
      return ECbmModuleId::kMuch;
    else if (temp == "trd")
      return ECbmModuleId::kTrd;
    else if (temp == "tof")
      return ECbmModuleId::kTof;
    else if (temp == "psd")
      return ECbmModuleId::kPsd;
    else if (temp == "fsd")
      return ECbmModuleId::kFsd;
    else if (temp == "bmon")
      return ECbmModuleId::kBmon;
    else
      return ECbmModuleId::kNotExist;
  }
  // ----------------------------------------------------------------------------


  // ----- Convert ECbmModuleId to std::string  ---------------------------------
  string Config::ToString(ECbmModuleId moduleId)
  {
    if (moduleId == ECbmModuleId::kMvd)
      return "mvd";
    else if (moduleId == ECbmModuleId::kSts)
      return "sts";
    else if (moduleId == ECbmModuleId::kRich)
      return "rich";
    else if (moduleId == ECbmModuleId::kMuch)
      return "much";
    else if (moduleId == ECbmModuleId::kTrd)
      return "trd";
    else if (moduleId == ECbmModuleId::kTof)
      return "tof";
    else if (moduleId == ECbmModuleId::kPsd)
      return "psd";
    else if (moduleId == ECbmModuleId::kFsd)
      return "fsd";
    else if (moduleId == ECbmModuleId::kBmon)
      return "bmon";
    else
      return "";
  }
  // ----------------------------------------------------------------------------


  // -----  Convert std::string to ECbmTreeAccess  ------------------------------
  ECbmTreeAccess Config::ToECbmTreeAccess(std::string treeAccessString)
  {
    std::string temp = treeAccessString;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "random")
      return ECbmTreeAccess::kRandom;
    else if (temp == "repeat")
      return ECbmTreeAccess::kRepeat;
    else
      return ECbmTreeAccess::kRegular;
  }
  // ----------------------------------------------------------------------------


  // -----  Convert ECbmTreeAccess to std::string  ------------------------------
  string Config::ToString(ECbmTreeAccess treeAccessString)
  {
    if (treeAccessString == ECbmTreeAccess::kRandom)
      return "random";
    else if (treeAccessString == ECbmTreeAccess::kRepeat)
      return "repeat";
    else
      return "regular";
  }
  // ----------------------------------------------------------------------------


  // -----   Save settings to YAML node   ---------------------------------------
  YAML::Node Config::ToYaml()
  {
    YAML::Node settings;

    // --- Global settings
    settings["global"]["log_level"]      = f_glb_logLevel;
    settings["global"]["log_verbose"]    = f_glb_logVerbose;
    settings["global"]["log_color"]      = f_glb_logColor;
    settings["global"]["mode"]           = ToString(f_glb_mode);
    settings["global"]["nTimeslices"]    = f_glb_numTs;
    settings["global"]["firstTimeslice"] = f_glb_firstTs;

    // --- Detector choice
    settings["detectors"]["deactivateAllBut"] = ToString(f_det_deactivateAllBut);

    for (int i = 0; i < (int) f_det_deactivate.size(); ++i) {
      settings["detectors"]["deactivate"][i] = ToString(f_det_deactivate.at(i));
    }

    // --- Timeslice settings
    settings["timeslice"]["timeslicelength"]    = f_ts_tslength;
    settings["timeslice"]["storeAllTimeslices"] = f_ts_storeAllTS;
    settings["timeslice"]["startTime"]          = f_ts_startTime;
    settings["timeslice"]["timeDist"]           = ToString(f_ts_timeDist);

    // --- Background settings
    settings["background"]["produceNoise"] = f_bg_produceNoise;

    // --- Input sources
    for (int i = 0; i < (int) f_src_id.size(); ++i) {
      settings["sources"][i]["id"]             = f_src_id.at(i);
      settings["sources"][i]["rate"]           = f_src_rate.at(i);
      settings["sources"][i]["treeAccessMode"] = ToString(f_src_treeAccessMode.at(i));
      settings["sources"][i]["embedToId"]      = f_src_embedToId.at(i);
    }

    return settings;
  }
  // ----------------------------------------------------------------------------

}  // namespace cbm::sim::digitization
