/* Copyright (C) 2023 Facility for AntiProton and Ion Research in Europe, Darmstadt.
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

#include "CbmGeoBmon.h"

ClassImp(CbmGeoBmon)

  CbmGeoBmon::CbmGeoBmon()
  : FairGeoSet()
  , modName()
  , eleName()
{
  // Constructor
  fName      = "bmon";
  maxSectors = 0;
  maxModules = 1;
  strcpy(modName, "t");
  strcpy(eleName, "t");
}
