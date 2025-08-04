/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Hitfind.h
/// \brief  BMON hitfinder steering class
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmBmonDigi.h"
#include "PODVector.h"
#include "PartitionedVector.h"
#include "bmon/Clusterizer.h"
#include "bmon/HitfindSetup.h"
#include "tof/Hitfind.h"  // for tof::HitfindMonitorData

#include <gsl/span>
#include <optional>
#include <sstream>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::bmon
{
  /// \brief TOF hit-finder monitor, re-used for BMON
  using HitfindMonitorData = tof::HitfindMonitorData;

  /// \class Hitfind
  /// \brief Hit-finder steering class for BMON
  class Hitfind {
   public:
    /// \brief Output format
    using Output_t = std::tuple<PartitionedVector<Hit>, HitfindMonitorData, PODVector<int32_t>>;

    /// \brief Constructor
    /// \param  setup     Setup configuration
    /// \param  nThreads  Number of threads (for event-based mode)
    explicit Hitfind(HitfindSetup setup, uint32_t nThreads = 1);

    /// \brief Algorithm execution operator
    /// \param digiIn   A portion of digis in TS/event
    /// \param iThread  Index of thread
    Output_t operator()(gsl::span<CbmBmonDigi> digisIn, uint32_t iThread = 0);

    /// \brief Returns an index of the diamond by the address
    /// \param address  A hardware address of the digi
    size_t GetDiamondIndex(uint32_t address) const { return ((fSelectionBitmask & address) >> fSelectionBitsOffset); }

    /// \brief  Gets diamond addresses vector
    const PODVector<uint32_t>& GetDiamondAddresses() const { return fDiamondAddress; }

   private:                         // members
    uint32_t fNofThreads;           ///< Number of threads
    uint32_t fSelectionBitsOffset;  ///< Number of bits to ther right from the first bit in the selection mask
    uint32_t fSelectionBitmask;     ///< Selection bitmask

    std::vector<std::vector<Clusterizer>> fAlgo;  ///< Clusterizer algorithms [thread][diamond]
    PODVector<uint32_t> fDiamondAddress;          ///< Diamond address
  };
}  // namespace cbm::algo::bmon
