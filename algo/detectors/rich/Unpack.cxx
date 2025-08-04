/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#include "Unpack.h"

#include "AlgoFairloggerCompat.h"

using namespace cbm::algo::rich;
using fles::Subsystem;

Unpack::Unpack(const ReadoutConfig& readout) : fReadout(readout)
{
  constexpr i64 SystemTimeOffset = 100;
  constexpr u8 SystemVersion     = 0x03;


  // Create one algorithm per component for Bmon and configure it with parameters
  auto equipIdsRich = fReadout.GetEquipmentIds();
  for (auto& equip : equipIdsRich) {
    rich::UnpackPar par{};
    std::map<uint32_t, std::vector<double>> compMap = fReadout.Map(equip);
    for (auto const& val : compMap) {
      uint32_t address                       = val.first;
      par.fElinkParams[address].fToTshift    = val.second;
      par.fElinkParams[address].fTimeOffset  = SystemTimeOffset;
    }
    auto algo                      = std::make_unique<UnpackMS>(par);
    fAlgos[{equip, SystemVersion}] = std::move(algo);

    L_(info) << "--- Configured equipment " << equip << " with " << fReadout.GetNumElinks(equip) << " elinks";
  }
  L_(info) << "--- Configured " << fAlgos.size() << " unpacker algorithms for RICH.";
}

Unpack::Result_t Unpack::operator()(const fles::Timeslice& ts) const { return DoUnpack(Subsystem::RICH, ts); }
