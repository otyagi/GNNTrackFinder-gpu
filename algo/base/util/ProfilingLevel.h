/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include "EnumDict.h"

namespace cbm::algo
{

  enum ProfilingLevel
  {
    ProfilingNone    = 0,  //< Disable profiling
    ProfilingSummary = 1,  //< Only print times aggregated over all timeslices
    ProfilingPerTS   = 2,  //< Print times for each timeslice
  };

}  // namespace cbm::algo

CBM_ENUM_DICT(cbm::algo::ProfilingLevel,
  {"None", cbm::algo::ProfilingNone},
  {"Summary", cbm::algo::ProfilingSummary},
  {"PerTS", cbm::algo::ProfilingPerTS},
);
