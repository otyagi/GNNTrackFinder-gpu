/* Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmTrdModuleRec.h"

#include <TClonesArray.h>

//_______________________________________________________________________________
CbmTrdModuleRec::CbmTrdModuleRec()
  : CbmTrdModuleAbstract()
  , fCalled(0)
  , fClusters(new TClonesArray("CbmTrdCluster", 100))
  , fHits(new TClonesArray("CbmTrdHit", 100))
{
}

//_______________________________________________________________________________
CbmTrdModuleRec::CbmTrdModuleRec(Int_t mod, Int_t ly, Int_t rot)
  : CbmTrdModuleAbstract(mod, ly, rot)
  , fCalled(0)
  , fClusters(new TClonesArray("CbmTrdCluster", 100))
  , fHits(new TClonesArray("CbmTrdHit", 100))
{
}

//_______________________________________________________________________________
CbmTrdModuleRec::~CbmTrdModuleRec()
{
  if (fHits) {
    fHits->Delete();
    delete fHits;
  }
  if (fClusters) {
    fClusters->Delete();
    delete fClusters;
  }
}

//_______________________________________________________________________________
void CbmTrdModuleRec::Clear(Option_t* opt)
{
  fCalled++;
  if (strcmp(opt, "hit") == 0 && fHits) fHits->Clear();
  if (strcmp(opt, "cls") == 0 && fClusters) fClusters->Clear();
}

ClassImp(CbmTrdModuleRec)
