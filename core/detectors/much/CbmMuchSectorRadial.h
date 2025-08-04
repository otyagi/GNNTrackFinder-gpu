/* Copyright (C) 2012-2020 Petersburg Nuclear Physics Institute named by B.P.Konstantinov of National Research Centre "Kurchatov Institute", Gatchina
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Evgeny Kryshen [committer], Florian Uhlig */

/** CbmMuchSectorRadial.h
 *@author  E.Kryshen <e.kryshen@gsi.de>
 *@since   6.02.12
 *@version 1.0
 **
 ** This class describes the digitization scheme for a ring of pads
 **
 **/
#ifndef CbmMuchSectorRadial_H
#define CbmMuchSectorRadial_H 1

#include "CbmMuchSector.h"  // for CbmMuchSector

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Double_t, Int_t

class CbmMuchPadRadial;

class CbmMuchSectorRadial : public CbmMuchSector {
public:
  CbmMuchSectorRadial();
  CbmMuchSectorRadial(Int_t detId, Int_t iSector, Double_t r1, Double_t r2, Double_t phi1, Double_t phi2);
  virtual ~CbmMuchSectorRadial() {}
  Int_t GetPadIndexByPhi(Double_t phi);
  CbmMuchPadRadial* GetPadByPhi(Double_t phi);
  Double_t GetR1() const { return fR1; }
  Double_t GetR2() const { return fR2; }
  void AddPads();
  void DrawPads();

private:
  Double_t fR1;
  Double_t fR2;
  Double_t fPhi1;
  Double_t fPhi2;
  Double_t fPadDphi;
  ClassDef(CbmMuchSectorRadial, 1);
};

#endif
