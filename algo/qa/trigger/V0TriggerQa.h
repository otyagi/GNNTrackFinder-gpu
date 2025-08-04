/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   V0TriggerQa.h
/// \brief  A V0-trigger QA
/// \since  06.03.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "qa/QaTaskHeader.h"

namespace cbm::algo::qa
{
  class H1D;
}  // namespace cbm::algo::qa


namespace cbm::algo::evbuild
{
  /// \class V0TriggerQa
  /// \brief A QA module for the V0-trigger
  class V0TriggerQa : public qa::TaskHeader {
   public:
    friend class V0Trigger;  // for access to histograms

    /// \brief Constructor
    /// \param pManager  Pointer to the QA manager
    /// \param name      Name of the QA (directory)
    V0TriggerQa(const std::unique_ptr<qa::Manager>& pManager) : qa::TaskHeader(pManager, "V0Trigger") {}

    /// \brief Copy constructor
    V0TriggerQa(const V0TriggerQa&) = delete;

    /// \brief Move constructor
    V0TriggerQa(V0TriggerQa&&) = delete;

    /// \brief Destructor
    ~V0TriggerQa() = default;

    /// \brief Copy assignment operator
    V0TriggerQa& operator=(const V0TriggerQa&) = delete;

    /// \brief Move assignment operator
    V0TriggerQa& operator=(V0TriggerQa&&) = delete;

    /// \brief Initializes the task
    void Init();

   private:
    //* Constants
    static constexpr int kPairDeltaTB{100};       ///< Track pair time difference: n bins
    static constexpr double kPairDeltaTL{-50.};   ///< Track pair time difference: lower bound [ns]
    static constexpr double kPairDeltaTU{+50.};   ///< Track pair time difference: upper bound [ns]
    static constexpr int kPairZVertexB{120};      ///< Track pair z vertex: n bins
    static constexpr double kPairZVertexL{-60.};  ///< Track pair z vertex: lower bound [cm]
    static constexpr double kPairZVertexU{+60.};  ///< Track pair z vertex: upper bound [cm]
    static constexpr int kPairDcaB{300};          ///< Track pair DCA: n bins
    static constexpr double kPairDcaL{-0.5};      ///< Track pair DCA: lower bound [cm]
    static constexpr double kPairDcaU{+5.5};      ///< Track pair DCA: upper bound [cm]

    //* Histograms
    qa::H1D* fphPairDeltaT{nullptr};   ///< Track pair delta T
    qa::H1D* fphPairZVertex{nullptr};  ///< Track pair z-vertex
    qa::H1D* fphPairDca{nullptr};      ///< Track pair distance at closest approach
  };
}  // namespace cbm::algo::evbuild
