/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   HitfindSetup.h
/// \brief  Parameters of the BMON hitfinder
/// \since  06.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "Definitions.h"
#include "yaml/Property.h"

#include <string>
#include <vector>

namespace cbm::algo::bmon
{
  /// \struct HitfindSetup
  /// \brief  Parameters for the BMON hitfinder
  struct HitfindSetup {
    struct Diamond {
      u32 refAddress;
      u32 deadStrips;
      double maxTimeDist;
      double timeRes;

      CBM_YAML_PROPERTIES(
                        yaml::Property(&Diamond::refAddress, "refAddress", "reference address of the diamond"),
                        yaml::Property(&Diamond::deadStrips, "deadStrips", "bit mask for dead strips"),
                        yaml::Property(&Diamond::maxTimeDist, "maxTimeDist", "maximum time distance"),
                        yaml::Property(&Diamond::timeRes, "timeRes", "time resolution"));
    };

    uint32_t selectionMask;
    std::vector<Diamond> diamonds;

    CBM_YAML_PROPERTIES(
      yaml::Property(&HitfindSetup::selectionMask, "selectionMask", "A bit mask to distinguish between different diamonds"),
      yaml::Property(&HitfindSetup::diamonds, "diamonds", "Parameters of diamonds"));
  };
}  // namespace cbm::algo::bmon
