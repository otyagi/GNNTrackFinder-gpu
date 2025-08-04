/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TofHitfindQaParameters.h
/// \brief  A TOF hitfinder QA parameter configuration
/// \since  03.03.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "tof/HitfindSetup.h"

#include <unordered_map>
#include <vector>

namespace cbm::algo::tof
{
  /// \struct HitfindQaParameters
  /// \brief  A structure to handle TOF QA parameters
  struct HitfindQaParameters {

    /// \struct Rpc
    struct Rpc {
      std::vector<uint32_t> chAddresses;  ///< Address of channel vs channel ID
      uint32_t address;                   ///< Address of RPC
    };

    std::unordered_map<uint32_t, uint32_t> lookupMap;  ///< A lookup map for RPC addresses
    std::vector<Rpc> rpcs;                             ///< A map for different RPC properties

    /// \brief Default constructor
    HitfindQaParameters() = default;

    /// \brief Constructor
    /// \param hitSetup  Hitfinder parameters
    HitfindQaParameters(const HitfindSetup& hitSetup);
  };
}  // namespace cbm::algo::tof
