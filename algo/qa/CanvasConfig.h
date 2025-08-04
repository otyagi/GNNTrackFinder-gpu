/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CanvasConfig.h
/// \date   12.02.2024
/// \brief  A class representing a canvas in the message for the Histogram server
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "PadConfig.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cbm::algo::qa
{
  /// \class CanvasConfig
  /// \brief A canvas configuration for the histogram server
  ///
  /// The class represents a configuration of the canvas, which can be converted to the initialization
  /// message for the histogram server.
  class CanvasConfig {
   public:
    /// \brief Constructor
    /// \param name    Name of the canvas
    /// \param title   Title of the canvas
    /// \param nPadsX  Number of pads along x-axis
    /// \param nPadsY  Number of pads along y-axis
    CanvasConfig(std::string_view name, std::string_view title, int nPadsX = 1, int nPadsY = 1);

    /// \brief Copy constructor
    CanvasConfig(const CanvasConfig&) = default;

    /// \brief Move constructor
    CanvasConfig(CanvasConfig&&) = default;

    /// \brief Copy assignment operator
    CanvasConfig& operator=(const CanvasConfig&) = default;

    /// \brief Move assignment operator
    CanvasConfig& operator=(CanvasConfig&&) = default;

    /// \brief Destructor
    ~CanvasConfig() = default;

    /// \brief Adds a pad to the canvas
    void AddPadConfig(const PadConfig& pad);

    /// \brief Returns message config
    std::string ToString() const;

   private:
    std::string fsName;                      ///< Name of the canvas
    std::string fsTitle;                     ///< Name of the pad
    std::vector<std::string> fvsPadConfigs;  ///< Vector of pad config messages
    int fNofPadsX = 1;                       ///< Number of pads along the x-axis
    int fNofPadsY = 1;                       ///< Number of pads along the y-axis
  };
}  // namespace cbm::algo
