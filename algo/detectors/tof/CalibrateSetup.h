/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */
#ifndef CBM_ALGO_DETECTOR_TOF_CALIBRATE_SETUP_H
#define CBM_ALGO_DETECTOR_TOF_CALIBRATE_SETUP_H

#include "Definitions.h"
#include "yaml/Property.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace cbm::algo::tof
{
  /**
   * @brief TOF calibration setup
   */
  struct CalibrateSetup {

    struct Channel {
      std::vector<double> vCPTOff;
      std::vector<double> vCPTotGain;
      std::vector<double> vCPTotOff;
      std::vector<std::vector<double>> vCPWalk;

      CBM_YAML_PROPERTIES(yaml::Property(&Channel::vCPTOff, "vCPTOff", "CPT offset"),
                        yaml::Property(&Channel::vCPTotGain, "vCPTotGain", "CP time over threshold gain"),
                        yaml::Property(&Channel::vCPTotOff, "vCPTotOff", "CP time over threshold offset"),
                        yaml::Property(&Channel::vCPWalk, "vCPWalk", "CP walk correction", YAML::Block, YAML::Flow));
    };

    struct Rpc {
      i32 numClWalkBinX;
      double TOTMax;
      double TOTMin;
      bool swapChannelSides;
      double channelDeadtime;
      std::vector<Channel> chanPar;

      CBM_YAML_PROPERTIES(yaml::Property(&Rpc::numClWalkBinX, "numClWalkBinX", "number of walk correction bins"),
                        yaml::Property(&Rpc::TOTMax, "TOTMax", "maximum time over threshold"),
                        yaml::Property(&Rpc::TOTMin, "TOTMin", "minimum time over threshold"),
                        yaml::Property(&Rpc::swapChannelSides, "swapChannelSides", "flag for swapping channel sides"),
                        yaml::Property(&Rpc::channelDeadtime, "channelDeadtime", "channel dead time"),
                        yaml::Property(&Rpc::chanPar, "chanPar", "channel parameters"));
    };

    /* Members */
    std::vector<int32_t> NbSm;
    std::vector<int32_t> NbRpc;
    std::vector<std::vector<Rpc>> rpcs;

    CBM_YAML_PROPERTIES(
      yaml::Property(&CalibrateSetup::NbSm, "NbSm", "Number of SMs per super module type", {}, YAML::Flow),
      yaml::Property(&CalibrateSetup::NbRpc, "NbRpc", "Number of RPCs per super module type", {}, YAML::Flow),
      yaml::Property(&CalibrateSetup::rpcs, "rpcs", "Parameters of RPCs"));
  };

}  // namespace cbm::algo::tof

#endif  // CBM_ALGO_DETECTOR_TOF_CALIBRATE_SETUP_H
