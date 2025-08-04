/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingInterface.cxx
/// \date   19.04.2024
/// \brief  A TOF-parameter and geometry interface used for tracking input data initialization (source)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "TrackingInterface.h"

#include "AlgoFairloggerCompat.h"
#include "CbmStsAddress.h"

using cbm::algo::sts::TrackingInterface;

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingInterface::Init() { L_(info) << "STS: TrackingInterface initialization"; }

// ---------------------------------------------------------------------------------------------------------------------
//
int TrackingInterface::GetTrackingStation(uint32_t address) const
{
  // NOTE: SZh 30.04.2024:
  //   This returns the valid STS station index at least for mCBM at beamtimes 2022 and 2024. There is no
  //   guarantie, that it will work for other setups, should be cross-checked.
  return CbmStsAddress::GetElementId(address, kStsUnit);
}
