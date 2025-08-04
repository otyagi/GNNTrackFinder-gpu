/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_DATA_STS_LANDAUTABLE_H
#define CBM_ALGO_DATA_STS_LANDAUTABLE_H

#include "Definitions.h"
#include "compat/Filesystem.h"

#include <vector>

namespace cbm::algo::sts
{

  struct LandauTable {

    static LandauTable FromFile(fs::path path);

    std::vector<f32> values;
    f32 stepSize = 0;
  };

}  // namespace cbm::algo::sts

#endif  // CBM_ALGO_DATA_STS_LANDAUTABLE_H
