/* Copyright (C) 2023 Facility for AntiProton and Ion Research in Europe, Darmstadt.
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

#ifndef CBMGEOBMON_H
#define CBMGEOBMON_H

#include "FairGeoSet.h"

class CbmGeoBmon : public FairGeoSet {
protected:
  char modName[2];  // name of module
  char eleName[2];  // substring for elements in module
public:
  CbmGeoBmon();
  ~CbmGeoBmon() {}
  const char* getModuleName(Int_t) { return modName; }
  const char* getEleName(Int_t) { return eleName; }
  ClassDef(CbmGeoBmon, 0)  // Class for the geometry of BMON
};

#endif /* !CBMGEOBMON_H */
