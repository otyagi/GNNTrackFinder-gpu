/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderConfig.h
/// \date   29.01.2025
/// \brief  Configuration structure for V0 selector in mCBM
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "yaml/Yaml.h"

#include <string>
#include <vector>

namespace cbm::algo::kfp
{
  /// \struct CutsKfp
  /// \brief  Cuts for the KFParticleFinder
  struct CutsKfp {
    float minDecayLength;  ///< Minimal decay length of particles [cm]
    float minDecayLDL;     ///< Minimal value of decay length to decay length error ratio
    float maxChi2NdfPrim;  ///< Maximal chi2/NDF for primary particles (coming from the PV)
    float maxChi2NdfGeo;   ///< Maximal chi2/NDF for V0 candidates

    CBM_YAML_PROPERTIES(
      yaml::Property(&CutsKfp::minDecayLength, "min_decay_length", "Minimal decay length of particles [cm]"),
      yaml::Property(&CutsKfp::minDecayLDL, "min_decay_ldl", "Minimal value of decay length to decay length error ratio"),
      yaml::Property(&CutsKfp::maxChi2NdfPrim, "max_chi2_ndf_prim", "Maximal chi2/NDF for primary particles"),
      yaml::Property(&CutsKfp::maxChi2NdfGeo, "max_chi2_ndf_geo", "Maximal chi2/NDF for V0 candidates"));

    /// \brief String representation of the structure
    std::string ToString() const;
  };

  /// \struct ParticlePid
  /// \brief  PID and pre-selection cuts for a given particle
  struct ParticlePid {
    int pdg;             ///< PDG code for particle
    double minDca;       ///< Minimal DCA to PV [cm]
    double minVelocity;  ///< Minimal velocity [cm/ns]
    double maxVelocity;  ///< Maximal velocity [cm/ns]

    CBM_YAML_PROPERTIES(
      yaml::Property(&ParticlePid::pdg, "pdg", "PDG code of the particle"),
      yaml::Property(&ParticlePid::minDca, "min_dca", "Minimal DCA to PV [cm]"),
      yaml::Property(&ParticlePid::minVelocity, "min_velocity", "Minimal velocity [cm/ns]"),
      yaml::Property(&ParticlePid::maxVelocity, "max_velocity", "Maximal velocity [cm/ns]"));

    /// \brief String representation of the structure
    std::string ToString() const;
  };

  /// \struct Cuts;
  struct Cuts {
    CutsKfp kfp;                         ///< Specific cuts for the KFParticleFinder
    std::vector<ParticlePid> particles;  ///< Daughter PID cuts and other properties

    CBM_YAML_PROPERTIES(
      yaml::Property(&Cuts::kfp, "kfp", "Specific cuts for the KFParticleFinder"),
      yaml::Property(&Cuts::particles,  "particles", "Particle identification cuts and properties"));

    /// \brief String representation of the structure
    std::string ToString() const;
  };

  /// \struct LambdaFinderConfig
  /// \brief  Configuration for the V0 finder
  struct V0FinderConfig {
    Cuts cuts;                     ///< Different selection cuts
    uint32_t bmonAddress;  ///< Address of BMON diamond (if multiple alternative are present, only one must be selected)
    double tZeroOffset;            ///< Offset for T0 [ns]
    double qpAssignedUncertainty;  ///< Assigned relative uncertainty for q/p estimation
    int primaryAssignedPdg;        ///< Assigned PDG hypothesis for primary particles
    int reconstructPdg;            ///< PDG of the particle, the decay of which is to be reconstructed

    CBM_YAML_PROPERTIES(
      yaml::Property(&V0FinderConfig::cuts, "cuts", "Different selection cuts"),
      yaml::Property(&V0FinderConfig::bmonAddress, "bmon_address", "Address of reference BMON diamond"),
      yaml::Property(&V0FinderConfig::tZeroOffset, "t0_offset", "The t0 offset [ns]"),
      yaml::Property(&V0FinderConfig::qpAssignedUncertainty, "qa_uncertainty", "Assigned relative uncertainty for q/p"),
      yaml::Property(&V0FinderConfig::primaryAssignedPdg, "primary_pdg", "Assigned PDG code for primary tracks"),
      yaml::Property(&V0FinderConfig::reconstructPdg, "reconstruct_pdg", "PDG code of the particle to be reconstructed"));

    /// \brief String representation of the contents
    std::string ToString() const;
  };
}  // namespace cbm::algo::kfp
