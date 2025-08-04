/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingInterface.cxx
/// \date   30.04.2024
/// \brief  A TRD-parameter and geometry interface used for tracking input data initialization (source)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "TrackingInterface.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTrdAddress.h"

using cbm::algo::trd::TrackingInterface;

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingInterface::Init() { L_(info) << "TRD: TrackingInterface initialization"; }

// ---------------------------------------------------------------------------------------------------------------------
//
int TrackingInterface::GetTrackingStation(uint32_t address) const { return CbmTrdAddress::GetLayerId(address); }
