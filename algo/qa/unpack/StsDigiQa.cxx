/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   StsDigiQa.h
/// \date   03.03.2024
/// \brief  QA module for STS raw digis (source)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "StsDigiQa.h"

#include "CbmStsAddress.h"

#include <fmt/format.h>

using cbm::algo::sts::DigiQa;
using fmt::format;

// ---------------------------------------------------------------------------------------------------------------------
//
void DigiQa::Init()
{
  using cbm::algo::qa::CanvasConfig;
  using cbm::algo::qa::Data;
  using cbm::algo::qa::EHistFlag;
  using cbm::algo::qa::H1D;
  using cbm::algo::qa::H2D;
  using cbm::algo::qa::PadConfig;
  using cbm::algo::qa::Prof1D;

  if (!fpSender.get()) {
    return;
  }

  int nModules = fpReadoutSetup->modules.size();

  auto GetAddressName = [&](int32_t address) -> std::string {
    uint32_t u = CbmStsAddress::GetElementId(address, kStsUnit);
    uint32_t l = CbmStsAddress::GetElementId(address, kStsLadder);
    uint32_t m = CbmStsAddress::GetElementId(address, kStsModule);
    return format("u{}_l{}_m{}", u, l, m);
  };

  auto GetAddressTitle = [&](int32_t address) -> std::string {
    uint32_t u = CbmStsAddress::GetElementId(address, kStsUnit);
    uint32_t l = CbmStsAddress::GetElementId(address, kStsLadder);
    uint32_t m = CbmStsAddress::GetElementId(address, kStsModule);
    return format("U{} L{} M{}", u, l, m);
  };

  // Histograms per address
  {
    fvphAddressChannel.resize(nModules);
    fvphAddressCharge.resize(nModules);
    fvphAddressChannelCharge.resize(nModules);
    if (fbAux) {
      fvphAddressChannelElink.resize(nModules);
      fvppAddressChannelMissedEvt.resize(nModules);
      fvppAddressTimeMissedEvt.resize(nModules);
    }
    for (int iM = 0; iM < nModules; ++iM) {
      int32_t address       = fpReadoutSetup->modules.at(iM).address;
      fmAddressMap[address] = iM;
      auto aName            = GetAddressName(address);
      auto aTitl            = GetAddressTitle(address);
      auto cName            = format("sts_digi/sts_digi_vs_channel_charge_{}", aName);
      auto cTitl            = format("STS digis per channel and charge for module {}", aTitl);
      auto canv             = CanvasConfig(cName, cTitl, 3, fbAux ? 2 : 1);
      {
        auto pad               = PadConfig();
        auto name              = format("sts_digi_{}_channel", aName);
        auto titl              = format("Number of digis per channel for module {};channel;N_{{digis}}", aTitl);
        fvphAddressChannel[iM] = fQaData.MakeObj<H1D>(name, titl, 2048, -0.5, 2047.5);
        if (fbAux) {
          fvphAddressChannel[iM]->SetFlag(EHistFlag::StoreVsTsId);
        }
        pad.RegisterHistogram(fvphAddressChannel[iM], "hist");
        canv.AddPadConfig(pad);
      }
      {
        auto pad              = PadConfig();
        auto name             = format("sts_digi_{}_charge", aName);
        auto titl             = format("STS digi charge for mudule {};charge [ADS units];N_{{digis}}", aTitl);
        fvphAddressCharge[iM] = fQaData.MakeObj<H1D>(name, titl, 32, -0.5, 31.5);
        pad.RegisterHistogram(fvphAddressCharge[iM], "hist");
        canv.AddPadConfig(pad);
      }
      {
        auto pad  = PadConfig();
        auto name                    = format("sts_digi_{}_channel_charge", aName);
        auto titl                    = format("STS digi charge for module {};charge [ADS units];channel", aTitl);
        fvphAddressChannelCharge[iM] = fQaData.MakeObj<H2D>(name, titl, 32, -0.5, 31.5, 2048, -0.5, 2047.5);
        pad.RegisterHistogram(fvphAddressChannelCharge[iM], "colz");
        canv.AddPadConfig(pad);
      }
      if (fbAux) {
        {
          auto pad                    = PadConfig();
          auto name                   = format("sts_digi_{}_channel_elink", aName);
          auto titl                   = format("STS digi charge for module {};E-link;channel", aTitl);
          fvphAddressChannelElink[iM] = fQaData.MakeObj<H2D>(name, titl, 40, -0.5, 39.5, 2048, -0.5, 2047.5);
          pad.RegisterHistogram(fvphAddressChannelElink[iM], "colz");
          canv.AddPadConfig(pad);
        }
        {
          auto pad  = PadConfig();
          auto name = format("sts_digi_{}_channel_missed_evt", aName);
          auto titl =
            format("STS digi missed event ratio for module {};channel; N_{{w/ missed events}} / N_{{tot.}}", aTitl);
          fvppAddressChannelMissedEvt[iM] = fQaData.MakeObj<Prof1D>(name, titl, 2048, -0.5, 2047.5, 0., 1.);
          pad.RegisterHistogram(fvppAddressChannelMissedEvt[iM], "");
          canv.AddPadConfig(pad);
        }
        {
          auto pad  = PadConfig();
          auto name = format("sts_digi_{}_time_missed_evt", aName);
          auto titl =
            format("STS digi missed event ratio for module {};time [ms]; N_{{w/ missed events}} / N_{{tot.}}", aTitl);
          constexpr double timeMin     = 0.;    // [ms]
          constexpr double timeMax     = 150.;  // [ms]
          int timeBins                 = std::ceil(timeMax - timeMin);
          fvppAddressTimeMissedEvt[iM] = fQaData.MakeObj<Prof1D>(name, titl, timeBins, timeMin, timeMax, 0., 1.);
          fvppAddressTimeMissedEvt[iM]->SetFlag(EHistFlag::StoreVsTsId);
          fvppAddressTimeMissedEvt[iM]->SetFlag(EHistFlag::OmitIntegrated);
          pad.RegisterHistogram(fvppAddressTimeMissedEvt[iM], "");
          canv.AddPadConfig(pad);
        }
      }

      fQaData.AddCanvasConfig(canv);
    }
  }

  fQaData.Init(fpSender);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void DigiQa::Exec()
{
  if (!fpSender.get()) {
    return;
  }

  // --- Loop over STS digis and fill histograms
  for (const auto& digi : (*fpvDigis)) {
    int32_t address = digi.GetAddress();
    double channel  = digi.GetChannel();
    double charge   = digi.GetCharge();

    //  Ensure, that the address is defined
    auto itHistID = fmAddressMap.find(address);
    if (itHistID == fmAddressMap.end()) {
      L_(error) << format("sts::DigiQa: found address {:x}, which is not defined in the ReadoutSetup config", address);
      continue;
    }

    int iM = itHistID->second;
    fvphAddressChannel[iM]->Fill(channel);
    fvphAddressCharge[iM]->Fill(charge);
    fvphAddressChannelCharge[iM]->Fill(charge, channel);
  }

  // --- Loop over aux QA digis
  if (fbAux) {
    for (const auto& ms : fpAuxDigis->msAux) {
      for (const auto& auxDigi : ms.fQaDigis) {
        int32_t address = auxDigi.address;
        auto itHistID   = fmAddressMap.find(address);
        if (itHistID == fmAddressMap.end()) {
          L_(error) << format("sts::DigiQa: found address {:x}, which is not defined in the ReadoutSetup config",
                              address);
          continue;
        }
        int iM = itHistID->second;
        fvphAddressChannelElink[iM]->Fill(static_cast<double>(auxDigi.elink), static_cast<double>(auxDigi.channel));
        fvppAddressChannelMissedEvt[iM]->Fill(static_cast<double>(auxDigi.channel),
                                              static_cast<double>(auxDigi.missedEvent));
        fvppAddressTimeMissedEvt[iM]->Fill(auxDigi.time * 1.e-6, static_cast<double>(auxDigi.missedEvent));
      }
    }
  }

  fQaData.Send(fpSender);
}
