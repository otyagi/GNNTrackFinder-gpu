/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#include "Unpack.h"

#include "AlgoFairloggerCompat.h"

using namespace cbm::algo::trd;
using fles::Subsystem;

Unpack::Unpack(const ReadoutConfig& readout) : fReadout(readout)
{
  constexpr std::array<u8, 2> SystemVersion = {0x01, 0x10};

  // Create one algorithm per component for TRD and configure it with parameters
  auto equipIdsTrd = fReadout.GetEquipmentIds();
  for (auto& equip : equipIdsTrd) {
    trd::UnpackPar par{};
    const size_t numCrobs = fReadout.GetNumCrobs(equip);

    for (size_t crob = 0; crob < numCrobs; crob++) {
      trd::UnpackCrobPar crobPar;
      const size_t numElinks = fReadout.GetNumElinks(equip, crob);

      for (size_t elink = 0; elink < numElinks; elink++) {
        trd::UnpackElinkPar elinkPar;
        auto addresses        = fReadout.Map(equip, crob, elink);
        elinkPar.fAddress     = addresses.first;   // Asic address for this elink
        elinkPar.fChanAddress = addresses.second;  // Channel addresses for this elink
        elinkPar.fTimeOffset  = fReadout.GetSystemTimeOffset() + fReadout.GetElinkTimeOffset(crob, elink);
        crobPar.fElinkParams.push_back(elinkPar);
      }
      par.fCrobParams.push_back(crobPar);
    }
    auto algo_01                      = std::make_unique<UnpackMS<SystemVersion[0]>>(std::move(par));
    auto algo_10                      = std::make_unique<UnpackMS<SystemVersion[1]>>(std::move(par));
    fAlgos[{equip, SystemVersion[0]}] = std::move(algo_01);
    fAlgos[{equip, SystemVersion[1]}] = std::move(algo_10);

    L_(debug) << "--- Configured equipment " << equip << " with " << numCrobs << " crobs";
  }
  L_(info) << "--- Configured " << fAlgos.size() << " unpacker algorithms for TRD.";
}

Unpack::Result_t Unpack::operator()(const fles::Timeslice& ts) const { return DoUnpack(Subsystem::TRD, ts); }
