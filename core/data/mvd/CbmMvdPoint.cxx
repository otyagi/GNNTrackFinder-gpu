/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Florian Uhlig, Philipp Sitzmann */

// -------------------------------------------------------------------------
// -----                      CbmMvdPoint source file                  -----
// -----                  Created 06/11/06  by V. Friese               -----
// -------------------------------------------------------------------------
#include "CbmMvdPoint.h"

#include <FairMCEventHeader.h>     // for FairMCEventHeader
#include <FairMCPoint.h>           // for FairMCPoint
#include <FairPrimaryGenerator.h>  // for FairPrimaryGenerator
#include <FairRunSim.h>            // for FairRunSim
#include <Logger.h>                // for Logger, LOG

#include <TVector3.h>  // for TVector3

// -----   Default constructor   -------------------------------------------
CbmMvdPoint::CbmMvdPoint()
  : FairMCPoint()
  , CbmMvdDetectorId()
  , fX_out(0)
  , fY_out(0)
  , fZ_out(0)
  , fPx_out(0)
  , fPy_out(0)
  , fPz_out(0)
  , fPdgCode(0)
  , fPointId(-1)
  , fFrame(0)
  , fStartTime(0.)
{
}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmMvdPoint::CbmMvdPoint(int32_t trackID, int32_t pdgCode, int32_t stationNr, TVector3 posIn, TVector3 posOut,
                         TVector3 momIn, TVector3 momOut, double tof, double length, double eLoss, int32_t frame)
  : FairMCPoint(trackID, stationNr, posIn, momIn, tof, length, eLoss)
  , CbmMvdDetectorId()
  , fX_out(posOut.X())
  , fY_out(posOut.Y())
  , fZ_out(posOut.Z())
  , fPx_out(momOut.Px())
  , fPy_out(momOut.Py())
  , fPz_out(momOut.Pz())
  , fPdgCode(pdgCode)
  , fPointId(-1)
  , fFrame(frame)
  , fStartTime(0)
{
  FairRunSim* run           = FairRunSim::Instance();
  FairPrimaryGenerator* gen = run->GetPrimaryGenerator();
  FairMCEventHeader* event  = gen->GetEvent();

  fStartTime  = event->GetT();
  fDetectorID = DetectorId(stationNr);
}

// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmMvdPoint::~CbmMvdPoint() {}
// -------------------------------------------------------------------------


// -----   Public method Print   -------------------------------------------
void CbmMvdPoint::Print(const Option_t* /*opt*/) const
{
  LOG(info) << "MVD Point for track " << fTrackID << " in station " << GetStationNr();
  LOG(info) << "    Position (" << fX << ", " << fY << ", " << fZ << ") cm";
  LOG(info) << "    Momentum (" << fPx << ", " << fPy << ", " << fPz << ") GeV";
  LOG(info) << "    Time " << fTime << " ns,  Length " << fLength << " cm,  Energy loss " << fELoss * 1.0e06 << " keV";
}
// -------------------------------------------------------------------------

// -----   Public method GetAbsTime   --------------------------------------
int32_t CbmMvdPoint::GetAbsTime()
{


  int32_t absTime = fTime + fStartTime;

  return absTime;
}
// -------------------------------------------------------------------------

ClassImp(CbmMvdPoint)
