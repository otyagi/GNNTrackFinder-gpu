/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TaskProperties.h
/// \date   09.02.2025
/// \brief  QA-task properties structure
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "Histogram.h"  // for H1D, H2D

#include <forward_list>
#include <string>
#include <utility>

namespace cbm::algo::qa
{
  /// \struct HistogramContainer
  /// \brief  Structure to keep the histograms for sending them on the histogram server
  struct TaskProperties {
    template<class H>
    using IteratorPair_t = std::pair<typename std::forward_list<H>::iterator, typename std::forward_list<H>::iterator>;

    std::string fsName;                   ///< Name of the task
    IteratorPair_t<qa::H1D> fRangeH1;     ///< A pair (begin, end) for 1D-histograms in the task
    IteratorPair_t<qa::H2D> fRangeH2;     ///< A pair (begin, end) for 2D-histograms in the task
    IteratorPair_t<qa::Prof1D> fRangeP1;  ///< A pair (begin, end) for 1D-profiles in the task
    IteratorPair_t<qa::Prof2D> fRangeP2;  ///< A pair (begin, end) for 2D-profiles in the task
  };
}  // namespace cbm::algo::qa
