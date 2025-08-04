/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "RecoResultsInputArchive.h"

namespace fles
{
  template class InputArchive<cbm::algo::StorableRecoResults, cbm::algo::StorableRecoResults,
                              ArchiveType::RecoResultsArchive>;
}
