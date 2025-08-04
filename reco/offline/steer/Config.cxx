/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file .cxx
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 31.05.2023
 **/


#include "Config.h"

#include <Logger.h>

#include <fstream>

#include <yaml-cpp/yaml.h>

using std::string;

namespace cbm::reco::offline
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
    f_glb_mode       = ToCbmRecoMode(settings["global"]["mode"].as<string>());
    f_glb_numTs      = settings["global"]["nTimeslices"].as<Int_t>();
    f_glb_firstTs    = settings["global"]["firstTimeslice"].as<Int_t>();

    // --- Event builder
    f_evbuild_type       = ToCbmEvbuildType(settings["evbuild"]["type"].as<string>());
    f_evbuild_ovlapmode  = ToOverlapModeRaw(settings["evbuild"]["overlap"].as<string>());
    f_evbuild_trigDet    = ToCbmModuleIdCaseInsensitive(settings["evbuild"]["triggerDetector"].as<string>());
    f_evbuild_trigNumMin = settings["evbuild"]["minNumDigis"].as<int64_t>();
    f_evbuild_trigNumMax = settings["evbuild"]["maxNumDigis"].as<int64_t>();
    f_evbuild_trigWinMin = settings["evbuild"]["trigWinMin"].as<float>();
    f_evbuild_trigWinMax = settings["evbuild"]["trigWinMax"].as<float>();

    // --- STS
    f_sts_usegpu = settings["sts"]["usegpu"].as<bool>();

    // --- TRD
    f_trd_trigThresh = settings["trd"]["trigThresh"].as<double>();

    // --- littrack
    f_lit_trackType = settings["littrack"]["trackingType"].as<string>();
    f_lit_mergeType = settings["littrack"]["mergingType"].as<string>();
  }
  // ----------------------------------------------------------------------------


  // ------   String to ECbmEvbuildType   ---------------------------------------
  ECbmEvbuildType Config::ToCbmEvbuildType(string choice)
  {
    string temp = choice;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "ideal")
      return ECbmEvbuildType::Ideal;
    else if (temp == "real")
      return ECbmEvbuildType::Real;
    else
      return ECbmEvbuildType::Undefined;
  }
  // ----------------------------------------------------------------------------


  // ------   String to ECbmRecoMode   ------------------------------------------
  ECbmRecoMode Config::ToCbmRecoMode(string choice)
  {
    string temp = choice;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "timeslice")
      return ECbmRecoMode::Timeslice;
    else if (temp == "event")
      return ECbmRecoMode::EventByEvent;
    else
      return ECbmRecoMode::Undefined;
  }
  // ----------------------------------------------------------------------------


  // ------   String to EOverlapModeRaw   ---------------------------------------
  EOverlapModeRaw Config::ToOverlapModeRaw(string choice)
  {
    string temp = choice;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return std::tolower(c); });
    if (temp == "no")
      return EOverlapModeRaw::NoOverlap;
    else if (temp == "allow")
      return EOverlapModeRaw::AllowOverlap;
    else if (temp == "merge")
      return EOverlapModeRaw::MergeOverlap;
    else
      return EOverlapModeRaw::Undefined;
  }
  // ----------------------------------------------------------------------------


  // -----   ECbmEvbuildType to string   ----------------------------------------
  string Config::ToString(ECbmEvbuildType type)
  {
    if (type == ECbmEvbuildType::Ideal)
      return "ideal";
    else if (type == ECbmEvbuildType::Real)
      return "real";
    else
      return "undefined";
  }
  // ----------------------------------------------------------------------------


  // -----   ECbmRecoMode to string   -------------------------------------------
  string Config::ToString(ECbmRecoMode mode)
  {
    if (mode == ECbmRecoMode::Timeslice)
      return "timeslice";
    else if (mode == ECbmRecoMode::EventByEvent)
      return "event";
    else
      return "undefined";
  }
  // ----------------------------------------------------------------------------


  // -----   EOverlapMode to string   -------------------------------------------
  string Config::ToString(EOverlapModeRaw mode)
  {
    if (mode == EOverlapModeRaw::NoOverlap)
      return "no";
    else if (mode == EOverlapModeRaw::AllowOverlap)
      return "allow";
    else if (mode == EOverlapModeRaw::MergeOverlap)
      return "merge";
    else
      return "undefined";
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

    // --- Event builder
    settings["evbuild"]["type"]            = ToString(f_evbuild_type);
    settings["evbuild"]["overlap"]         = ToString(f_evbuild_ovlapmode);
    settings["evbuild"]["triggerDetector"] = ::ToString(f_evbuild_trigDet);
    settings["evbuild"]["minNumDigis"]     = f_evbuild_trigNumMin;
    settings["evbuild"]["maxNumDigis"]     = f_evbuild_trigNumMax;
    settings["evbuild"]["trigWinMin"]      = f_evbuild_trigWinMin;
    settings["evbuild"]["trigWinMax"]      = f_evbuild_trigWinMax;

    // --- STS
    settings["sts"]["usegpu"] = f_sts_usegpu;

    // --- TRD
    settings["trd"]["trigThresh"] = f_trd_trigThresh;

    // --- littrack
    settings["littrack"]["trackingType"] = f_lit_trackType;
    settings["littrack"]["mergingType"]  = f_lit_mergeType;

    return settings;
  }
  // ----------------------------------------------------------------------------

}  // namespace cbm::reco::offline
