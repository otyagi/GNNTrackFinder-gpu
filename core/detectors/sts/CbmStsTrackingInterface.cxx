/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmStsTrackingInterface.cxx
 * @brief  Input data and parameters interface from STS subsystem used in L1 tracker (definition)
 * @since  27.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#include "CbmStsTrackingInterface.h"

#include "FairDetector.h"
#include "FairRunAna.h"
#include <Logger.h>

#include "TGeoPhysicalNode.h"

ClassImp(CbmStsTrackingInterface)

  // ---------------------------------------------------------------------------------------------------------------------
  //
  CbmStsTrackingInterface::CbmStsTrackingInterface()
  : FairTask("CbmStsTrackingInterface")
{
  if (!fpInstance) { fpInstance = this; }
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmStsTrackingInterface::~CbmStsTrackingInterface()
{
  if (fpInstance == this) { fpInstance = nullptr; }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::tuple<double, double> CbmStsTrackingInterface::GetStereoAnglesSensor(int address) const
{
  /// Gets front & back strip angle

  const CbmStsSensor* sensor =
    dynamic_cast<const CbmStsSensor*>(CbmStsSetup::Instance()->GetElement(address, EStsElementLevel::kStsSensor));

  assert(sensor);

  const TGeoPhysicalNode* node = sensor->GetNode();
  assert(node);
  const CbmStsParSensor* param = sensor->GetParams();
  assert(param);

  double angleF = param->GetPar(8) * TMath::DegToRad();
  double angleB = param->GetPar(9) * TMath::DegToRad();

  const TGeoHMatrix* matrix = node->GetMatrix();

  if (matrix) {
    TGeoRotation rot(*matrix);
    {
      Double_t local[3] = {TMath::Cos(angleF), TMath::Sin(angleF), 0.};
      Double_t global[3];
      rot.LocalToMaster(local, global);
      angleF = TMath::ATan2(global[1], global[0]);
    }
    {
      Double_t local[3] = {TMath::Cos(angleB), TMath::Sin(angleB), 0.};
      Double_t global[3];
      rot.LocalToMaster(local, global);
      angleB = TMath::ATan2(global[1], global[0]);
    }
  }

  return std::tuple(angleF, angleB);
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmStsTrackingInterface::Init()
{
  // Check, if all the necessary parameter containers were found
  if (!fStsParSetModule) { return kFATAL; }
  if (!fStsParSetSensor) { return kFATAL; }
  if (!fStsParSetSensorCond) { return kFATAL; }

  // Initialize CbmStsSetup instance
  auto stsSetup = CbmStsSetup::Instance();
  if (!stsSetup->IsInit()) { stsSetup->Init(nullptr); }
  if (!stsSetup->IsModuleParsInit()) { stsSetup->SetModuleParameters(fStsParSetModule); }
  if (!stsSetup->IsSensorParsInit()) { stsSetup->SetSensorParameters(fStsParSetSensor); }
  if (!stsSetup->IsSensorCondInit()) { stsSetup->SetSensorConditions(fStsParSetSensorCond); }

  int nStations = CbmStsSetup::Instance()->GetNofStations();
  fvStationFullVolume.clear();
  fvStationFullVolume.reserve(nStations);
  for (int iSt = 0; iSt < nStations; ++iSt) {
    const auto* pStsStation{CbmStsSetup::Instance()->GetStation(iSt)};
    if constexpr (!kLegacy) {
      fvStationFullVolume.emplace_back(pStsStation->GetXmin(), pStsStation->GetXmax(), pStsStation->GetYmin(),
                                       pStsStation->GetYmax(), pStsStation->GetZmin(), pStsStation->GetZmax());
    }
    else {
      fvStationFullVolume.emplace_back(pStsStation->GetXmin(), pStsStation->GetXmax(), pStsStation->GetYmin(),
                                       pStsStation->GetYmax(), pStsStation->GetZ() - 2., pStsStation->GetZ() + 2.);
    }
  }
  fvStationActiveVolume = fvStationFullVolume;


  // Check the validity of the parameters
  if (!this->Check()) {
    LOG(error)
      << "Some errors occurred in the tracking detector interface initialization for STS (see information above)";
    return kFATAL;
  }

  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmStsTrackingInterface::ReInit()
{
  this->SetParContainers();
  return Init();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmStsTrackingInterface::SetParContainers()
{
  auto runtimeDb       = FairRunAna::Instance()->GetRuntimeDb();
  fStsParSetModule     = dynamic_cast<CbmStsParSetModule*>(runtimeDb->getContainer("CbmStsParSetModule"));
  fStsParSetSensor     = dynamic_cast<CbmStsParSetSensor*>(runtimeDb->getContainer("CbmStsParSetSensor"));
  fStsParSetSensorCond = dynamic_cast<CbmStsParSetSensorCond*>(runtimeDb->getContainer("CbmStsParSetSensorCond"));
  if (!fStsParSetModule) {
    LOG(fatal) << "CbmStsTrackingInterface::SetParContainers: error accessing to CbmStsParSetModule container";
  }
  if (!fStsParSetSensor) {
    LOG(fatal) << "CbmStsTrackingInterface::SetParContainers: error accessing to CbmStsParSetSensor container";
  }
  if (!fStsParSetSensorCond) {
    LOG(fatal) << "CbmStsTrackingInterface::SetParContainers: error accessing to CbmStsParSetSensorCond container";
  }
}

