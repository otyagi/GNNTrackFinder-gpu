/* Copyright (C) 2012-2015 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Michael Deveaux, Philipp Sitzmann [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                  CbmMvdSensorPlugin source file              -----
// -----                  Created 02.02.2012 by M. Deveaux            -----
// -------------------------------------------------------------------------
#include "CbmMvdSensorPlugin.h"

#include "TH1.h"

// -----   Default constructor   -------------------------------------------
CbmMvdSensorPlugin::CbmMvdSensorPlugin()
  : TObject()
  , fNextPlugin(nullptr)
  , fPreviousPlugin(nullptr)
  , bFlag(false)
  , initialized(kFALSE)
  , fShowDebugHistos(kFALSE)
  , fName("CbmMvdSensorPlugin")
  , fPluginIDNumber(-1)
{
}
// -------------------------------------------------------------------------
// -----   constructor   -------------------------------------------
CbmMvdSensorPlugin::CbmMvdSensorPlugin(const char* name)
  : TObject()
  , fNextPlugin(nullptr)
  , fPreviousPlugin(nullptr)
  , bFlag(false)
  , initialized(kFALSE)
  , fShowDebugHistos(kFALSE)
  , fName(name)
  , fPluginIDNumber(-1)
{
}
// -------------------------------------------------------------------------

TH1* CbmMvdSensorPlugin::GetHistogram(UInt_t number)
{
  if (fHistoArray) {
    if (number < (UInt_t) fHistoArray->GetEntriesFast()) { return (TH1*) fHistoArray->At(number); }
  }
  return 0;
}

// -----   Destructor   ----------------------------------------------------
CbmMvdSensorPlugin::~CbmMvdSensorPlugin() {}
// -------------------------------------------------------------------------


ClassImp(CbmMvdSensorPlugin)
