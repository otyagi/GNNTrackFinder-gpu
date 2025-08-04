/* Copyright (C) 2007-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Mikhail Ryzhinskiy, Florian Uhlig, Volker Friese [committer], Evgeny Kryshen */

/** CbmMuchStation.cxx
 *@author  M.Ryzhinskiy <m.ryzhinskiy@gsi.de>
 *@version 1.0
 *@since   11.02.08
 **
 ** This class holds the transport geometry parameters
 ** of one MuCh tracking station.
 **/
#include "CbmMuchStation.h"

#include "CbmMuchAddress.h"  // for CbmMuchAddress
#include "CbmMuchLayer.h"    // for CbmMuchLayer

#include <TMathBase.h>  // for Abs
#include <TObjArray.h>  // for TObjArray

// -----   Default constructor   -------------------------------------------
CbmMuchStation::CbmMuchStation()
  : TObject()
  , fDetectorId(0)
  , fZ(0.)
  , fLayers()
  , fRmin(0.)
  , fRmax(0.)
  , fModuleDesign(kFALSE)
  , fTubeRmin(0.)
  , fTubeRmax(0.)
  , fTubeZ(0.)
{
}
// -------------------------------------------------------------------------

// -----   Standard constructor   ------------------------------------------
CbmMuchStation::CbmMuchStation(Int_t iStation, Double_t z)
  : TObject()
  , fDetectorId(CbmMuchAddress::GetAddress(iStation))
  , fZ(z)
  , fLayers()
  , fRmin(0.)
  , fRmax(0.)
  , fModuleDesign(kFALSE)
  , fTubeRmin(0.)
  , fTubeRmax(0.)
  , fTubeZ(0.)
{
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMuchStation::~CbmMuchStation() {}
// -------------------------------------------------------------------------

// -----   Public method AddSector   ---------------------------------------
void CbmMuchStation::AddLayer(CbmMuchLayer* layer) { fLayers.Add(layer); }
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
Double_t CbmMuchStation::GetTubeDz() const
{
  Double_t dzmax = 0;

  for (Int_t l = 0; l < GetNLayers(); l++) {
    CbmMuchLayer* layer = GetLayer(l);
    Double_t ldz        = layer->GetDz();
    Double_t z          = layer->GetZ();
    Double_t dz         = TMath::Abs(z - fZ) + ldz;
    if (dz > dzmax) dzmax = dz;
  }

  return dzmax;
}
// -------------------------------------------------------------------------

ClassImp(CbmMuchStation)
