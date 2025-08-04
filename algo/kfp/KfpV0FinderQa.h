/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderQa.h
/// \date   13.02.2025
/// \brief  A V0 finding algorithm QA
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "qa/QaTaskHeader.h"

namespace cbm::algo
{
  class RecoResults;

  namespace qa
  {
    class H1D;
    class H2D;
  }  // namespace qa

  namespace kfp
  {
    class V0Finder;
  }
}  // namespace cbm::algo

namespace cbm::algo::kfp
{
  /// \class V0FinderQa
  /// \brief A QA-task for the V0-finding algorithm
  class V0FinderQa : public qa::TaskHeader {
   public:
    /// \brief Constructor
    /// \param pManager  Pointer to the QA manager
    /// \param name      Name of the QA
    V0FinderQa(const std::unique_ptr<qa::Manager>& pManager, std::string_view name) : qa::TaskHeader(pManager, name) {}

    /// \brief Copy constructor
    V0FinderQa(const V0FinderQa&) = delete;

    /// \brief Move constructor
    V0FinderQa(V0FinderQa&&) = delete;

    /// \brief Copy assignment operator
    V0FinderQa& operator=(const V0FinderQa&) = delete;

    /// \brief Move assignment operator
    V0FinderQa& operator=(V0FinderQa&&) = delete;

    /// \brief Executes the task, fills the histograms
    /// \param recoEvent  A reconstructed event instance
    /// \param v0Finder   A V0-finder instance
    void Exec(const RecoResults& recoEvent, const V0Finder& v0Finder);

    /// \brief Initialized the task
    void Init();

   private:
    //* Constants
    static constexpr int kMassB    = 50;    ///< Lambda-candidate mass: number of bins
    static constexpr double kMassL = 1.08;  ///< Lambda-candidate mass: lower bound [GeV/c2]
    static constexpr double kMassU = 1.18;  ///< Lambda-candidate mass: upper bound [GeV/c2]
    static constexpr int kDcaB     = 240;   ///< DCA to origin: number of bins
    static constexpr double kDcaL  = 0.;    ///< DCA to origin: lower bound [cm]
    static constexpr double kDcaU  = 12.;   ///< DCA to origin: upper bound [cm]
    static constexpr int kBetaB    = 240;   ///< Speed of particle: number of bins
    static constexpr double kBetaL = 0.;    ///< Speed of particle: lower bound [c]
    static constexpr double kBetaU = 1.2;   ///< Speed of particle: upper bound [c]

    //* Histograms
    qa::H1D* fvphMassLambdaCand{nullptr};  ///< Mass of Lambda-candidates
    qa::H1D* fvphMassAll{nullptr};         ///< Mass of all particles in the topology
    qa::H1D* fvphDcaAll{nullptr};          ///< DCA of particles to origin
    qa::H1D* fvphBetaAll{nullptr};         ///< Speed of all particles
    qa::H1D* fvphBetaPion{nullptr};        ///< Speed of pion-candidates
    qa::H1D* fvphBetaProton{nullptr};      ///< Speed of proton-candidates
    qa::H1D* fvphMomAll{nullptr};          ///< Speed of all particles
    qa::H1D* fvphMomPion{nullptr};         ///< Speed of pion-candidates
    qa::H1D* fvphMomProton{nullptr};       ///< Speed of proton-candidates
  };
}  // namespace cbm::algo::kfp
