/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Lukas Chlad [committer] */

#include "CbmFsdDigiPar.h"

#include <FairParGenericSet.h>  // for FairParGenericSet
#include <FairParamList.h>      // for FairParamList
#include <Logger.h>             // for LOG, Logger

ClassImp(CbmFsdDigiPar)

  CbmFsdDigiPar::CbmFsdDigiPar(const char* name, const char* title, const char* context)
  : FairParGenericSet(name, title, context)
  , fNumPhotoDets(-1)
  , fTimeResolution(-1.)
  , fEnergyResolution(-1.)
{
  detName = "Fsd";
}

CbmFsdDigiPar::~CbmFsdDigiPar(void) { clear(); }

void CbmFsdDigiPar::clear(void)
{
  status = kFALSE;
  resetInputVersions();
}

void CbmFsdDigiPar::putParams(FairParamList* l)
{
  if (!l) { return; }

  l->add("NumPhotoDets", fNumPhotoDets);
  l->add("NumUnits", fNumUnits);
  l->add("TimeResolution", fTimeResolution);
  l->add("EnergyResolution", fEnergyResolution);
  l->add("DeadTime", fDeadTime);
}

Bool_t CbmFsdDigiPar::getParams(FairParamList* l)
{
  if (!l) { return kFALSE; }

  LOG(debug2) << "Get the FSD digitization parameters.";

  if (!l->fill("NumPhotoDets", &fNumPhotoDets)) return kFALSE;
  if (!l->fill("NumUnits", &fNumUnits)) return kFALSE;

  fTimeResolution.Set(fNumUnits);
  fEnergyResolution.Set(fNumUnits);
  fDeadTime.Set(fNumUnits);
  if (!l->fill("TimeResolution", &fTimeResolution)) return kFALSE;
  if (!l->fill("EnergyResolution", &fEnergyResolution)) return kFALSE;
  if (!l->fill("DeadTime", &fDeadTime)) return kFALSE;

  return kTRUE;
}

Double_t CbmFsdDigiPar::GetTimeResolution(Int_t iUnitId) const
{
  if (iUnitId < fNumUnits) return fTimeResolution[iUnitId];
  else
    return -1.;
}

Double_t CbmFsdDigiPar::GetEnergyResolution(Int_t iUnitId) const
{
  if (iUnitId < fNumUnits) return fEnergyResolution[iUnitId];
  else
    return -1.;
}

Double_t CbmFsdDigiPar::GetDeadTime(Int_t iUnitId) const
{
  if (iUnitId < fNumUnits) return fDeadTime[iUnitId];
  else
    return -1.;
}
