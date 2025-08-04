/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   HistogramContainer.cxx
/// \date   29.02.2024
/// \brief  A histogram container for the histogram server (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "HistogramContainer.h"

#include <algorithm>

using cbm::algo::qa::HistogramContainer;

// ---------------------------------------------------------------------------------------------------------------------
//
void HistogramContainer::Reset()
{
  constexpr auto ResetHistograms = [&](auto& h) { h.Reset(); };
  std::for_each(fvH1.begin(), fvH1.end(), ResetHistograms);
  std::for_each(fvH2.begin(), fvH2.end(), ResetHistograms);
  std::for_each(fvP1.begin(), fvP1.end(), ResetHistograms);
  std::for_each(fvP2.begin(), fvP2.end(), ResetHistograms);
}
