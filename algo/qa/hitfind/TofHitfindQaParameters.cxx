/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   BmonHitfindQaParameters.cxx
/// \brief  A BMON hitfinder QA parameter configuration (implementation)
/// \since  10.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "qa/hitfind/TofHitfindQaParameters.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTofAddress.h"

using cbm::algo::tof::HitfindQaParameters;
using cbm::algo::tof::HitfindSetup;

// ---------------------------------------------------------------------------------------------------------------------
//
HitfindQaParameters::HitfindQaParameters(const HitfindSetup& hitSetup)
{
  rpcs.clear();
  lookupMap.clear();
  uint32_t iUniqueRpcId = 0;
  for (const auto& smType : hitSetup.rpcs) {
    for (const auto& rpc : smType) {
      uint32_t address   = static_cast<uint32_t>(CbmTofAddress::GetModFullId(rpc.chanPar[0].address));
      lookupMap[address] = iUniqueRpcId;
      auto& rpcPar       = rpcs.emplace_back();
      rpcPar.address     = address;
      rpcPar.chAddresses.resize(rpc.chanPar.size());
      L_(debug) << "RPC: " << CbmTofAddress::ToString(address);
      for (int iCh = 0; iCh < rpcPar.chAddresses.size(); ++iCh) {
        rpcPar.chAddresses[iCh] = rpc.chanPar[iCh].address;
        L_(debug) << "----> ch: " << CbmTofAddress::ToString(rpcPar.chAddresses[iCh]);
      }
      iUniqueRpcId++;
    }
  }
}
