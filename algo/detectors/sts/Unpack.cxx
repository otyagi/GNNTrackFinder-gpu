/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#include "Unpack.h"

#include "AlgoFairloggerCompat.h"

using namespace cbm::algo::sts;
using fles::Subsystem;

Unpack::Unpack(const Config& config) : fConfig(config)
{
  uint32_t numChansPerAsicSts   = 128;  // R/O channels per ASIC for STS
  uint32_t numAsicsPerModuleSts = 16;   // Number of ASICs per module for STS

  constexpr u8 SystemVersion = 0x20;

  auto equipIdsSts = fConfig.readout.GetEquipmentIds();
  for (auto& equip : equipIdsSts) {
    sts::UnpackPar par{};
    par.fWriteAux          = fConfig.bCollectAuxData;
    par.fNumChansPerAsic   = numChansPerAsicSts;
    par.fNumAsicsPerModule = numAsicsPerModuleSts;
    const size_t numElinks = fConfig.readout.GetNumElinks(equip);
    for (size_t elink = 0; elink < numElinks; elink++) {
      sts::UnpackElinkPar elinkPar;
      auto mapEntry        = fConfig.readout.Map(equip, elink);
      elinkPar.fAddress    = mapEntry.moduleAddress;  // Module address for this elink
      elinkPar.fAsicNr     = mapEntry.asicNumber;     // ASIC number within module
      elinkPar.fTimeOffset = fConfig.walkMap.GetSystemTimeOffset();
      elinkPar.fAdcMinCut  = fConfig.readout.AdcCutMap(equip, elink);
      elinkPar.fAdcOffset  = 1.;
      elinkPar.fAdcGain    = 1.;
      elinkPar.fWalk       = fConfig.walkMap.Get(elinkPar.fAddress, elinkPar.fAsicNr);
      elinkPar.fChanMask   = fConfig.readout.MaskMap(equip, elink);
      // TODO: Add parameters for time and ADC calibration
      par.fElinkParams.push_back(elinkPar);
    }
    auto algo                      = std::make_unique<UnpackMS>(par);
    fAlgos[{equip, SystemVersion}] = std::move(algo);
    L_(debug) << "--- Configured equipment " << equip << " with " << numElinks << " elinks";
  }  //# equipments

  L_(info) << "--- Configured " << fAlgos.size() << " unpacker algorithms for STS.";
}

Unpack::Result_t Unpack::operator()(const fles::Timeslice& ts) const
{

  auto result = DoUnpack(Subsystem::STS, ts);
  // PrintDigisPerModule(result.first);
  return result;
}

void Unpack::PrintDigisPerModule(const PODVector<CbmStsDigi>& digis) const
{
  std::map<u32, size_t> digisPerModule;
  for (const auto& digi : digis) {
    digisPerModule[digi.GetAddress()]++;
  }
  for (const auto& [module, numDigis] : digisPerModule) {
    L_(info) << "Module " << std::hex << module << std::dec << " has " << numDigis << " digis";
  }
}
