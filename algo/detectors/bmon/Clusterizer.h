/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Clusterizer.h
/// \brief  A clusterizer algorithm for BMON
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "base/PODVector.h"
#include "bmon/ClusterizerPars.h"
#include "bmon/Hit.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

class CbmBmonDigi;

namespace cbm::algo::bmon
{
  /// \class Clusterizer
  /// \brief A clusterizer algorithm for a BMON
  ///
  /// The algorithm is executed on a single hardware module
  class Clusterizer {
   public:
    using Input_t  = std::vector<std::pair<CbmBmonDigi, int32_t>>;     ///< Input type
    using Output_t = std::pair<std::vector<Hit>, PODVector<int32_t>>;  ///< Output type

    /// \brief Constructor
    /// \param params RPC parameters
    explicit Clusterizer(ClusterizerPars params) : fParams(params) {}

    /// \brief Hit building function
    Output_t operator()(const Input_t& digisInput);

    /// \brief  Applies selection on a digis
    /// \return true   Digi is selected
    /// \return false  Digi is cut out
    bool SelectDigi(const CbmBmonDigi& digi) const;

   private:
    /// \brief Creates a hit from a single digi
    Hit CreateHit(const CbmBmonDigi& digi) const;

    /// \brief Creates a hit from two digis
    Hit CreateHit(const CbmBmonDigi& digiL, const CbmBmonDigi& digiR) const;


    ClusterizerPars fParams;  ///< parameters container
  };
}  // namespace cbm::algo::bmon
