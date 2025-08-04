/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Dominik Smith */

#include "HitMultTrigger.h"

#include "CbmStsHit.h"
#include "CbmTofHit.h"
#include "CbmTrdHit.h"

#include <iterator>
#include <sstream>

#include <xpu/host.h>

namespace cbm::algo::evbuild
{

  // -----   Execution   ------------------------------------------------------
  HitMultTrigger::Result HitMultTrigger::operator()(const RecoResults& recoData) const
  {

    xpu::push_timer("HitMultTrigger");
    xpu::t_add_bytes(1);

    Result result;
    auto hitTimes             = GetHitTimes(recoData, fConfig.Detector());
    auto [triggers, moniData] = fAlgo(hitTimes);
    result.first              = std::move(triggers);
    result.second             = std::move(moniData);
    result.second.time        = xpu::pop_timer();

    L_(info) << "HitMultTrigger: hits " << hitTimes.size() << ", triggers " << result.first.size();

    return result;
  };
  // --------------------------------------------------------------------------


  // -----   Get hit times from reconstructed data   --------------------------
  std::vector<double> HitMultTrigger::GetHitTimes(const RecoResults& recoResults, ECbmModuleId system) const
  {
    std::vector<double> result;
    switch (system) {
      case ECbmModuleId::kSts: return GetTimeStamps<cbm::algo::sts::Hit>(recoResults.stsHits.Data()); break;
      case ECbmModuleId::kTrd: return GetTimeStamps<cbm::algo::trd::Hit>(recoResults.trdHits.Data()); break;
      case ECbmModuleId::kTof: return GetTimeStamps<cbm::algo::tof::Hit>(recoResults.tofHits.Data()); break;
      default: {
        L_(error) << "HitMultTrigger::GetHitTimes: Unknown system " << system;
        break;
      }
    }  //? system

    return result;
  }
  // --------------------------------------------------------------------------


  // -----   Info to string ---------------------------------------------------
  std::string HitMultTrigger::ToString() const
  {
    std::stringstream out;
    out << "--- Using Hit Multiplicity Trigger with trigger detector " << ::ToString(fConfig.Detector());
    out << "\n" << fAlgo.ToString();
    return out.str();
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::evbuild
