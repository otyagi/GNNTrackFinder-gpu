/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfParticlePDG.h
/// @brief  Track particle representation in the KF-core
/// @since  27.08.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "KfDefs.h"

namespace cbm::algo::kf
{
  /// \class  ParticlePDG
  /// \brief  Properties of a tracked particle
  class alignas(VcMemAlign) ParticlePDG {
    /// \brief Constructor from parameters
    /// \param pid      PID code (note: abs value is only accounted)
    /// \param mass     Particle mass [GeV/c2]
    /// \param bremsstr If bremsstrahlung should be accounted (i.e. if particle an electron)
    constexpr ParticlePDG(int pid, double mass, bool bremsstr) : fMass(mass), fPid(pid), fbBremsstrahlung(bremsstr) {}

    /// \brief Default constructor
    ParticlePDG() = delete;

    /// \brief Gets particle mass [GeV/c2]
    constexpr double GetMass() const { return fMass; }

    /// \brief Gets squared particle mass [(GeV/c2)2]
    constexpr double GetMassSq() const { return fMass * fMass; }

    /// \brief Gets particle PID (NOTE: absolute value is used)
    constexpr int GetPid() const { return fPid; }

    /// \brief Gets bremsstrahlung flag
    constexpr bool IfBremsstrahlung() const { return fbBremsstrahlung; }

   private:
    double fMass{};                ///< Particle mass [GeV/c2]
    int fPid{};                    ///< Particle encoding according to PDG
    bool fbBremsstrahlung{false};  ///< Is Bremsstrahlung should be taken into account in E-loss estimation
  };

  // Predefined particle definitions
  namespace particle
  {
    constexpr auto Electron = ParticlePDG(11, defs::ElectronMass<double>, true);
    constexpr auto Muon     = ParticlePDG(13, defs::MuonMass<double>, false);
    constexpr auto Pion     = ParticlePDG(211, defs::PionMass<double>, false);
    constexpr auto Kaon     = ParticlePDG(321, defs::KaonMass<double>, false);
    constexpr auto Proton   = ParticlePDG(2212, defs::ProtonMass<double>, false);
  }  // namespace particle

}  // namespace cbm::algo::kf
