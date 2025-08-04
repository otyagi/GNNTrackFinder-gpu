/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Mikhail Ryzhinskiy, Volker Friese, Florian Uhlig, Denis Bertini [committer] */

/** CbmMuchPoint.cxx
 *
 * @author  M.Ryzhinskiy <m.ryzhinskiy@gsi.de>
 * @version 1.0
 * @since   21.03.07
 *
 *  Class for Monte Carlo points in MUon CHambers detector
 *
 */

#include "CbmMuchPoint.h"

#include <FairMCPoint.h>  // for FairMCPoint
#include <Logger.h>       // for Logger, LOG

#include <TVector3.h>   // for TVector3

#include <cmath>

// -----   Default constructor   -------------------------------------------
CbmMuchPoint::CbmMuchPoint() : FairMCPoint(), fX_out(0.), fY_out(0.), fZ_out(0.), fPx_out(0.), fPy_out(0.), fPz_out(0.)
{
}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmMuchPoint::CbmMuchPoint(int32_t trackID, int32_t detID, TVector3 posIn, TVector3 posOut, TVector3 momIn,
                           TVector3 momOut, double tof, double length, double eLoss, int32_t eventId)
  : FairMCPoint(trackID, detID, posIn, momIn, tof, length, eLoss, eventId)
  , fX_out(posOut.X())
  , fY_out(posOut.Y())
  , fZ_out(posOut.Z())
  , fPx_out(momOut.Px())
  , fPy_out(momOut.Py())
  , fPz_out(momOut.Pz())
{
}
// -------------------------------------------------------------------------


// -----   Copy constructor with event and epoch time   --------------------
CbmMuchPoint::CbmMuchPoint(const CbmMuchPoint& point, int32_t eventId, double eventTime, double epochTime)
  : FairMCPoint(point)
  , fX_out(point.fX_out)
  , fY_out(point.fY_out)
  , fZ_out(point.fZ_out)
  , fPx_out(point.fPx_out)
  , fPy_out(point.fPy_out)
  , fPz_out(point.fPz_out)
{
  //  *this = point;
  if (eventId > 0) fEventId = eventId;
  fTime = point.GetTime() + eventTime - epochTime;
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmMuchPoint::~CbmMuchPoint() {}
// -------------------------------------------------------------------------


// -----   Public method Print   -------------------------------------------
void CbmMuchPoint::Print(const Option_t* /*opt*/) const
{
  LOG(info) << "-I- CbmMuchPoint: MUCH Point for track " << fTrackID << " in detector " << fDetectorID;
  LOG(info) << "    Position (" << fX << ", " << fY << ", " << fZ << ") cm";
  LOG(info) << "    Momentum (" << fPx << ", " << fPy << ", " << fPz << ") GeV";
  LOG(info) << "    Time " << fTime << " ns,  Length " << fLength << " cm,  Energy loss " << fELoss * 1.0e06 << " keV";
}
// -------------------------------------------------------------------------


// -----   Point x coordinate from linear extrapolation   ------------------
double CbmMuchPoint::GetX(double z) const
{
  double dz = fZ_out - fZ;
  if (abs(dz) < 1.e-3) return (fX_out + fX) / 2.;
  return (fX + (z - fZ) / dz * (fX_out - fX));
}
// -------------------------------------------------------------------------


// -----   Point y coordinate from linear extrapolation   ------------------
double CbmMuchPoint::GetY(double z) const
{
  double dz = fZ_out - fZ;
  if (abs(dz) < 1.e-3) return (fY_out + fY) / 2.;
  return (fY + (z - fZ) / dz * (fY_out - fY));
}
// -------------------------------------------------------------------------


// -----   Public method IsUsable   ----------------------------------------
bool CbmMuchPoint::IsUsable() const
{
  double dz = fZ_out - fZ;
  if (abs(dz) < 1.e-4) return false;
  return true;
}
// -------------------------------------------------------------------------


ClassImp(CbmMuchPoint)
