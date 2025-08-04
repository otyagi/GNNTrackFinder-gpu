/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderQa.cxx
/// \date   13.02.2025
/// \brief  A V0 finding algorithm QA (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "kfp/KfpV0FinderQa.h"

#include "global/RecoResults.h"
#include "kfp/KfpV0Finder.h"


using cbm::algo::kfp::V0FinderQa;

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderQa::Init()
{
  using qa::CanvasConfig;
  using qa::H1D;
  using qa::PadConfig;

  //* Histogram initialisation
  fvphMassLambdaCand =
    MakeObj<H1D>("kfp_mass_lambda", "Mass of #Lambda-candidates;m [GeV/c^{2}];Counts", kMassB, kMassL, kMassU);
  fvphMassAll    = MakeObj<H1D>("kfp_mass_all", "Mass of particles;m [GeV/c^{2}];Counts", 300, 0., 1.5);
  fvphDcaAll     = MakeObj<H1D>("kfp_dca_all", "DCA of tracks to origin;DCA [cm];Counts", kDcaB, kDcaL, kDcaU);
  fvphBetaAll    = MakeObj<H1D>("kfp_beta_all", "Speed of tracks;#beta;Counts", kBetaB, kBetaL, kBetaU);
  fvphBetaPion   = MakeObj<H1D>("kfp_beta_pion", "Speed of #pi-candidates;#beta;Counts", kBetaB, kBetaL, kBetaU);
  fvphBetaProton = MakeObj<H1D>("kfp_beta_proton", "Speed of proton-candidates;#beta;Counts", kBetaB, kBetaL, kBetaU);
  fvphMomAll     = MakeObj<H1D>("kfp_mom_all", "Momentum of tracks;p [GeV/c];Counts", kBetaB, kBetaL, kBetaU);
  fvphMomPion    = MakeObj<H1D>("kfp_mom_pion", "Momentum of #pi-candidates;p [GeV/c];Counts", kBetaB, kBetaL, kBetaU);
  fvphMomProton =
    MakeObj<H1D>("kfp_mom_proton", "Momentum of proton-candidates;p [GeV/c];Counts", kBetaB, kBetaL, kBetaU);

  //* Canvas initialisation
  auto canv = CanvasConfig("kfp_lambda", "Lambda-trigger summary QA", 3, 2);
  canv.AddPadConfig(PadConfig(fvphMassLambdaCand, "hist"));  // (0, 0)
  canv.AddPadConfig(PadConfig(fvphDcaAll, "hist"));          // (0, 1)
  canv.AddPadConfig(PadConfig(fvphBetaAll, "hist"));
  canv.AddPadConfig(PadConfig(fvphBetaPion, "hist"));
  canv.AddPadConfig(PadConfig(fvphBetaProton, "hist"));
  AddCanvasConfig(canv);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderQa::Exec(const RecoResults& recoEvent, const V0Finder& v0Finder)
{
  //* Fill track distributions
  const auto& tracks{recoEvent.tracks};
  if (v0Finder.GetT0s().size() == 1) {
    for (uint32_t iTrk = 0; iTrk < tracks.size(); ++iTrk) {
      const auto& particleInfo{v0Finder.GetParticleInfo()[iTrk]};
      const auto& trkParFst{(v0Finder.GetTrackAssignedParams()[iTrk]).first};
      bool bPdgDefined = (particleInfo.fPdg != V0Finder::kUndefPdg);
      // All particles with defined DCA (primaries + secondary pions and protons)
      fvphDcaAll->Fill(std::isnan(particleInfo.fDca) ? -999 : particleInfo.fDca);
      fvphBetaAll->Fill(particleInfo.fBeta);
      if (bPdgDefined) {
        if (particleInfo.fPdg == -211) {
          fvphBetaPion->Fill(particleInfo.fBeta);
          fvphMomPion->Fill(trkParFst.GetP());
        }
        else if (particleInfo.fPdg == 2212) {
          fvphBetaProton->Fill(particleInfo.fBeta);
          fvphMomProton->Fill(trkParFst.GetP());
        }
      }
    }
  }
  else {
    for (uint32_t iTrk = 0; iTrk < tracks.size(); ++iTrk) {
      const auto& particleInfo{v0Finder.GetParticleInfo()[iTrk]};
      bool bPdgDefined = (particleInfo.fPdg != V0Finder::kUndefPdg);
      fvphDcaAll->Fill(std::isnan(particleInfo.fDca) ? -999 : particleInfo.fDca);
      fvphBetaAll->Fill(-9999);
      if (bPdgDefined) {
        if (particleInfo.fPdg == -211) {
          fvphBetaPion->Fill(-9999);
          fvphMomPion->Fill(-9999);
        }
        else if (particleInfo.fPdg == 2212) {
          fvphBetaProton->Fill(-9999);
        }
      }
    }
  }

  //* Fill particle distributions
  const auto& particles = v0Finder.GetTopoReconstructor()->GetParticles();
  for (const auto& particle : particles) {
    fvphMassAll->Fill(particle.GetMass());
    if (particle.GetPDG() == 3122) {
      fvphMassLambdaCand->Fill(particle.GetMass());
    }
  }
}
