/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Lukas Chlad [committer] */

#ifndef CBMFSDDETECTORSPECS_H
#define CBMFSDDETECTORSPECS_H

#include <RtypesCore.h>  // for Double_t, Int_t
#include <TString.h>     // for TString

struct CbmFsdModuleSpecs {
  // location indices
  Int_t fUnitId;
  Int_t fModuleId;
  // module center position
  Double_t fX;
  Double_t fY;
  Double_t fZ;
  // module half-lengths
  Double_t dX;
  Double_t dY;
  Double_t dZ;
};

struct CbmFsdUnitSpecs {
  // name of unit
  TString fUnitName;
  // unit id
  Int_t fUnitId;
  // number of modules in unit
  Int_t fNumModules;
};

#endif /* CBMFSDDETECTORSPECS_H */
