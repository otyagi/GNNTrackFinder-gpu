/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CalibrateSetup.h
/// \brief  Configuration of the calibrator for the BMON digis
/// \since  04.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "Definitions.h"
#include "yaml/Property.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace cbm::algo::bmon
{
  /// \struct  CalibrateSetup
  /// \brief   BMON calibration per channel
  struct CalibrateSetup {
    // FIXME: remove 'v' from non-vector variable names
    struct Channel {
      double vCPTOff;
      double vCPTotGain;
      double vCPTotOff;
      std::vector<double> vCPWalk;

      CBM_YAML_PROPERTIES(yaml::Property(&Channel::vCPTOff, "vCPTOff", "CPT offset"),
                        yaml::Property(&Channel::vCPTotGain, "vCPTotGain", "CP time over threshold gain"),
                        yaml::Property(&Channel::vCPTotOff, "vCPTotOff", "CP time over threshold offset"),
                        yaml::Property(&Channel::vCPWalk, "vCPWalk", "CP walk correction", YAML::Block, YAML::Flow));
    };

    struct Diamond {
      uint32_t refAddress;
      int32_t numClWalkBinX;
      double TOTMax;
      double TOTMin;
      double channelDeadtime;
      std::vector<Channel> chanPar;

      CBM_YAML_PROPERTIES(
                        yaml::Property(&Diamond::refAddress, "refAddress", "reference HW address to distinguish this BMON"),
                        yaml::Property(&Diamond::numClWalkBinX, "numClWalkBinX", "number of walk correction bins"),
                        yaml::Property(&Diamond::TOTMax, "TOTMax", "maximum time over threshold"),
                        yaml::Property(&Diamond::TOTMin, "TOTMin", "minimum time over threshold"),
                        yaml::Property(&Diamond::channelDeadtime, "channelDeadtime", "channel dead time"),
                        yaml::Property(&Diamond::chanPar, "chanPar", "channel parameters"));
    };

    /* Members */
    uint32_t selectionMask;
    std::vector<Diamond> diamonds;

    CBM_YAML_PROPERTIES(
      yaml::Property(&CalibrateSetup::selectionMask, "selectionMask", "A bit mask to distinguish between different diamonds"),
      yaml::Property(&CalibrateSetup::diamonds, "diamonds", "Parameters of each diamond"));
  };

}  // namespace cbm::algo::bmon
