/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "StorableRecoResults.h"

#include <OutputArchive.hpp>

namespace cbm::algo
{

  using RecoResultsOutputArchive =
    fles::OutputArchive<StorableRecoResults, StorableRecoResults, fles::ArchiveType::RecoResultsArchive>;

}  // namespace cbm::algo

// explicit instantiation of the template class because of long compile times
namespace fles
{
  extern template class OutputArchive<cbm::algo::StorableRecoResults, cbm::algo::StorableRecoResults,
                                      ArchiveType::RecoResultsArchive>;
}
