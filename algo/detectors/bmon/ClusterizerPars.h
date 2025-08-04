/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   ClusterizerPars.h
/// \brief  BMON clusterizer parameters
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include <cstdint>
#include <vector>

namespace cbm::algo::bmon
{
  /// \struct ClusterizerPars
  /// \brief  Clusterizer parameters for Diamond
  struct ClusterizerPars {
    uint32_t fAddress;     ///< Address of the diamond (the channel bit field is 0)
    uint32_t fDeadStrips;  ///< Dead strip bitmask
    double fdMaxTimeDist;  ///< Maximum time difference between two consecutive digis to form a single hit
    double fTimeRes;       ///< Time resolution
  };
}  // namespace cbm::algo::bmon
