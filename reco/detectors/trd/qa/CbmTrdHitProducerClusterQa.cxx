/* Copyright (C) 2013-2016 Institut fuer Kernphysik, Westfaelische Wilhelms-Universitaet Muenster, Muenster
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Cyrano Bergmann [committer] */

// -----------------------------------------------------------------------
// -----                     CbmTrdHitProducerClusterQa                     -----
// -----               Created 19.03.13  by C. Bergmann               -----
// -----------------------------------------------------------------------


#include "CbmTrdHitProducerClusterQa.h"

#include "CbmMCTrack.h"
#include "CbmTrdHit.h"
#include "CbmTrdPoint.h"
#include "CbmTrdTrack.h"
#include "FairRootManager.h"
#include "TClonesArray.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMath.h"
#include "TVector3.h"

#include <iostream>
using std::cout;
using std::endl;

CbmTrdHitProducerClusterQa::CbmTrdHitProducerClusterQa() : CbmTrdHitProducerClusterQa("TrdHitProducerClusterQa", "") {}

CbmTrdHitProducerClusterQa::CbmTrdHitProducerClusterQa(const char* name, const char*) : FairTask(name) {}

CbmTrdHitProducerClusterQa::~CbmTrdHitProducerClusterQa() {}

InitStatus CbmTrdHitProducerClusterQa::Init()
{
  FairRootManager* rootMgr = FairRootManager::Instance();
  if (NULL == rootMgr) {
    cout << "-E- CbmTrdHitProducerClusterQa::Init : "
         << "ROOT manager is not instantiated !" << endl;
    return kFATAL;
  }
  PrepareHistograms();
  return kSUCCESS;
}
void CbmTrdHitProducerClusterQa::Exec(Option_t*) {}
void CbmTrdHitProducerClusterQa::Finish() { WriteHistograms(); }
void CbmTrdHitProducerClusterQa::PrepareHistograms() {}
void CbmTrdHitProducerClusterQa::WriteHistograms()
{
  gDirectory->mkdir("CbmTrdHitProducerClusterQa");
  gDirectory->cd("CbmTrdHitProducerClusterQa");
  gDirectory->cd("..");
}
ClassImp(CbmTrdHitProducerClusterQa);
