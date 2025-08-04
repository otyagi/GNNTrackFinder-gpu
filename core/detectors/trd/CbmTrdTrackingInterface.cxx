/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmTrdTrackingInterface.cxx
 * @brief  Input data and parameters interface from TRD subsystem used in L1 tracker (definition)
 * @since  31.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#include "CbmTrdTrackingInterface.h"

#include "CbmTrdHit.h"
#include "FairDetector.h"
#include "FairRunAna.h"
#include "TGeoManager.h"
#include "TString.h"

#include <Logger.h>

#include <regex>

ClassImp(CbmTrdTrackingInterface)

  // ---------------------------------------------------------------------------------------------------------------------
  //
  CbmTrdTrackingInterface::CbmTrdTrackingInterface()
  : FairTask("CbmTrdTrackingInterface")
{
  if (!fpInstance) { fpInstance = this; }
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmTrdTrackingInterface::~CbmTrdTrackingInterface()
{
  if (fpInstance == this) { fpInstance = nullptr; }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::tuple<double, double> CbmTrdTrackingInterface::GetStereoAnglesSensor(int address) const
{
  const CbmTrdParModDigi* par = dynamic_cast<const CbmTrdParModDigi*>(fTrdDigiPar->GetModulePar(address));
  if (!par) {
    LOG(fatal) << "CbmTrdTrackingInterface::Init: error accessing the TRD module for address " << address
               << " (failed dynamic cast to CbmTrdParModDigi)";
  }
  if ((par->GetOrientation() == 1) || (par->GetOrientation() == 3)) {
    // swap X & Y for orientations 1 or 3
    return std::tuple(TMath::Pi() / 2., 0.);
  }
  return std::tuple(0., TMath::Pi() / 2.);
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::tuple<double, double, double> CbmTrdTrackingInterface::GetHitRanges(const CbmPixelHit& hit) const
{
  const CbmTrdHit* trdHit = dynamic_cast<const CbmTrdHit*>(&hit);
  if (!trdHit) { LOG(fatal) << "CbmTrdTrackingInterface::GetHitRange: input hit is not of type CbmTrdHit"; }
  const CbmTrdParModDigi* par = dynamic_cast<const CbmTrdParModDigi*>(fTrdDigiPar->GetModulePar(trdHit->GetAddress()));

  auto [rangeX, rangeY, rangeT] = CbmTrackingDetectorInterfaceBase::GetHitRanges(hit);

  if (trdHit->GetClassType() == 1) {
    // TRD 2D hit
    // TODO: replace with more sophisticated values once the errors are correct
    rangeX = 1.7;
    rangeY = 3.5;
    rangeT = 200.;
  }
  else {  // TRD 1D hit
    if ((par->GetOrientation() == 1) || (par->GetOrientation() == 3)) {
      // Y coordinate is uniformly distributed
      rangeY = sqrt(3.) * trdHit->GetDy();
    }
    else {  // X coordinate is uniformly distributed
      rangeX = sqrt(3.) * trdHit->GetDx();
    }
  }

  return std::tuple(rangeX, rangeY, rangeT);
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmTrdTrackingInterface::Init()
{
  // Collect nodes
  if constexpr (!kLegacy) {
    auto vLayerPaths{CollectNodes("trd", "layer", "", gGeoManager->GetTopNode())};
    fvStationFullVolume.clear();
    fvStationFullVolume.resize(vLayerPaths.size());
    fvStationActiveVolume.clear();
    fvStationActiveVolume.resize(vLayerPaths.size());
    std::regex stationPattern{"layer(\\d+)_(\\d+)"};
    for (const auto& layerPath : vLayerPaths) {
      std::smatch match;
      std::string line{layerPath.Data()};
      if (std::regex_search(line, match, stationPattern)) {
        int iStation{std::stoi(match[1]) - 1};  // the tracking station is a TRD layer
        fvStationFullVolume[iStation] = ReadVolume(layerPath);
        gGeoManager->cd(layerPath);
        auto vGasPaths{CollectNodes("trd", "gas", layerPath(0, layerPath.Last('/')), gGeoManager->GetCurrentNode())};
        for (const auto& gasPath : vGasPaths) {
          fvStationActiveVolume[iStation] += ReadVolume(gasPath);
        }
      }
    }
  }
  else {
    int nStations = 0;
    {
      auto topNodes = gGeoManager->GetTopNode()->GetNodes();
      for (int iTopNode = 0; iTopNode < topNodes->GetEntriesFast(); ++iTopNode) {
        auto topNode = static_cast<TGeoNode*>(topNodes->At(iTopNode));
        if (TString(topNode->GetName()).Contains("trd")) {
          auto layers = topNode->GetNodes();
          for (int iLayer = 0; iLayer < layers->GetEntriesFast(); ++iLayer) {
            auto layer = static_cast<TGeoNode*>(layers->At(iLayer));
            if (TString(layer->GetName()).Contains("layer")) {
              ++nStations;
            }
          }
        }
      }
    }
    fvStationFullVolume.clear();
    fvStationFullVolume.reserve(nStations);
    for (int iSt = 0; iSt < nStations; ++iSt) {
      const auto* pModulePar = GetTrdModulePar(iSt);
      fvStationFullVolume.emplace_back(
        pModulePar->GetX() - pModulePar->GetSizeX(), pModulePar->GetX() + pModulePar->GetSizeX(),
        pModulePar->GetY() - pModulePar->GetSizeY(), pModulePar->GetY() + pModulePar->GetSizeY(),
        pModulePar->GetZ() - pModulePar->GetSizeZ(), pModulePar->GetZ() + pModulePar->GetSizeZ());
    }
    fvStationActiveVolume = fvStationFullVolume;
  }

  // Check access to TRD modules
  for (int iSt = 0; iSt < this->GetNtrackingStations(); ++iSt) {
    if (!dynamic_cast<CbmTrdParModDigi*>(fTrdDigiPar->GetModulePar(fTrdDigiPar->GetModuleId(iSt)))) {
      LOG(fatal) << "CbmTrdTrackingInterface::Init: error accessing the TRD tracking station " << iSt
                 << " (failed dynamic cast to CbmTrdParModDigi)";
    }
  }

  // Check the validity of the parameters
  if (!this->Check()) {
    LOG(error)
      << "Some errors occurred in the tracking detector interface initialization for TRD (see information above)";
    return kFATAL;
  }

  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmTrdTrackingInterface::ReInit()
{
  this->SetParContainers();
  return Init();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmTrdTrackingInterface::SetParContainers()
{
  auto runtimeDb = FairRunAna::Instance()->GetRuntimeDb();
  fTrdDigiPar    = dynamic_cast<CbmTrdParSetDigi*>(runtimeDb->getContainer("CbmTrdParSetDigi"));
  if (!fTrdDigiPar) {
    LOG(fatal) << "CbmTrdTrackingInterface::SetParContainers: error accessing to CbmTrdParSetDigi container";
  }
}

