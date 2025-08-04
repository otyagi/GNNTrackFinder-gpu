/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */
#pragma once

#include "Definitions.h"
#include "yaml/Property.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace cbm::algo::trd
{

  /**
   * @brief Hitfind setup / Hardware cabling for TRD
   * Used to create the hardware mapping for the TRD hitfinder.
   */
  struct HitfindSetup {

    struct Pad {
      std::array<double, 3> position;
      std::array<double, 3> positionError;

      CBM_YAML_PROPERTIES(yaml::Property(&Pad::position, "position", "Local position", YAML::Flow),
			yaml::Property(&Pad::positionError, "positionError", "Local position error", YAML::Flow));
    };

    struct Row {
      std::vector<Pad> padPar;

      CBM_YAML_PROPERTIES(yaml::Property(&Row::padPar, "padPar", "pad parameters"));
    };


    struct Mod {
      double padSizeX;
      double padSizeY;
      double padSizeErrX;
      double padSizeErrY;
      u16 address;
      int orientation;
      std::vector<Row> rowPar;
      std::array<double, 3> translation;
      std::array<double, 9> rotation;

      CBM_YAML_PROPERTIES(yaml::Property(&Mod::padSizeX, "padSizeX", "X size of pads"),
			yaml::Property(&Mod::padSizeY, "padSizeY", "Y size of pads"),
			yaml::Property(&Mod::padSizeErrX, "padSizeErrX", "error of X size of pads"),
			yaml::Property(&Mod::padSizeErrY, "padSizeErrY", "error of Y size of pads"),
                        yaml::Property(&Mod::address, "address", "module address"),
                        yaml::Property(&Mod::orientation, "orientation", "module orientation"),
                        yaml::Property(&Mod::translation, "translation", "Module position", YAML::Flow),
			yaml::Property(&Mod::rotation, "rotation", "Module rotation", YAML::Flow),
			yaml::Property(&Mod::rowPar, "rowPar", "row parameters"));
    };

    std::vector<Mod> modules;

    CBM_YAML_PROPERTIES(yaml::Property(&HitfindSetup::modules, "modules", "Parameters of modules"));
  };

}  // namespace cbm::algo::trd
