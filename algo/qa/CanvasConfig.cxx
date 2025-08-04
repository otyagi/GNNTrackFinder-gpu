/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CanvasConfig.cxx
/// \date   12.02.2024
/// \brief  A class representing a canvas in the message for the Histogram server (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "CanvasConfig.h"

#include "AlgoFairloggerCompat.h"

#include <sstream>

using cbm::algo::qa::CanvasConfig;
using cbm::algo::qa::PadConfig;

// ---------------------------------------------------------------------------------------------------------------------
//
CanvasConfig::CanvasConfig(std::string_view name, std::string_view title, int nPadsX, int nPadsY)
  : fsName(name)
  , fsTitle(title)
  , fNofPadsX(nPadsX)
  , fNofPadsY(nPadsY)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CanvasConfig::AddPadConfig(const PadConfig& pad)
{
  fvsPadConfigs.push_back(pad.ToString());

  // Re-calculate number of pads
  if (fNofPadsX * fNofPadsY < static_cast<int>(fvsPadConfigs.size())) {
    if (fNofPadsX > fNofPadsY) {
      ++fNofPadsY;
    }
    else {
      ++fNofPadsX;
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string CanvasConfig::ToString() const
{
  // TODO: What to return (throw), if 0 pads defined?

  std::stringstream cfg;
  cfg << fsName << ';' << fsTitle << ';' << fNofPadsX << ';' << fNofPadsY;
  if (fvsPadConfigs.empty()) {
    L_(warning) << "CanvasConfig::ToString(): creating a config message for an empty pad";
    auto pad = PadConfig();
    cfg << ';' << pad.ToString();
  }
  else {
    for (const auto& padCfg : fvsPadConfigs) {
      cfg << ';' << padCfg;
    }
  }
  return cfg.str() + ";";
}
