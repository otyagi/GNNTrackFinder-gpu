/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderConfig.cxx
/// \date   01.02.2025
/// \brief  Configuration structure for V0 selector in mCBM (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "KfpV0FinderConfig.h"

#include <iomanip>
#include <sstream>

using cbm::algo::kfp::Cuts;
using cbm::algo::kfp::CutsKfp;
using cbm::algo::kfp::ParticlePid;
using cbm::algo::kfp::V0FinderConfig;

// ---------------------------------------------------------------------------------------------------------------------
//
std::string CutsKfp::ToString() const
{
  std::stringstream msg;
  msg << "CUTS FOR KF PARTICLE FINDER:";
  msg << "\n\tmin. decay length (l):                           " << minDecayLength << " [cm]";
  msg << "\n\tmin. l/dl ratio:                                 " << minDecayLDL;
  msg << "\n\tmax. chi2/NDF for primary particles (chi2_prim): " << maxChi2NdfPrim;
  msg << "\n\tmax. chi2/NDF for V0 candidates (chi2_geo):      " << maxChi2NdfGeo;
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string ParticlePid::ToString() const
{
  std::stringstream msg;
  msg << "\tPDG: " << pdg;
  msg << "\n\t\tmin. DCA to PV: " << minDca << " [cm]";
  msg << "\n\t\tmin. velocity:  " << minVelocity << " [cm/ns]";
  msg << "\n\t\tmax. velocity:  " << maxVelocity << " [cm/ns]";
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string Cuts::ToString() const
{
  std::stringstream msg;
  msg << kfp.ToString() << '\n';
  msg << "PARTICLE SELECTION PROPERTIES:";
  for (const auto& particle : particles) {
    msg << '\n' << particle.ToString();
  }
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string V0FinderConfig::ToString() const
{
  using std::dec;
  using std::hex;
  using std::setfill;
  using std::setw;
  std::stringstream msg;
  msg << "\n-------------- KFP V0-finder Configuration: ------------------------------------------------";
  msg << "\nGOAL V0 (PDG): " << reconstructPdg;
  msg << '\n' << cuts.ToString();
  msg << "\nOTHER PARAMETERS:";
  msg << "\n\tt0-offset:                " << tZeroOffset << " [ns]";
  msg << "\n\tReference BMON:           0x" << hex << setw(8) << setfill('0') << bmonAddress << dec;
  msg << "\n\tq/p relative uncertainty: " << qpAssignedUncertainty;
  msg << "\n\tassigned PDG of primary:  " << primaryAssignedPdg;
  msg << "\n--------------------------------------------------------------------------------------------";
  return msg.str();
}
