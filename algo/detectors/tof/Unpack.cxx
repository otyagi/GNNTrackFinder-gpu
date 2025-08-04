/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#include "Unpack.h"

#include "AlgoFairloggerCompat.h"

using namespace cbm::algo::tof;
using fles::Subsystem;

Unpack::Unpack(const ReadoutConfig& readout) : fReadout(readout)
{
  constexpr u8 SystemVersion = 0x00;
  i32 SysTimeOffset          = fReadout.GetSystemTimeOffset();

  auto equipIdsTof = fReadout.GetEquipmentIds();
  for (auto& equip : equipIdsTof) {
    tof::UnpackPar par{};
    const size_t numElinks = fReadout.GetNumElinks(equip);
    for (size_t elink = 0; elink < numElinks; elink++) {
      tof::UnpackElinkPar elinkPar;
      elinkPar.fChannelUId = fReadout.Map(equip, elink);  // Vector of TOF addresses for this elink
      elinkPar.fTimeOffset = SysTimeOffset + fReadout.GetElinkTimeOffset(equip, elink);  // Time offset for this elink
      par.fElinkParams.push_back(elinkPar);
    }
    auto algo                      = std::make_unique<UnpackMS>(std::move(par));
    fAlgos[{equip, SystemVersion}] = std::move(algo);
    L_(debug) << "--- Configured equipment " << equip << " with " << numElinks << " elinks";
  }

  L_(info) << "--- Configured " << fAlgos.size() << " unpacker algorithms for TOF.";
}

Unpack::Result_t Unpack::operator()(const fles::Timeslice& ts) const { return DoUnpack(Subsystem::TOF, ts); }
