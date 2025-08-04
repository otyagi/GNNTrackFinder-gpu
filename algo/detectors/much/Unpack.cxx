/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#include "Unpack.h"

#include "AlgoFairloggerCompat.h"

using namespace cbm::algo::much;
using fles::Subsystem;

Unpack::Unpack(const ReadoutConfig& readout) : fReadout(readout)
{
  constexpr i64 SystemTimeOffset = -980;
  constexpr u8 SystemVersion     = 0x20;

  // Create one algorithm per component for MUCH and configure it with parameters
  auto equipIdsMuch = fReadout.GetEquipmentIds();
  for (auto& equip : equipIdsMuch) {
    much::UnpackPar par{};
    const size_t numElinks = fReadout.GetNumElinks(equip);
    for (size_t elink = 0; elink < numElinks; elink++) {
      much::UnpackElinkPar elinkPar;
      elinkPar.fAddress    = fReadout.Map(equip, elink);  // Vector of MUCH addresses for this elink
      elinkPar.fTimeOffset = SystemTimeOffset;
      elinkPar.fChanMask   = fReadout.MaskMap(equip, elink);
      par.fElinkParams.push_back(elinkPar);
    }
    auto algo                      = std::make_unique<UnpackMS>(par);
    fAlgos[{equip, SystemVersion}] = std::move(algo);
    L_(debug) << "--- Configured equipment " << equip << " with " << numElinks << " elinks";
  }
  L_(info) << "--- Configured " << fAlgos.size() << " unpacker algorithms for MUCH.";
}

Unpack::Result_t Unpack::operator()(const fles::Timeslice& ts) const { return DoUnpack(Subsystem::MUCH, ts); }
