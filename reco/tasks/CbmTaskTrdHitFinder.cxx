/* Copyright (C) 2010-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Pascal Raisig, Alexandru Bercuci */

#include "CbmTaskTrdHitFinder.h"

#include "CbmDigiManager.h"

#include <FairRootManager.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <Logger.h>

// C++ Classes and includes
#include "compat/Filesystem.h"
#include "yaml/Yaml.h"

#include <TStopwatch.h>
#include <TVector3.h>

#include <cmath>
#include <iomanip>
#include <iostream>

using std::fixed;
using std::left;
using std::right;
using std::setprecision;
using std::setw;
using std::stringstream;

//_____________________________________________________________________
CbmTaskTrdHitFinder::CbmTaskTrdHitFinder() : FairTask("TrdClusterFinder", 1) {}

// ---- Destructor ----------------------------------------------------
CbmTaskTrdHitFinder::~CbmTaskTrdHitFinder()
{
  if (fClusters) {
    fClusters->clear();
    delete fClusters;
  }
  if (fHits) {
    fHits->clear();
    delete fHits;
  }
}

//_____________________________________________________________________
void CbmTaskTrdHitFinder::SetParContainers()
{
  // FairRuntimeDb* rtdb = FairRunAna::Instance()->GetRuntimeDb();
}

//_____________________________________________________________________
InitStatus CbmTaskTrdHitFinder::Init()
{
  CbmDigiManager::Instance()->Init();
  if (!CbmDigiManager::Instance()->IsPresent(ECbmModuleId::kTrd)) LOG(fatal) << GetName() << "Missing Trd digi branch.";

  fClusters              = new std::vector<CbmTrdCluster>();
  FairRootManager* ioman = FairRootManager::Instance();
  ioman->RegisterAny("TrdCluster", fClusters, true);

  fHits = new std::vector<CbmTrdHit>();
  ioman->RegisterAny("TrdHit", fHits, true);

  // Read hitfinder parameters and initialize algo
  fAlgo = std::make_unique<cbm::algo::trd::Hitfind>(
    cbm::algo::yaml::ReadFromFile<cbm::algo::trd::HitfindSetup>("TrdHitfinderPar.yaml"),
    cbm::algo::yaml::ReadFromFile<cbm::algo::trd::Hitfind2DSetup>("TrdHitfinder2DPar.yaml"));

  return kSUCCESS;
}

//_____________________________________________________________________
void CbmTaskTrdHitFinder::Exec(Option_t* /*option*/)
{
  fClusters->clear();
  fHits->clear();

  TStopwatch timerTs;
  timerTs.Start();

  std::vector<CbmTrdDigi> digiVec;

  for (int32_t iDigi = 0; iDigi < CbmDigiManager::Instance()->GetNofDigis(ECbmModuleId::kTrd); iDigi++) {
    const CbmTrdDigi* tDigi = CbmDigiManager::Instance()->Get<CbmTrdDigi>(iDigi);
    digiVec.push_back(CbmTrdDigi(*tDigi));
  }

  auto [hits, monitor] = (*fAlgo)(digiVec);
  AddHits(hits.Data());

  timerTs.Stop();
  fProcessTime += timerTs.RealTime();

  stringstream logOut;
  logOut << setw(20) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timerTs.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNrTs;
  LOG(info) << logOut.str();
}


//_____________________________________________________________________
void CbmTaskTrdHitFinder::AddHits(gsl::span<cbm::algo::trd::Hit> hits)
{
  for (auto& hit : hits) {
    TVector3 pos(hit.X(), hit.Y(), hit.Z());
    TVector3 dpos(hit.Dx(), hit.Dy(), hit.Dz());

    CbmTrdHit& hitSave =
      fHits->emplace_back(hit.Address(), pos, dpos, hit.Dxy(), hit.GetRefId(), hit.GetELoss(), hit.Time(),
                          hit.TimeError());  // TODO implement copy constructor
    hitSave.SetOverFlow(hit.HasOverFlow());
    hitSave.SetRowCross(hit.IsRowCross());
    hitSave.SetClassType(hit.GetClassType());
    hitSave.SetMaxType(hit.GetMaxType());
  }
}


//_____________________________________________________________________
void CbmTaskTrdHitFinder::Finish()
{
  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Finish run";
  LOG(info) << GetName() << ": Run summary ";
  LOG(info) << GetName() << ": Processing time         : " << std::fixed << std::setprecision(3) << fProcessTime;
  LOG(info) << GetName() << ": Nr of events            : " << fNrEvents;
  LOG(info) << GetName() << ": Nr of input digis       : " << fNrDigis;
  LOG(info) << GetName() << ": Nr of produced clusters : " << fNrClusters;
  LOG(info) << GetName() << ": Nr of clusters / event  : " << std::fixed << std::setprecision(2)
            << (fNrEvents > 0 ? fNrClusters / (Double_t) fNrEvents : 0);
  LOG(info) << GetName() << ": Nr of digis / cluster   : " << std::fixed << std::setprecision(2)
            << (fNrClusters > 0 ? fNrDigis / (Double_t) fNrClusters : 0);
  LOG(info) << "=====================================";

  LOG(info) << GetName() << ": Nr of events         : " << fNrEvents;
  LOG(info) << GetName() << ": Nr of input clusters : " << fNrClusters;
  LOG(info) << GetName() << ": Nr of produced hits  : " << fNrHits;
  LOG(info) << GetName() << ": Nr of hits / event   : " << std::fixed << std::setprecision(2)
            << (fNrEvents > 0 ? fNrHits / (Double_t) fNrEvents : 0);
  LOG(info) << GetName() << ": Nr of hits / clusters: " << std::fixed << std::setprecision(2)
            << (fNrClusters > 0 ? fNrHits / (Double_t) fNrClusters : 0);
  LOG(info) << "=====================================";
  std::cout << std::endl;
}

ClassImp(CbmTaskTrdHitFinder)
