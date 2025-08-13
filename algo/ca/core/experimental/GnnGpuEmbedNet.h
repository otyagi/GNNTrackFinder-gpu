/* Copyright (C) 2021-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnEmbedNet.h
/// \brief Parameter container for the GPU CA library

#pragma once  // include this header only once per compilation unit

#include <xpu/device.h>

namespace cbm::algo::ca
{

  class GnnGpuEmbedNet {
   public:
    /// \brief Default constructor
    GnnGpuEmbedNet() = default;

    /// \brief Destructor
    ~GnnGpuEmbedNet() = default;

    std::array<std::array<float, 3>, 16> embedWeights_0;   ///< Layer 0
    std::array<std::array<float, 16>, 16> embedWeights_1;  ///< Layer 1
    std::array<std::array<float, 16>, 6> embedWeights_2;   ///< Layer 2

    std::array<float, 16> embedBias_0;  ///< Layer 0
    std::array<float, 16> embedBias_1;  ///< Layer 1
    std::array<float, 6> embedBias_2;   ///< Layer 2
  };
}  // namespace cbm::algo::ca
