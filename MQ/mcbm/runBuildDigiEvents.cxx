/* Copyright (C) 2021 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "CbmDeviceBuildDigiEvents.h"

#include <iomanip>
#include <string>

#include "runFairMQDevice.h"

namespace bpo = boost::program_options;
using namespace std;

void addCustomOptions(bpo::options_description& options)
{
  options.add_options()("FillHistos", bpo::value<bool>()->default_value(true),
                        "Fill histograms and send them to histo server if true");
  options.add_options()("IgnTsOver", bpo::value<bool>()->default_value(false), "Ignore TS overlap if true");
  options.add_options()("EvtOverMode", bpo::value<std::string>()->default_value("NoOverlap"),
                        "Set the event overlap mode, use string matching an EOverlapMode ");
  options.add_options()("RefDet", bpo::value<std::string>()->default_value("kBmon"),
                        "Set the reference (seed) detector, use string matching an ECbmModuleId ");
  options.add_options()("AddDet", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Add a detector for digis selection, use string matching an ECbmModuleId ");
  options.add_options()("DelDet", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Remove a detector for digis selection, use string matching an "
                        "ECbmModuleId ");
  options.add_options()("SetTrigWin", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Set trigger window for selected detector, use string matching "
                        "ECbmModuleId,dWinBeg,dWinEnd e.g. kSts,-10.5,100.0");
  options.add_options()("SetTrigMinNb", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Set minimum number of digis for selected detector, use string matching "
                        "ECbmModuleId,uMinNb e.g. kTof,10");
  options.add_options()("SetTrigMaxNb", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Set maximum number of digis for selected detector, use string matching "
                        "ECbmModuleId,uMaxNb e.g. kTof,10");
  options.add_options()("SetTrigMinLayersNb", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Set minimum number of fired layers for selected detector, use string matching "
                        "ECbmModuleId,uMinLayersNb e.g. kTof,3");
  options.add_options()("SetHistMaxDigiNb", bpo::value<std::vector<std::string>>()->multitoken()->composing(),
                        "Set max nb of digi in histograms for selected detector, use string matching "
                        "ECbmModuleId,dMaxDigiNb e.g. kTof,1000");
  options.add_options()("DoNotSend", bpo::value<bool>()->default_value(false), "Disable the sending of data if true");
  options.add_options()("DigiEventOutput", bpo::value<bool>()->default_value(false),
                        "Enable output of CbmDigiEvents instead of raw data + CbmEvents if true");
  options.add_options()("TsNameIn", bpo::value<std::string>()->default_value("unpts_0"),
                        "MQ channel name for unpacked TS data");
  options.add_options()("EvtNameOut", bpo::value<std::string>()->default_value("events"),
                        "MQ channel name for built events");
  options.add_options()("ChNameIn", bpo::value<std::string>()->default_value("histogram-in"),
                        "MQ channel name for histos");
  options.add_options()("ChNameHistCfg", bpo::value<std::string>()->default_value("histo-conf"),
                        "MQ channel name for histos config");
  options.add_options()("ChNameCanvCfg", bpo::value<std::string>()->default_value("canvas-conf"),
                        "MQ channel name for canvases config");
  options.add_options()("PubFreqTs", bpo::value<uint32_t>()->default_value(100), "Histo publishing frequency in TS");
  options.add_options()("PubTimeMin", bpo::value<double_t>()->default_value(1.0),
                        "Minimal time between two publishing");
  options.add_options()("PubTimeMax", bpo::value<double_t>()->default_value(10.0),
                        "Maximal time between two publishing");
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/) { return new CbmDeviceBuildDigiEvents(); }
