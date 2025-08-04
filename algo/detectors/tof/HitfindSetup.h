/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */
#ifndef CBM_ALGO_DETECTOR_TOF_HITFIND_SETUP_H
#define CBM_ALGO_DETECTOR_TOF_HITFIND_SETUP_H

#include "Definitions.h"
#include "yaml/Property.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace cbm::algo::tof
{

  /**
   * @brief Hitfind setup / Hardware cabling for TOF
   * Used to create the hardware mapping for the TOF hitfinder.
   */
  struct HitfindSetup {

    struct Cell {
      double sizeX;
      double sizeY;
      std::array<double, 3> translation;
      std::array<double, 9> rotation;

      CBM_YAML_PROPERTIES(yaml::Property(&Cell::sizeX, "sizeX", "size in X direction"),
                        yaml::Property(&Cell::sizeY, "sizeY", "size in Y direction"),
                        yaml::Property(&Cell::translation, "translation", "Translation vector", YAML::Flow),
                        yaml::Property(&Cell::rotation, "rotation", "Rotation matrix", YAML::Flow));
    };

    struct Channel {
      i32 address;

      CBM_YAML_PROPERTIES(yaml::Property(&Channel::address, "address", "unique address", YAML::Hex));
    };

    struct Rpc {
      u32 deadStrips;
      double posYMaxScal;
      double maxTimeDist;
      double maxSpaceDist;
      double sigVel;
      double timeRes;
      Cell cell;
      i32 trackingStationId;
      double CPTOffYBinWidth;
      double CPTOffYRange;
      std::vector<double> CPTOffY;
      std::vector<Channel> chanPar;

      CBM_YAML_PROPERTIES(yaml::Property(&Rpc::deadStrips, "deadStrips", "bit mask for dead strips"),
                        yaml::Property(&Rpc::posYMaxScal, "posYMaxScal", "maximum value of y position"),
                        yaml::Property(&Rpc::maxTimeDist, "maxTimeDist", "maximum time distance"),
                        yaml::Property(&Rpc::maxSpaceDist, "maxSpaceDist", "maximum space distance"),
                        yaml::Property(&Rpc::sigVel, "sigVel", "signal velocity"),
                        yaml::Property(&Rpc::timeRes, "timeRes", "time resolution"),
                        yaml::Property(&Rpc::cell, "cell", "cell parameters"),
                        yaml::Property(&Rpc::trackingStationId, "trackingStationId", "tracking station index"),
                        yaml::Property(&Rpc::CPTOffYBinWidth, "CPTOffYBinWidth", "CPT Y offset bin width"),
                        yaml::Property(&Rpc::CPTOffYRange, "CPTOffYRange", "CPT Y offset range"),
                        yaml::Property(&Rpc::CPTOffY, "CPTOffY", "CPT Y offset array"),
                        yaml::Property(&Rpc::chanPar, "chanPar", "channel parameters"));
    };

    std::vector<int32_t> NbSm;
    std::vector<int32_t> NbRpc;
    std::vector<std::vector<Rpc>> rpcs;

    CBM_YAML_PROPERTIES(yaml::Property(&HitfindSetup::NbSm, "NbSm", "Number of SMs per super module type", {}, YAML::Flow),
      yaml::Property(&HitfindSetup::NbRpc, "NbRpc", "Number of RPCs per super module type", {}, YAML::Flow),
      yaml::Property(&HitfindSetup::rpcs, "rpcs", "Parameters of RPCs"));
  };


}  // namespace cbm::algo::tof

#endif  // CBM_ALGO_DETECTOR_TOF_HITFIND_SETUP_H
