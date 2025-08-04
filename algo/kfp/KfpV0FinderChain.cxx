/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0FinderChain.cxx
/// \date   01.02.2025
/// \brief  A chain for V0 finding (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "kfp/KfpV0FinderChain.h"

#include "global/ParFiles.h"
#include "kfp/KfpV0FinderConfig.h"
#include "log/AlgoFairloggerCompat.h"
#include "yaml/Yaml.h"

#include <sstream>

using cbm::algo::V0FinderChain;

// ---------------------------------------------------------------------------------------------------------------------
//
V0FinderChain::V0FinderChain(const std::unique_ptr<qa::Manager>& qaManager)
  : fpFinderQa(qaManager != nullptr ? std::make_unique<kfp::V0FinderQa>(qaManager, "V0Finder") : nullptr)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderChain::Finalize()
{
  GetMonitor();  // A hack to update the run monitor
  L_(info) << fMonitorRun.ToString();
}

// ---------------------------------------------------------------------------------------------------------------------
//
cbm::algo::kfp::V0FinderMonitorData_t V0FinderChain::GetMonitor()
{
  cbm::algo::kfp::V0FinderMonitorData_t monitorData = fMonitorTimeslice;
  fMonitorTimeslice.Reset();
  fMonitorRun.AddMonitorData(monitorData);
  return monitorData;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderChain::Init()
try {
  L_(info) << "kfp::V0FinderChain: initializing the V0-finder chain ...";

  // ----- Read configuration file
  ParFiles parFiles(Opts().RunId());
  auto config = yaml::ReadFromFile<kfp::V0FinderConfig>(Opts().ParamsDir() / parFiles.kfp.V0FinderConfig);
  L_(info) << config.ToString();

  //* Check the V0 type
  if (config.reconstructPdg != 3122) {  // At the moment only Lambda analysis is possible
    std::stringstream msg;
    msg << "kfp::V0FinderChain: at the moment only lambda finding is possible. Provided PDG: " << config.reconstructPdg;
    throw std::runtime_error(msg.str());
  }

  //* Define daughter particles
  auto& particles{config.cuts.particles};
  int iPion{-1};
  int iProton{-1};
  for (int iParticle = 0; iParticle < int(particles.size()); ++iParticle) {
    const auto& particle  = particles[iParticle];
    auto CheckOutParticle = [&](int pdg, int& iParticleToFind) {
      if (particle.pdg == pdg) {
        if (iParticleToFind == -1) {
          iParticleToFind = iParticle;
        }
        else {
          std::stringstream msg;
          msg << "kfp::V0FinderChain: entry for pdg= " << pdg << " is defined more then one time in the ";
          msg << "config.cuts.particles";
          throw std::runtime_error(msg.str());
        }
      }
    };
    CheckOutParticle(-211, iPion);
    CheckOutParticle(2212, iProton);
  }
  if (iProton == -1 || iPion == -1) {
    std::stringstream msg;
    msg << "kfp::V0FinderChain: config cuts/particles: either pion or proton settings are not found";
    throw std::runtime_error(msg.str());
  }
  const auto& pion{particles[iPion]};
  const auto& proton{particles[iProton]};

  // ----- Define a BMON diamond
  if (fBmonDefinedAddresses.empty()) {
    throw std::runtime_error("kfp::V0FinderChain: BMON available addresses were not set");
  }
  int iBmonPartitionSelect = -1;
  for (int iPart = 0; iPart < static_cast<int>(fBmonDefinedAddresses.size()); ++iPart) {
    if (config.bmonAddress == fBmonDefinedAddresses[iPart]) {
      iBmonPartitionSelect = iPart;
      break;
    }
  }
  if (iBmonPartitionSelect < 0) {
    std::stringstream msg;
    msg << "kfp::V0FinderChain: a reference BMON address ( " << std::hex << config.bmonAddress << std::dec
        << " ) differs from ones, provided by hitfinder. Please check your configuration";
    throw std::runtime_error(msg.str());
  }

  // ----- Set the V0-finder properties
  // TODO: In future, there are will be a several instances of the V0Finder, each for a particular thread
  {
    //* Set particle PID properties
    fFinder.SetBmonPartitionIndex(iBmonPartitionSelect);
    fFinder.SetMinPionDca(pion.minDca);
    fFinder.SetMinProtonDca(proton.minDca);
    fFinder.SetPionVelocityRange(pion.minVelocity, pion.maxVelocity);
    fFinder.SetProtonVelocityRange(proton.minVelocity, proton.maxVelocity);

    //* Set KFParticleFinder properties
    auto& kfpCuts{config.cuts.kfp};
    fFinder.SetLdLCut2D(kfpCuts.minDecayLDL);
    fFinder.SetLCut(kfpCuts.minDecayLength);
    fFinder.SetChi2Cut2D(kfpCuts.maxChi2NdfGeo);
    fFinder.SetChiPrimaryCut2D(kfpCuts.maxChi2NdfPrim);

    //* Set other properties
    fFinder.SetTzeroOffset(config.tZeroOffset);
    fFinder.SetQpAssignedUncertainty(config.qpAssignedUncertainty);
    fFinder.AddDecayToReconstructionList(config.reconstructPdg);
    fFinder.SetPrimaryAssignedPdg(config.primaryAssignedPdg);

    //* Init the V0 finder
    fFinder.Init();
  }

  if (fpFinderQa != nullptr) {
    fpFinderQa->Init();
  }

  L_(info) << "kfp::V0FinderChain: initializing the V0-finder chain ... done";
}
catch (const std::exception& err) {
  L_(info) << "kfp::V0FinderChain: initializing the V0-finder chain ... failed. Reason: " << err.what();
  throw std::runtime_error("initialization of V0-finder chain failed");
}

// ---------------------------------------------------------------------------------------------------------------------
//
V0FinderChain::EventOutput V0FinderChain::ProcessEvent(const RecoResults& recoEvent)
{
  EventOutput res = fFinder.Process(recoEvent);
  if (fpFinderQa != nullptr) {
    fpFinderQa->Exec(recoEvent, fFinder);
  }
  fMonitorTimeslice.AddMonitorData(fFinder.GetEventMonitor());
  return res;
}
