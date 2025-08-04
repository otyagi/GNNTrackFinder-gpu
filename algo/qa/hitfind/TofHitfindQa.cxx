/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TofHitfindQa.cxx
/// \brief  A TOF hitfinder QA (implementation)
/// \since  04.03.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "qa/hitfind/TofHitfindQa.h"

#include "CbmTofAddress.h"
#include "qa/Histogram.h"
#include "tof/Hit.h"

#include <fmt/format.h>

using cbm::algo::tof::HitfindQa;

// ---------------------------------------------------------------------------------------------------------------------
//
void HitfindQa::Init()
try {
  using fmt::format;
  using qa::CanvasConfig;
  using qa::PadConfig;
  if (!IsActive()) {
    return;
  }

  size_t nRpcs = fParameters.rpcs.size();
  if (nRpcs < 1) {
    throw std::runtime_error("parameters were not initialized. Please, provide the configuration using the function "
                             "HitfindQa::InitParameters(hitSetup)");
  }

  // Histogram initialization
  fvphRpcHitOccupX.resize(nRpcs, nullptr);
  fvphRpcHitOccupY.resize(nRpcs, nullptr);
  fvphRpcHitOccupCh.resize(nRpcs, nullptr);

  auto cOccupX  = CanvasConfig(format("{}/tof_rpc_occup_x", GetTaskName()), "RPC hit occupancy vs. X", 1, 1);
  auto cOccupY  = CanvasConfig(format("{}/tof_rpc_occup_y", GetTaskName()), "RPC hit occupancy vs. Y", 1, 1);
  auto cOccupCh = CanvasConfig(format("{}/tof_rpc_occup_ch", GetTaskName()), "RPC hit occupancy vs. channel", 1, 1);
  for (size_t iRpc = 0; iRpc < nRpcs; ++iRpc) {
    const auto& rpcPar = fParameters.rpcs[iRpc];
    int nCh            = rpcPar.chAddresses.size();
    auto sDN           = format("{:#010x}", rpcPar.address);  // diamond suffix
    auto sDT           = CbmTofAddress::ToString(rpcPar.address);

    // Histograms initialisation
    fvphRpcHitOccupX[iRpc]  = MakeObj<qa::H1D>(format("tof_hit_occup_x_{}", sDN), format("RPC {};x [cm];counts", sDT),
                                              kHitOccupB, kHitOccupL, kHitOccupU);
    fvphRpcHitOccupY[iRpc]  = MakeObj<qa::H1D>(format("tof_hit_occup_y_{}", sDN), format("RPC {};y [cm];counts", sDT),
                                              kHitOccupB, kHitOccupL, kHitOccupU);
    fvphRpcHitOccupCh[iRpc] = MakeObj<qa::H1D>(format("tof_hit_occup_chan_{}", sDN),
                                               format("RPC {};channel;counts", sDT), nCh, -0.5, nCh - 0.5);

    {
      auto pad = PadConfig(fvphRpcHitOccupX[iRpc], "hist");
      pad.SetLog(false, true, false);
      cOccupX.AddPadConfig(pad);
    }
    {
      auto pad = PadConfig(fvphRpcHitOccupY[iRpc], "hist");
      pad.SetLog(false, true, false);
      cOccupY.AddPadConfig(pad);
    }
    {
      auto pad = PadConfig(fvphRpcHitOccupCh[iRpc], "hist");
      pad.SetLog(false, true, false);
      cOccupCh.AddPadConfig(pad);
    }
  }

  AddCanvasConfig(cOccupX);
  AddCanvasConfig(cOccupY);
  AddCanvasConfig(cOccupCh);
}
catch (const std::exception& err) {
  L_(fatal) << "tof::HitfindQa: initialization failed. Reason: " << err.what();
  throw std::runtime_error("tof::HitfindQa initialization failure");
}

// ---------------------------------------------------------------------------------------------------------------------
//
void HitfindQa::Exec()
{
  if (!IsActive()) {
    return;
  }

  // Fill hit distributions
  const auto& hits = fpHits->Data();
  for (size_t iH = 0; iH < hits.size(); ++iH) {
    const auto& hit     = hits[iH];
    int32_t hitAddress  = hit.address;
    int32_t iCh         = CbmTofAddress::GetChannelId(hitAddress);
    uint32_t rpcAddress = CbmTofAddress::GetModFullId(hitAddress);
    auto itAddress      = fParameters.lookupMap.find(rpcAddress);
    if (itAddress == fParameters.lookupMap.end()) {
      L_(error) << "tof::HitfindQa: unknown RPC address " << CbmTofAddress::ToString(rpcAddress);
      continue;
    }
    uint32_t iRpc = itAddress->second;
    fvphRpcHitOccupX[iRpc]->Fill(hit.X());
    fvphRpcHitOccupY[iRpc]->Fill(hit.Y());
    fvphRpcHitOccupCh[iRpc]->Fill(iCh);
  }
}
