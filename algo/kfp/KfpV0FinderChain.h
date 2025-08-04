/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderChain.h
/// \date   01.02.2025
/// \brief  A chain for V0 finding
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmEventTriggers.h"
#include "PODVector.h"
#include "base/SubChain.h"
#include "global/RecoResults.h"
#include "kfp/KfpV0Finder.h"
#include "kfp/KfpV0FinderMonitor.h"
#include "kfp/KfpV0FinderQa.h"

namespace cbm::algo
{
  namespace qa
  {
    class Manager;
  }
}  // namespace cbm::algo

namespace cbm::algo
{
  /// \class V0FinderChain
  /// \brief A chain for the V0 finder
  class V0FinderChain : public SubChain {
   public:
    using EventOutput = CbmEventTriggers;

    /// \brief Default constructor
    V0FinderChain() = default;

    /// \brief Constructor from parameters
    /// \param pQaManager   A QA-manager
    V0FinderChain(const std::unique_ptr<qa::Manager>& qaManager);

    /// \brief Copy constructor
    V0FinderChain(const V0FinderChain&) = delete;

    /// \brief Move constructor
    V0FinderChain(V0FinderChain&&) = delete;

    /// \brief Destructor
    ~V0FinderChain() = default;

    /// \brief Copy assignment operator
    V0FinderChain& operator=(const V0FinderChain&) = delete;

    /// \brief Move assignment operator
    V0FinderChain& operator=(V0FinderChain&&) = delete;

    /// \brief Finalizes the instance (called in the end of the run)
    void Finalize();

    /// \brief Gets a monitor
    kfp::V0FinderMonitorData_t GetMonitor();

    /// \brief Sets BMON diamond addresses array
    /// \note  The addresses must be taken from bmon::Hitfind::GetDiamondAddresses()
    void SetBmonDefinedAddresses(const PODVector<uint32_t>& addresses) { fBmonDefinedAddresses = addresses; }

    /// \brief Initializes the instance (called in the beginning of the run)
    void Init();

    /// \brief  Processes an event, returns a collection of fired triggers
    EventOutput ProcessEvent(const RecoResults& recoEvent);

   private:
    kfp::V0Finder fFinder;                                 ///< Instance of the V0-finding algorithm
    kfp::V0FinderMonitor fMonitorRun;                      ///< Monitor per run
    kfp::V0FinderMonitorData_t fMonitorTimeslice;          ///< Monitor per timeslice
    std::unique_ptr<kfp::V0FinderQa> fpFinderQa{nullptr};  ///< QA module
    PODVector<uint32_t> fBmonDefinedAddresses;             ///< Available addresses of BMON
  };
}  // namespace cbm::algo
