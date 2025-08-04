/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   BmonHitfindQaParameters.h
/// \brief  A BMON hitfinder QA parameter configuration
/// \since  10.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "bmon/CalibrateSetup.h"
#include "bmon/HitfindSetup.h"

#include <vector>

namespace cbm::algo::bmon
{
  /// \struct HitfindQaParameters
  /// \brief  A structure to handle BMON QA parameters
  struct HitfindQaParameters {
    /// \struct Diamond
    /// \brief  A diamond representation
    struct Diamond {
      double timeRes{0.};      ///< Time resolution [ns]
      double maxTimeDist{0.};  ///< Max time distance between digis in a hit [ns]
      int32_t address{0};      ///< Address of a diamond
      int32_t nChannels{0};    ///< Number of channels in a diamond
      uint32_t deadStrips{0};  ///< A bit mask of dead strips
    };

    uint32_t selectionMask{0};  ///< A bitmask to distinguish different diamonds
    std::vector<Diamond> diamonds{};

    /// \brief Default constructor
    HitfindQaParameters() = default;

    /// \brief Constructor
    /// \param calSetup  Calibration parameters
    /// \param hitSetup  Hitfinder parameters
    HitfindQaParameters(const CalibrateSetup& calSetup, const HitfindSetup& hitSetup);

    /// \brief Returns an index of the diamond by the address
    /// \param address  A hardware address of the digi
    size_t GetDiamondIndex(uint32_t address) const { return ((selectionMask & address) >> fSelectionBitsOffset); }

   private:
    uint32_t fSelectionBitsOffset{0};  ///< Number of bits to the right from the first bit in the selection mask
  };
}  // namespace cbm::algo::bmon
