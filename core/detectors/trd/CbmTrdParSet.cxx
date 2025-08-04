/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci */

#include "CbmTrdParSet.h"

#include "CbmTrdParMod.h"  // for CbmTrdParMod

#include <FairParGenericSet.h>  // for FairParGenericSet
#include <FairParamList.h>      // for FairParamList

#include <TGenericClassInfo.h>  // for TGenericClassInfo

#include <utility>  // for pair

#include <stdio.h>  // for printf

using std::map;
//_______________________________________________________________________________
CbmTrdParSet::CbmTrdParSet(const char* name, const char* title, const char* context)
  : FairParGenericSet(name, title, context)
  , fNrOfModules(0)
  , fModuleMap()
{
  //printf("%s (%s, %s, %s)\n", GetName(), name, title, context);
}

//_______________________________________________________________________________
CbmTrdParSet::~CbmTrdParSet()
{
  //   for (map<Int_t, CbmTrdParMod*>::iterator imod = fModuleMap.begin(); imod != fModuleMap.end(); imod++)
  //     if (imod->second) delete imod->second;  // TODO check ownership of module parameters
  fModuleMap.clear();
}

//_______________________________________________________________________________
Int_t CbmTrdParSet::GetModuleId(Int_t i) const
{
  if (i < 0 || i >= fNrOfModules) return -1;
  Int_t j(0);
  for (map<Int_t, CbmTrdParMod*>::const_iterator imod = fModuleMap.begin(); imod != fModuleMap.end(); imod++, j++) {
    if (j < i) continue;
    return imod->first;
  }
  return -1;
}

//_______________________________________________________________________________
const CbmTrdParMod* CbmTrdParSet::GetModulePar(Int_t detId) const
{
  map<Int_t, CbmTrdParMod*>::const_iterator imod = fModuleMap.find(detId);
  if (imod == fModuleMap.end()) return nullptr;
  return imod->second;
}

//_______________________________________________________________________________
CbmTrdParMod* CbmTrdParSet::GetModulePar(Int_t detId)
{
  map<Int_t, CbmTrdParMod*>::const_iterator imod = fModuleMap.find(detId);
  if (imod == fModuleMap.end()) return nullptr;
  return imod->second;
}

//_______________________________________________________________________________
Bool_t CbmTrdParSet::getParams(FairParamList* l)
{
  if (!l) return kFALSE;
  l->print();
  return kTRUE;
}


//_______________________________________________________________________________
void CbmTrdParSet::putParams(FairParamList* /*l*/)
{
  printf("%s(%s)::putParams not implemented \n", GetName(), GetTitle());
}

//_______________________________________________________________________________
void CbmTrdParSet::addParam(CbmTrdParMod* mod)
{
  fModuleMap[mod->GetModuleId()] = mod;
  fNrOfModules++;
}

//_______________________________________________________________________________
void CbmTrdParSet::Print(Option_t* opt) const
{
  printf(" %s Modules[%d]\n", GetName(), fNrOfModules);
  for (auto imod : fModuleMap) {
    printf("  %d %s(%s)\n", imod.first, imod.second->GetName(), imod.second->GetTitle());
    imod.second->Print(opt);
  }
}

ClassImp(CbmTrdParSet)
