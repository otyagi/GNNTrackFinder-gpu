/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmMvdTrackingInterface.cxx
 * @brief  Input data and parameters interface from MVD subsystem used in L1 tracker (definition)
 * @since  31.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#include "CbmMvdTrackingInterface.h"

#include "CbmMvdDetector.h"
#include "CbmMvdPoint.h"
#include "CbmMvdSensor.h"

#include <Logger.h>

ClassImp(CbmMvdTrackingInterface)

  // ---------------------------------------------------------------------------------------------------------------------
  //
  CbmMvdTrackingInterface::CbmMvdTrackingInterface()
  : FairTask("CbmMvdTrackingInterface")
{
  if (!fpInstance) { fpInstance = this; }
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmMvdTrackingInterface::~CbmMvdTrackingInterface()
{
  if (fpInstance == this) { fpInstance = nullptr; }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmMvdTrackingInterface::Init()
{
  fMvdStationPar = CbmMvdDetector::Instance()->GetParameterFile();

  if (!fMvdStationPar) { return kFATAL; }

  fvStationFullVolume.clear();
  // FIXME: Provide station parameters from geometry definition (seems to be unavailable)
  int nStations{fMvdStationPar->GetStationCount()};
  fvStationFullVolume.reserve(nStations);
  for (int iSt = 0; iSt < nStations; ++iSt) {
    fvStationFullVolume.emplace_back(-fMvdStationPar->GetWidth(iSt), +fMvdStationPar->GetWidth(iSt),
                                     -fMvdStationPar->GetHeight(iSt), +fMvdStationPar->GetHeight(iSt),
                                     fMvdStationPar->GetZPosition(iSt) - 0.5 * fMvdStationPar->GetZThickness(iSt),
                                     fMvdStationPar->GetZPosition(iSt) + 0.5 * fMvdStationPar->GetZThickness(iSt));
  }
  fvStationActiveVolume = fvStationFullVolume;

  // Check the validity of the parameters
  if (!this->Check()) {
    LOG(error)
      << "Some errors occurred in the tracking detector interface initialization for MVD (see information above)";
    return kFATAL;
  }

  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmMvdTrackingInterface::ReInit()
{
  this->SetParContainers();
  return Init();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmMvdTrackingInterface::SetParContainers() {}

// ---------------------------------------------------------------------------------------------------------------------
//
int CbmMvdTrackingInterface::GetTrackingStationIndex(const FairMCPoint* point) const
{
  const CbmMvdPoint* mvdPoint = [&] {
    if constexpr (kUseDynamicCast) { return dynamic_cast<const CbmMvdPoint*>(point); }
    else {
      return static_cast<const CbmMvdPoint*>(point);
    }
  }();
  assert(mvdPoint);
  const CbmMvdSensor* sensor = CbmMvdDetector::Instance()->GetSensorMap()[mvdPoint->GetDetectorID()];
  assert(sensor);
  return sensor->GetStationNr();
}
