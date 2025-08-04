/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */
#include "CommonUnpacker.h"

#include <iomanip>

using namespace cbm::algo;

detail::MSData::MSData(const fles::Timeslice& ts, fles::Subsystem subsystem, gsl::span<u16> legalEqIds)
{
  monitor.system = subsystem;

  for (uint64_t comp = 0; comp < ts.num_components(); comp++) {
    auto this_subsystem = static_cast<fles::Subsystem>(ts.descriptor(comp, 0).sys_id);

    if (this_subsystem != subsystem) {
      continue;
    }

    const u64 numMsInComp = ts.num_microslices(comp);
    const u16 componentId = ts.descriptor(comp, 0).eq_id;

    if (std::find(legalEqIds.begin(), legalEqIds.end(), componentId) == legalEqIds.end()) {
      L_(error) << "Invalid equipment id 0x" << std::hex << std::setw(4) << componentId << std::dec << " for subsystem "
                << ToString(subsystem);
      monitor.errInvalidEqId++;
      continue;
    }

    monitor.numComponents++;
    monitor.sizeBytesIn += ts.size_component(comp);
    monitor.numMs += numMsInComp;
    for (u64 mslice = 0; mslice < numMsInComp; mslice++) {
      msDesc.push_back(ts.descriptor(comp, mslice));
      msContent.push_back(ts.content(comp, mslice));
    }
  }
  L_(debug) << "Found " << monitor.numMs << " microslices for subsystem " << ToString(subsystem);
}
