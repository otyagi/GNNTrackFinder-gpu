/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Calibrate.h
/// \brief  Calibratior for the BMON digis
/// \since  04.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmBmonDigi.h"
#include "PartitionedVector.h"
#include "bmon/CalibrateSetup.h"
#include "tof/Calibrate.h"  // for the monitor data

#include <gsl/span>
#include <optional>
#include <sstream>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::bmon
{
  // NOTE: reusing TOF monitor
  using CalibrateMonitorData = tof::CalibrateMonitorData;

  /// \class Calibrate
  /// \brief Algorithm to calibrate BMon digis
  class Calibrate {
   public:
    using resultType = std::tuple<std::vector<CbmBmonDigi>, CalibrateMonitorData>;

    /// \brief Constructor
    /// \param params  Calibration parameters
    explicit Calibrate(CalibrateSetup params);

    /// \brief Calibrates a portion of digis
    /// \param digiIn  A portion of digis to calibrate
    resultType operator()(gsl::span<const CbmBmonDigi> digiIn);

   private:
    /// \brief Returns an index of the diamond by the address
    /// \param address  A hardware address of the digi
    size_t GetDiamondIndex(uint32_t address) const
    {
      return ((fSetup.selectionMask & address) >> fSelectionBitsOffset);
    }

    CalibrateSetup fSetup;                 ///< Parameters of calibrator
    std::vector<size_t> fChannelOffset;    ///< Channel offset: offset for the channel index of each diamond
    std::vector<double> fChannelDeadTime;  ///< Dead time, stored for a channel
    uint32_t fSelectionBitsOffset;         ///< Number of bits to ther right from the first bit in the selection mask
  };
}  // namespace cbm::algo::bmon
