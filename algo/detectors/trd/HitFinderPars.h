/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Felix Weiglhofer */
#pragma once

#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

#include <vector>

namespace cbm::algo::trd
{
  struct HitFinderPadPar {
    ROOT::Math::XYZVector pos;
    ROOT::Math::XYZVector posErr;  /// TO DO: probably not needed
  };

  struct HitFinderRowPar {
    std::vector<HitFinderPadPar> padPar;
  };

  struct HitFinderModPar {
    double padSizeX    = 0;
    double padSizeY    = 0;
    double padSizeErrX = 0;
    double padSizeErrY = 0;
    uint16_t address   = 0;
    int orientation    = 0;
    std::vector<HitFinderRowPar> rowPar;
    ROOT::Math::XYZVector translation;
    ROOT::Math::Rotation3D rotation;
  };
}  // namespace cbm::algo::trd
