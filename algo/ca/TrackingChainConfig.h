/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingChainConfig.h
/// \date   18.02.2024
/// \brief  A configuration reader for the TrackingChain class
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "yaml/Property.h"

#include <string>
#include <tuple>

namespace cbm::algo
{
  /// \struct TrackingChainConfig
  /// \brief Configuration reader for the TrackingChain class
  struct TrackingChainConfig {
    std::string
      fsGeomConfig;  ///< Tracking geometry file name (TMP: includes all other settings, but the settings are rewritten)
    std::string fsSetupFilename;  ///< Geometry setup input file
    std::string fsMainConfig;     ///< Main configuration file (rel path in online parameters directory)
    std::string fsUserConfig;     ///< User configuration file (full path)
    std::string fsMoniOutName;    ///< Monitor output file name
    bool fbStoreMonitor;          ///< Stores monitor snapshot

    CBM_YAML_PROPERTIES(
                      yaml::Property(&TrackingChainConfig::fsGeomConfig, "GeomConfigName", "CA geometry input"),
                      yaml::Property(&TrackingChainConfig::fsSetupFilename, "SetupFilename", "CA geometry setup"),
                      yaml::Property(&TrackingChainConfig::fsMainConfig, "MainConfigName", "Main cofniguration"),
                      yaml::Property(&TrackingChainConfig::fsUserConfig, "UserConfigName", "User cofniguration"),
                      yaml::Property(&TrackingChainConfig::fsMoniOutName, "MoniOutName", "Monitor output"),
                      yaml::Property(&TrackingChainConfig::fbStoreMonitor, "StoreMonitor", "If store monitor"));
  };
}  // namespace cbm::algo
