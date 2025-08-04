/* Copyright (C) 2010-2013 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

#include "qa/CbmLitCreateStandaloneData.h"

#include "CbmMuchPixelHit.h"
#include "CbmStsTrack.h"
#include "FairRootManager.h"
#include "FairTrackParam.h"
#include "TClonesArray.h"

#include <Logger.h>

#include <fstream>
#include <iostream>

CbmLitCreateStandaloneData::CbmLitCreateStandaloneData() : fEventNo(0) {}

CbmLitCreateStandaloneData::~CbmLitCreateStandaloneData() {}

InitStatus CbmLitCreateStandaloneData::Init()
{
  std::cout << "-I- CbmLitCreateStandaloneData::Init Creating MUCH layout" << std::endl;

  // Save MUCH layout to text file
  //   CbmLitEnvironment* env = CbmLitEnvironment::Instance();
  //   lit::parallel::LitDetectorLayoutMuonScal layout;
  //   env->GetMuchLayoutScal(layout);

  std::ofstream outfile("sa_geometry.txt");
  //outfile << layout;
  //   outfile << layout.ToString();
  // outfile << layout.GetNofStationGroups() << std::endl;
  // for (int isg = 0; isg < layout.GetNofStationGroups(); isg++) {
  //    LitStationGroupScal sg = layout.GetStationGroup(isg);
  //    outfile << isg << " " << sg.GetNofStations() << std::endl;
  //
  //    LitAbsorber ab = sg.absorber;
  //
  //    for (int ist = 0; ist < sg.GetNofStations(); ist++) {
  //
  //    }
  // }

  outfile.close();

  std::cout << "-I- CbmLitCreateStandaloneData::Init Finish creating MUCH layout" << std::endl;
  //

  //
  FairRootManager* ioman = FairRootManager::Instance();
  if (NULL == ioman) {
    LOG(fatal) << GetName() << "::Init: CbmRootManager is not instantiated";
  }

  fStsTracks = (TClonesArray*) ioman->GetObject("StsTrack");
  if (NULL == fStsTracks) {
    LOG(fatal) << GetName() << "::Init: No StsTrack array!";
  }

  fMuchPixelHits = (TClonesArray*) ioman->GetObject("MuchPixelHit");
  if (NULL == fMuchPixelHits) {
    LOG(fatal) << GetName() << "::Init: No MuchPixelHit array!";
  }

  fHitsFile.open("sa_hits.txt");
  fSeedsFile.open("sa_seeds.txt");
  //

  return kSUCCESS;
}

void CbmLitCreateStandaloneData::SetParContainers() {}

void CbmLitCreateStandaloneData::Exec(Option_t* /*opt*/)
{
  fSeedsFile << fEventNo << " " << fStsTracks->GetEntriesFast() << std::endl;
  for (Int_t iStsTrack = 0; iStsTrack < fStsTracks->GetEntriesFast(); iStsTrack++) {
    CbmStsTrack* stsTrack     = (CbmStsTrack*) fStsTracks->At(iStsTrack);
    const FairTrackParam* par = stsTrack->GetParamLast();
    fSeedsFile << par->GetX() << " " << par->GetY() << " " << par->GetZ() << " " << par->GetTx() << " " << par->GetTy()
               << " " << par->GetQp() << " ";
    double cov[15];
    par->CovMatrix(cov);
    for (unsigned int i = 0; i < 15; ++i) {
      fSeedsFile << cov[i] << " ";
    }
    fSeedsFile << std::endl;
  }

  fHitsFile << fEventNo << " " << fMuchPixelHits->GetEntriesFast() << std::endl;
  for (Int_t iMuchHit = 0; iMuchHit < fMuchPixelHits->GetEntriesFast(); iMuchHit++) {
    CbmMuchPixelHit* muchHit = (CbmMuchPixelHit*) fMuchPixelHits->At(iMuchHit);
    fHitsFile << muchHit->GetX() << " " << muchHit->GetY() << " " << muchHit->GetZ() << " " << muchHit->GetDx() << " "
              << muchHit->GetDy() << " " << muchHit->GetDxy() << " " << (muchHit->GetPlaneId()) - 1 << " "
              << muchHit->GetRefId() << std::endl;
  }

  std::cout << "Event: " << fEventNo++ << std::endl;
}

void CbmLitCreateStandaloneData::Finish()
{
  fHitsFile.close();
  fSeedsFile.close();
}

ClassImp(CbmLitCreateStandaloneData);
