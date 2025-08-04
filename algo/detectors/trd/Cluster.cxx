/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#include "Cluster.h"

#include "CbmTrdDigi.h"

namespace cbm::algo::trd
{
  //____________________________________________________________________
  Cluster::Cluster(const std::vector<int32_t>& indices, const std::vector<const CbmTrdDigi*>& digis, int32_t address,
                   uint16_t ncols, uint16_t nrows)
    : fDigiInd()
    , fDigis()
    , fAddress(address)
    , fNCols(ncols)
  {
    fDigiInd.assign(indices.begin(), indices.end());
    fDigis.assign(digis.begin(), digis.end());
    SetNRows(nrows);
  }

}  // namespace cbm::algo::trd
