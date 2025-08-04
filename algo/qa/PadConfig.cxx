/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Pad.cxx
/// \date   12.02.2024
/// \brief  A class representing a Pad in the message for the Histogram server (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "PadConfig.h"

#include "AlgoFairloggerCompat.h"

#include <sstream>

using cbm::algo::qa::PadConfig;

// ---------------------------------------------------------------------------------------------------------------------
//
void PadConfig::SetGrid(bool gridX, bool gridY)
{
  fbGridX = gridX;
  fbGridY = gridY;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void PadConfig::SetLog(bool logX, bool logY, bool logZ)
{
  fbLogX = logX;
  fbLogY = logY;
  fbLogZ = logZ;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string PadConfig::ToString() const
{
  std::stringstream cfg;
  cfg << fbGridX << ',' << fbGridY << ',' << fbLogX << ',' << fbLogY << ',' << fbLogZ;
  if (fvObjectList.empty()) {
    L_(warning) << "PadConfig::ToString(): creating a config message for an empty pad";
    cfg << ",(nullptr,nullptr)";
  }
  else {
    for (const auto& [name, opt] : fvObjectList) {
      cfg << ",(" << name << ',' << opt << ')';
    }
  }
  return cfg.str();
}
