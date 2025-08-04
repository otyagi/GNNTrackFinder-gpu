/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingInterface.h
/// \date   19.04.2024
/// \brief  A TOF-parameter and geometry interface used for tracking input data initialization (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "SubChain.h"

namespace cbm::algo::tof
{
  class TrackingInterface : public SubChain {
   public:
    /// \brief Default constructor
    TrackingInterface() = default;

    /// \brief Copy constructor
    TrackingInterface(const TrackingInterface&) = delete;

    /// \brief Move constructor
    TrackingInterface(TrackingInterface&&) = delete;

    /// \brief Destructor
    ~TrackingInterface() = default;

    /// \brief Initializer function
    void Init();

    /// \brief   Returns tracking station index by the TOF address
    /// \param   address  Unique address of a TOF element
    /// \return  Local index of tracking station
    int GetTrackingStation(uint32_t address) const;

   private:
    std::vector<int> fvNofRpc;                          ///< Number of RPCs [NbSmt]
    std::vector<int> fvNofSm;                           ///< Number of super modules [NbSmt]
    std::vector<std::vector<int>> fvTrackingStationId;  ///< Index of tracking station [NbSmt][NbSm * NbRpc]
  };
}  // namespace cbm::algo::tof
