/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   BmonHitfindQa.cxx
/// \brief  A BMON hitfinder QA (implementation)
/// \since  09.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "qa/hitfind/BmonHitfindQa.h"

#include "CbmBmonDigi.h"
#include "CbmTofAddress.h"
#include "bmon/Hit.h"
#include "qa/Histogram.h"

#include <fmt/format.h>

using cbm::algo::bmon::HitfindQa;

// ---------------------------------------------------------------------------------------------------------------------
//
void HitfindQa::Init()
try {
  using fmt::format;
  if (!IsActive()) {
    return;
  }

  size_t nDiamonds = fParameters.diamonds.size();
  if (nDiamonds < 1) {
    throw std::runtime_error("parameters were not initialized. Please, provide the configuration using the function "
                             "HitfindQa::InitParameters(calSetup, hitSetup)");
  }

  fvphDigiOccupVsChan.resize(nDiamonds, nullptr);
  fvphDigiChargeVsChan.resize(nDiamonds, nullptr);
  fvphHitNofChan.resize(nDiamonds, nullptr);
  fvphHitTimeDiff.resize(nDiamonds, nullptr);

  for (size_t iD = 0; iD < nDiamonds; ++iD) {
    const auto& diamondPar = fParameters.diamonds[iD];
    int nCh                = diamondPar.nChannels;
    auto sDN               = format("_diamond_{:#08x}", diamondPar.address);  // diamond suffix

    // Histograms initialisation
    /* clang-format off */
    fvphDigiOccupVsChan[iD] = MakeObj<qa::H1D>(
      format("bmon_digi_occup_channel{}", sDN),
      format("BMON-{} digi occupancy vs. channel;channel;counts", iD), 
      nCh, -0.5, nCh - 0.5);
    fvphDigiChargeVsChan[iD] = MakeObj<qa::H2D>(
      format("bmon_digi_charge_channel{}", sDN),
      format("BMON-{} digi charge vs. channel;channel;charge;counts", iD),
      nCh, -0.5, nCh - 0.5, kChrgB, kChrgL, kChrgU);
    fvphHitNofChan[iD]       = MakeObj<qa::H1D>(
      format("bmon_hit_nChannels{}", sDN),
      format("BMON-{} hit number of channels;N_{{chan}};counts", iD), 
      2, 0.5, 2.5);
    fvphHitTimeDiff[iD]      = MakeObj<qa::H1D>(
      format("bmon_hit_time_diff{}", sDN),
      format("BMON-{} digi time difference in a hit formed from two digis;#Delta t_{{digi}} [ns];counts", iD), 
      kDtimeB, kDtimeL, kDtimeU);
    /* clang-format on */

    // Canvas initialization
    auto cName = format("{}/bmon{}", GetTaskName(), sDN);
    auto cTitl = format("BMON-{}", iD);
    auto canv  = qa::CanvasConfig(cName, cTitl, 2, 2);
    canv.AddPadConfig(qa::PadConfig(fvphDigiOccupVsChan[iD], "hist"));   // (0,0)
    canv.AddPadConfig(qa::PadConfig(fvphDigiChargeVsChan[iD], "colz"));  // (1,0)
    canv.AddPadConfig(qa::PadConfig(fvphHitNofChan[iD], "hist"));        // (0,1)
    canv.AddPadConfig(qa::PadConfig(fvphHitTimeDiff[iD], "hist"));       // (1,1)
    AddCanvasConfig(canv);
  }
}
catch (const std::exception& err) {
  L_(fatal) << "bmon::HitfindQa: initialization failed. Reason: " << err.what();
  throw std::runtime_error("bmon::HitfindQa initialization failure");
}

// ---------------------------------------------------------------------------------------------------------------------
//
void HitfindQa::Exec()
{
  if (!IsActive()) {
    return;
  }

  // Fill digi distributions
  for (const auto& digi : *fpDigis) {
    size_t iDiamond = fParameters.GetDiamondIndex(digi.GetAddress());
    int32_t chan    = digi.GetChannel();
    fvphDigiOccupVsChan[iDiamond]->Fill(chan);
    fvphDigiChargeVsChan[iDiamond]->Fill(chan, digi.GetCharge());
  }

  // Fill hit distributions
  const auto& hits = fpHits->Data();
  for (size_t iH = 0; iH < hits.size(); ++iH) {
    const auto& hit = hits[iH];
    size_t iDiamond = fParameters.GetDiamondIndex(hit.GetAddress());
    int nChannels   = hit.GetNofChannels();
    fvphHitNofChan[iDiamond]->Fill(nChannels);
    if (nChannels == 2) {
      int32_t iDigi     = (*fpDigiIndices)[iH];
      const auto& digiF = (*fpDigis)[iDigi];
      const auto& digiS = (*fpDigis)[iDigi + 1];
      fvphHitTimeDiff[iDiamond]->Fill(digiS.GetTime() - digiF.GetTime());
    }
  }
}
