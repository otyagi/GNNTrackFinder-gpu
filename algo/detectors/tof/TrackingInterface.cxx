/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingInterface.cxx
/// \date   19.04.2024
/// \brief  A TOF-parameter and geometry interface used for tracking input data initialization (source)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "TrackingInterface.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTofAddress.h"
#include "HitfindSetup.h"
#include "ParFiles.h"
#include "fmt/format.h"

using cbm::algo::tof::HitfindSetup;
using cbm::algo::tof::TrackingInterface;

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingInterface::Init()
{
  L_(info) << "TOF: TrackingInterface initialization";

  // Read NbSm, NbRpc and tracking station ids from the config
  if (!this->GetContext()) {  // Default parameters for tests
    L_(info) << "TOF: TrackingInterface: no context found, using default setup";
    // Par: tof_v21j_mcbm.digibdf.par
    // Hardcoded setup, required for DigiEventSelector unittest
    fvNofSm  = {5, 0, 1, 0, 0, 1, 1, 1, 0, 1};
    fvNofRpc = {5, 3, 5, 1, 1, 1, 2, 2, 1, 2};
    fvTrackingStationId.resize(fvNofSm.size());
    fvTrackingStationId[0] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};
    fvTrackingStationId[1] = {};
    fvTrackingStationId[2] = {2, 2, 2, 2, 2};
    fvTrackingStationId[3] = {};
    fvTrackingStationId[4] = {};
    fvTrackingStationId[5] = {0};  // BMON
    fvTrackingStationId[6] = {1, 1};
    fvTrackingStationId[7] = {1, 1};
    fvTrackingStationId[8] = {};
    fvTrackingStationId[9] = {2, 2};
  }
  else {
    ParFiles parFiles(Opts().RunId());
    auto setup = yaml::ReadFromFile<HitfindSetup>(Opts().ParamsDir() / parFiles.tof.hitfinder);
    fvNofSm    = std::move(setup.NbSm);
    fvNofRpc   = std::move(setup.NbRpc);
    assert(fvNofSm.size() == fvNofRpc.size());
    int nSmTypes = fvNofSm.size();
    fvTrackingStationId.resize(nSmTypes);
    for (int iSmType = 0; iSmType < nSmTypes; ++iSmType) {
      int nSm        = fvNofSm[iSmType];
      int nRpc       = fvNofRpc[iSmType];
      auto& vStaIds  = fvTrackingStationId[iSmType];
      auto& vRpcPars = setup.rpcs[iSmType];
      vStaIds.resize(nSm * nRpc);
      for (int iSm = 0; iSm < nSm; ++iSm) {
        for (int iRpc = 0; iRpc < nRpc; ++iRpc) {
          int iGlobRpc      = iSm * nRpc + iRpc;
          vStaIds[iGlobRpc] = vRpcPars[iGlobRpc].trackingStationId;
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
int TrackingInterface::GetTrackingStation(uint32_t address) const
{
  int iSmType = CbmTofAddress::GetSmType(address);
  if (5 == iSmType) {
    return -1;  // Bmon hit
  }

  int iSm  = CbmTofAddress::GetSmId(address);
  int iRpc = CbmTofAddress::GetRpcId(address);
  if (iSmType < int(fvNofSm.size())) {
    if (iSm < fvNofSm[iSmType] && iRpc < fvNofRpc[iSmType]) {
      return fvTrackingStationId[iSmType][iSm * fvNofRpc[iSmType] + iRpc];
    }
  }

  L_(error) << "Undefined RPC address " << address << " (iSmType = " << iSmType << ", iSm = " << iSm
            << ", iRpc = " << iRpc << ')';
  return -1;  // iSmType, iSm or iRpc are out of range
}
