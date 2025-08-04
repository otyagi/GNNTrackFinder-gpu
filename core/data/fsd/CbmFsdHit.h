/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Lukas Chlad [committer] */

/** CbmFsdHit.h
 **@author Lukas Chlad <l.chlad@gsi.de>
 **@since 15.06.2023
 **@version 1
 **
 ** Data class for FSD reconstruction
 ** Energy deposition per module
 **
 **/


#ifndef CBMFSDHIT_H
#define CBMFSDHIT_H 1

#include "CbmPixelHit.h"  // for CbmPixelHit

#include <Rtypes.h>  // for ClassDef

#include <cstdint>

class CbmFsdHit : public CbmPixelHit {

public:
  /**   Default constructor   **/
  CbmFsdHit();

  CbmFsdHit(int32_t unit, int32_t module, double edep);

  CbmFsdHit(int32_t address, TVector3 pos, TVector3 dpos, int32_t refIndex, double time, double edep);


  /**   Destructor   **/
  virtual ~CbmFsdHit();


  /**   Setters - Getters   **/

  double GetEdep() const { return fEdep; }
  void SetEdep(double edep) { fEdep = edep; }

  int32_t GetModuleId() const { return fModuleId; }
  void SetModuleId(int32_t mod) { fModuleId = mod; }

  int32_t GetUnitId() const { return fUnitId; }
  void SetUnitId(int32_t unit) { fUnitId = unit; }

  void Print(Option_t* = "") const;

  std::string ToString() const;

private:
  /**   Data members  **/
  int32_t fUnitId;
  int32_t fModuleId;
  double fEdep;

  ClassDef(CbmFsdHit, 2);
};


#endif
