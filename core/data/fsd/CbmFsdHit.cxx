/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig, Lukas Chlad [committer] */

/** CbmFsdHit.cxx
 **@author Lukas Chlad <l.chlad@gsi.de>
 **@since 15.06.2023
 **
 **/
#include "CbmFsdHit.h"

#include "CbmFsdAddress.h"
#include "CbmHit.h"  // for kFSDHIT

#include <Logger.h>  // for Logger, LOG

// -----   Default constructor   -------------------------------------------
CbmFsdHit::CbmFsdHit() : CbmPixelHit(), fUnitId(-1), fModuleId(-1), fEdep(-1)
{
  SetType(kFSDHIT);
  SetTime(0.);
}

CbmFsdHit::CbmFsdHit(int32_t unit, int32_t module, double edep)
  : CbmPixelHit()
  , fUnitId(unit)
  , fModuleId(module)
  , fEdep(edep)
{
  SetType(kFSDHIT);
  SetTime(0.);
}

CbmFsdHit::CbmFsdHit(int32_t address, TVector3 pos, TVector3 dpos, int32_t refIndex, double time, double edep)
  : CbmPixelHit(address, pos, dpos, 0., refIndex, time)
  , fEdep(edep)
{
  SetType(kFSDHIT);
  SetUnitId(CbmFsdAddress::GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Unit)));
  SetModuleId(CbmFsdAddress::GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Module)));
}


// -----   Destructor   ----------------------------------------------------
CbmFsdHit::~CbmFsdHit() {}
// -------------------------------------------------------------------------

void CbmFsdHit::Print(Option_t*) const { LOG(info) << ToString(); }

std::string CbmFsdHit::ToString() const
{
  std::stringstream ss;
  ss << "unit : " << fUnitId << "module : " << fModuleId << "position: [" << GetX() << "," << GetY() << "," << GetZ()
     << "] "
     << " ELoss " << fEdep;
  return ss.str();
}

ClassImp(CbmFsdHit)
