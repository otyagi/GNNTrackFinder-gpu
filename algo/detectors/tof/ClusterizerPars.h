/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Felix Weiglhofer */
#pragma once

#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

#include <cstdint>
#include <vector>

namespace cbm::algo::tof
{
  struct TofCell {
    double sizeX, sizeY;
    ROOT::Math::XYZVector pos;
    ROOT::Math::Rotation3D rotation;
  };

  struct ClusterizerChanPar {
    int32_t address;  //unique address
    TofCell cell;
  };

  struct ClusterizerRpcPar {
    uint32_t fDeadStrips;
    double fPosYMaxScal;
    double fdMaxTimeDist;
    double fdMaxSpaceDist;
    double fSigVel;
    double fTimeRes;
    double fCPTOffYBinWidth;
    double fCPTOffYRange;
    std::vector<double> fCPTOffY;  //[nBin]
    std::vector<ClusterizerChanPar> fChanPar = {};
  };

}  // namespace cbm::algo::tof
