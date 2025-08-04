/* Copyright (C) 2018-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci */

#include "CbmTrdModuleAbstract.h"

#include "CbmTrdParAsic.h"

#include <Logger.h>

//_______________________________________________________________________________
CbmTrdModuleAbstract::CbmTrdModuleAbstract() : TNamed() {}

//_______________________________________________________________________________
CbmTrdModuleAbstract::CbmTrdModuleAbstract(Int_t mod, Int_t ly, Int_t rot)
  : TNamed("CbmTrdModule", "Abstract TRD module implementation")
{
  fModAddress = mod;
  fLayerId    = ly;
  fRotation   = rot;
}

//_______________________________________________________________________________
CbmTrdModuleAbstract::~CbmTrdModuleAbstract()
{
  if (fAsicPar) delete fAsicPar;
}

//_______________________________________________________________________________
bool CbmTrdModuleAbstract::IsChannelMasked(int ch) const
{
  if (!fAsicPar) return false;
  const CbmTrdParAsic* p = fAsicPar->GetAsicPar(ch);
  if (!p) return false;
  return p->IsChannelMasked(ch);
}

//_______________________________________________________________________________
void CbmTrdModuleAbstract::LocalToMaster(Double_t in[3], Double_t out[3])
{
  if (!fGeoPar) return;
  fGeoPar->LocalToMaster(in, out);
}

ClassImp(CbmTrdModuleAbstract)
