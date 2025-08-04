/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   HistogramContainer.h
/// \date   29.02.2024
/// \brief  A histogram container for the histogram server (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "Histogram.h"  // for H1D, H2D

#include <boost/serialization/forward_list.hpp>

#include <forward_list>

namespace cbm::algo::qa
{
  /// \struct HistogramContainer
  /// \brief  Structure to keep the histograms for sending them on the histogram server
  struct HistogramContainer {
    std::forward_list<qa::H1D> fvH1    = {};  ///< List of 1D-histograms
    std::forward_list<qa::H2D> fvH2    = {};  ///< List of 2D-histograms
    std::forward_list<qa::Prof1D> fvP1 = {};  ///< List of 1D-profiles
    std::forward_list<qa::Prof2D> fvP2 = {};  ///< List of 2D-profiles
    uint64_t fTimesliceId              = 0;   ///< Index of the timeslice

    /// \brief Resets the histograms
    void Reset();

   private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fvH1;
      ar& fvH2;
      ar& fvP1;
      ar& fvP2;
      ar& fTimesliceId;
    }
  };
}  // namespace cbm::algo::qa
