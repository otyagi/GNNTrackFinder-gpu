/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig, Lukas Chlad [committer] */

#include "CbmFsdPoint.h"

#include <FairMCPoint.h>  // for FairMCPoint
#include <Logger.h>       // for Logger, LOG

#include <sstream>  // for stringstream

// -----   Default constructor   -------------------------------------------
CbmFsdPoint::CbmFsdPoint() : FairMCPoint() {}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmFsdPoint::CbmFsdPoint(int32_t trackID, int32_t detID, TVector3 pos, TVector3 mom, double tof, double length,
                         double eLoss)
  : FairMCPoint(trackID, detID, pos, mom, tof, length, eLoss)
{
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmFsdPoint::~CbmFsdPoint() {}
// -------------------------------------------------------------------------


// -----   Public method Print   -------------------------------------------
void CbmFsdPoint::Print(const Option_t* /*opt*/) const { LOG(info) << ToString(); }

std::string CbmFsdPoint::ToString() const
{
  std::stringstream ss;
  ss << "Fsd point for track " << fTrackID << " in detector " << fDetectorID << "\n"
     << "      Position (" << fX << ", " << fY << ", " << fZ << ") cm\n"
     << "      Momentum (" << fPx << ", " << fPy << ", " << fPz << ") GeV\n"
     << "      Time " << fTime << " ns,  Length " << fLength << " cm,  Energy loss " << fELoss * 1.0e06 << " keV";
  return ss.str();
}
// -------------------------------------------------------------------------

ClassImp(CbmFsdPoint)
