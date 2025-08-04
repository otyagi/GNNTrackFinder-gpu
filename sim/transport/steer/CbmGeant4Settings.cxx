/* Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmGeant4Settings.h"

#include <FairRunSim.h>
#include <Logger.h>

#include <TGeant4.h>
#include <TPythia6Decayer.h>
#include <TVirtualMC.h>

#include <G4NuclearLevelData.hh>

void CbmGeant4Settings::Init(TVirtualMC* vmc)
{

  CbmVMCSettings::Init(vmc);

  TGeant4* vmcg4 = dynamic_cast<TGeant4*>(vmc);

  assert(vmcg4);

  // TODO: These settings were taken over from g4Config.C. To be documented.

  // --- Set external decayer (Pythia) if required
  if (FairRunSim::Instance()->IsExtDecayer()) {
    TVirtualMCDecayer* decayer = TPythia6Decayer::Instance();
    vmcg4->SetExternalDecayer(decayer);
    LOG(info) << ": Using Phythia6 decayer";
  }

  // --- Random seed and maximum number of steps
  size_t buf_size = 100;
  Text_t buffer[buf_size];
  // Get the infomation about the seed value defined by SetSeed from the base class.
  // Since ROOT 6.24 the derrived classes return a differnt value.
  fRandomSeed = gRandom->TRandom::GetSeed();
  LOG(info) << "Set Geant4 random seed to " << fRandomSeed;
  int result_length = snprintf(buffer, buf_size - 1, "/random/setSeeds %i  %i ", fRandomSeed, fRandomSeed);
  if (!(result_length > 0 && result_length < static_cast<int>(buf_size))) {
    LOG(fatal) << "Buffer overrun. Random seed for Geant4 would be improper.";
  }
  vmcg4->ProcessGeantCommand(buffer);

  vmcg4->SetMaxNStep(fMaxNumSteps);

  for (std::string command : fG4Commands) {
    LOG(info) << "Passing the following command to Geant4: " << command;
    vmcg4->ProcessGeantCommand(command.c_str());
  }

  LOG(warning) << "Fix for Geant4 bug applied (see "
                  "https://bugzilla-geant4.kek.jp/show_bug.cgi?id=2226)";
  G4NuclearLevelData::GetInstance()->GetParameters()->SetMaxLifeTime(1 * CLHEP::microsecond);
}

ClassImp(CbmGeant4Settings);
