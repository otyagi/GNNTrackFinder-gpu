/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "LandauTable.h"

#include "AlgoFairloggerCompat.h"

#include <fstream>

using namespace cbm::algo;

sts::LandauTable sts::LandauTable::FromFile(fs::path path)
{
  sts::LandauTable table;

  std::vector<f32> charge;
  std::vector<f32> prob;
  std::ifstream file(path.string());

  while (!file.eof()) {

    f32 q, p;
    file >> q >> p;
    charge.push_back(q);
    prob.push_back(p);

    L_(trace) << "Reading Landau table " << path << " q=" << q << " p=" << p;
  }

  // TODO: check if charge is monotonically increasing, also more than 2 entries

  table.stepSize = charge[1] - charge[0];
  table.values   = std::move(prob);

  return table;
}
