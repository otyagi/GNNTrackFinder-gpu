/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmTofTrackingInterface.cxx
 * @brief  Input data and parameters interface from TOF subsystem used in L1 tracker (definition)
 * @since  23.06.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#include "CbmTofTrackingInterface.h"

#include "CbmTofCreateDigiPar.h"
#include "FairDetector.h"
#include "FairRunAna.h"

#include <Logger.h>

#include <limits>
#include <regex>

ClassImp(CbmTofTrackingInterface)

  // ---------------------------------------------------------------------------------------------------------------------
  //
  CbmTofTrackingInterface::CbmTofTrackingInterface()
  : FairTask("CbmTofTrackingInterface")
{
  if (!fpInstance) {
    fpInstance = this;
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmTofTrackingInterface::~CbmTofTrackingInterface()
{
  if (fpInstance == this) {
    fpInstance = nullptr;
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmTofTrackingInterface::Init()
{
  static_assert(std::is_trivially_copyable_v<VolumeInfo> == true);

  // create digitization parameters from geometry file
  auto tofDigiPar = new CbmTofCreateDigiPar("TOF Digi Producer", "TOF task");
  LOG(info) << "Create DigiPar";
  tofDigiPar->Init();

  // ** ToF tracking station geometrical information initialization **

  auto nStations = fDigiBdfPar->GetNbTrackingStations();
  // Init ToF stations position z-components. For each ToF tracking station the position z-component is calculated
  // as an average of the components for each ToF module inside the tracking station.
  // Number of ToF RPCs for a given tracking station:
  std::vector<int> nTofStationModules(nStations, 0);

  fvStationFullVolume.clear();
  fvStationFullVolume.resize(nStations);
  fvStationActiveVolume.clear();
  fvStationActiveVolume.resize(nStations);
  if constexpr (!kLegacy) {
    // loop over all RPCs; assign a tracking station ID using DigiBdfPar; combine the RPCs for each station ID
    auto vRpcPaths{CollectNodes("tof", "counter", "", gGeoManager->GetTopNode())};
    std::regex rpcPattern{"module_(\\d+)_(\\d+)/gas_box_(\\d+)/counter_(\\d+)"};
    for (const auto& rpcPath : vRpcPaths) {
      std::smatch match;
      std::string line{rpcPath.Data()};
      if (std::regex_search(line, match, rpcPattern)) {
        int iSmType{std::stoi(match[1])};
        int iSm{std::stoi(match[2])};
        int iRpc{std::stoi(match[4])};
        int iStation{fDigiBdfPar->GetTrackingStation(iSmType, iSm, iRpc)};

        if (5 == iSmType || iStation < 0) {  // NOTE: Check for BeamOn modules or other inactive RPCs
          continue;
        }

        fvStationFullVolume[iStation] += ReadVolume(rpcPath);  // Adding RPC as a passive volume
        gGeoManager->cd(rpcPath);
        auto vCellPaths{CollectNodes("tof", "cell", rpcPath(0, rpcPath.Last('/')), gGeoManager->GetCurrentNode())};
        for (const auto& cellPath : vCellPaths) {
          fvStationActiveVolume[iStation] += ReadVolume(cellPath);
        }
      }
    }
  }
  else {  // old tracking station definition
    fTofStationZ.clear();
    fTofStationZ.resize(nStations);
    fTofStationZMin.clear();
    fTofStationZMin.resize(nStations, std::numeric_limits<double>::max());
    fTofStationZMax.clear();
    fTofStationZMax.resize(nStations, std::numeric_limits<double>::lowest());

    for (int iSmType{0}; iSmType < fDigiBdfPar->GetNbSmTypes(); ++iSmType) {
      for (int iSm{0}; iSm < fDigiBdfPar->GetNbSm(iSmType); ++iSm) {
        for (int iRpc{0}; iRpc < fDigiBdfPar->GetNbRpc(iSmType); ++iRpc) {
          auto address{CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iSmType)};
          int iStation{fDigiBdfPar->GetTrackingStation(iSmType, iSm, iRpc)};  // Local index of tracking station
          auto* pChannelInfo{dynamic_cast<CbmTofCell*>(fDigiPar->GetCell(address))};
          if (nullptr == pChannelInfo) {
            LOG(warn) << fName << ": CbmTofCell object is not defined for iSmType = " << iSmType << ", iSm = " << iSm
                      << ", iRpc = " << iRpc;
            continue;
          }

          // Tracking station sizes
          auto chPosZ{pChannelInfo->GetZ()};

          // Cuts on Bmon and undefined station ID
          if (5 == iSmType) {
            continue;
          }  // Skip Bmon
          if (iStation < 0) {
            continue;
          }

          fTofStationZ[iStation] += chPosZ;
          if (chPosZ > fTofStationZMax[iStation]) {
            fTofStationZMax[iStation] = chPosZ;
          }
          if (chPosZ < fTofStationZMin[iStation]) {
            fTofStationZMin[iStation] = chPosZ;
          }

          nTofStationModules[iStation] += 1;
        }
      }
    }

    /// Get the average values and define final arrays
    for (int iSt{0}; iSt < nStations; ++iSt) {
      fTofStationZ[iSt] = fTofStationZ[iSt] / nTofStationModules[iSt];
      auto& station{fvStationActiveVolume[iSt]};
      station.fXmin = -100.;
      station.fXmax = +100.;
      station.fYmin = -100.;
      station.fYmax = +100.;
      station.fZmin = fTofStationZMin[iSt] - .5;
      station.fZmax = fTofStationZMax[iSt] + .5;
    }
    fvStationFullVolume = fvStationActiveVolume;
  }

  // Check the validity of the parameters
  if (!this->Check()) {
    LOG(error)
      << "Some errors occurred in the tracking detector interface initialization for TOF (see information above)";
    return kFATAL;
  }

  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmTofTrackingInterface::ReInit()
{
  this->SetParContainers();
  return Init();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmTofTrackingInterface::SetParContainers()
{

  auto runtimeDb = FairRunAna::Instance()->GetRuntimeDb();
  fDigiPar       = dynamic_cast<CbmTofDigiPar*>(runtimeDb->getContainer("CbmTofDigiPar"));
  fDigiBdfPar    = dynamic_cast<CbmTofDigiBdfPar*>(runtimeDb->getContainer("CbmTofDigiBdfPar"));
  if (!fDigiPar) {
    LOG(fatal) << "CbmTofTrackingInterface::SetParContainers: error accessing to CbmTofDigiPar container";
  }
  if (!fDigiBdfPar) {
    LOG(fatal) << "CbmTofTrackingInterface::SetParContainers: error accessing to CbmTofDigiBdfPar container";
  }
  runtimeDb->initContainers(FairRunAna::Instance()->GetRunId());
}
