/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmKfTrackingSetupBuilder.cxx
/// \brief  Tracking setup initializer in CBM (source)
/// \since  28.08.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmKfTrackingSetupBuilder.h"

#include "CbmKfOriginalField.h"
#include "CbmKfTarget.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmSetup.h"
#include "CbmStsFindTracks.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdTrackingInterface.h"
#include "FairField.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "KfDefs.h"
#include "KfMaterialMapFactory.h"
#include "Logger.h"

#include <boost/filesystem.hpp>

#include <functional>

using cbm::algo::ca::EDetectorID;
using cbm::kf::TrackingSetupBuilder;
using kf::tools::MaterialMapFactory;

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingSetupBuilder::CheckDetectorPresence()
{
  LOG(info) << "TrackingSetupBuilder: detector subsystems in geometry: ";
  auto Check = [&](EDetectorID detID) {
    auto modId = cbm::algo::ca::ToCbmModuleId(detID);
    fvbDetInGeometry[detID] = CbmSetup::Instance()->IsActive(modId);
    if (!fbIgnoreHitPresence) {
      fvbDetHasHits[detID] = fvbDetInGeometry[detID] && FairRootManager::Instance()->GetObject(kDetHitBrName[detID]);
      LOG(info) << fmt::format("\t{:6} in geometry: {:5}, has hits: {:5}", ToString(modId), fvbDetInGeometry[detID],
                               fvbDetHasHits[detID]);
    }
    else {
      fvbDetHasHits[detID] = fvbDetInGeometry[detID];
      LOG(info) << fmt::format("\t{:6} in geometry: {:5}", ToString(modId), fvbDetInGeometry[detID]);
    }
  };

  Check(EDetectorID::kMvd);
  Check(EDetectorID::kSts);
  Check(EDetectorID::kMuch);
  Check(EDetectorID::kTrd);
  Check(EDetectorID::kTof);
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::shared_ptr<const cbm::algo::kf::Setup<double>> TrackingSetupBuilder::GetSharedGeoSetup()
{
  using cbm::algo::kf::EFieldMode;
  using cbm::algo::kf::Setup;
  if (!fpGeoSetup.get()) {
    fpGeoSetup = std::make_shared<Setup<double>>(this->MakeSetup<double>(EFieldMode::Orig));
  }
  return fpGeoSetup;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingSetupBuilder::Init()
try {
  using cbm::algo::kf::EFieldType;
  using cbm::algo::kf::GeoLayer;
  using cbm::algo::kf::defs::MinField;
  CheckDetectorPresence();

  fBuilder.Reset();
  // Magnetic field initialization
  if (auto* pField = FairRunAna::Instance()->GetField()) {
    LOG(info) << fabs(pField->GetBx(0., 0., 0.)) << ", " << fabs(pField->GetBy(0., 0., 0.)) << ", "
              << fabs(pField->GetBz(0., 0., 0.)) << ", " << pField->GetType();
    if (
      pField->GetType() == 0
      && fabs(pField->GetBx(0., 0., 0.)) < MinField<
           double> && fabs(pField->GetBy(0., 0., 0.)) < MinField<double> && fabs(pField->GetBz(0., 0., 0.)) < MinField<double>) {
      fBuilder.SetFieldFunction(cbm::kf::ZeroField(), EFieldType::Null);
    }
    else {
      fBuilder.SetFieldFunction(cbm::kf::OriginalField(), EFieldType::Normal);
    }
  }
  else {
    fBuilder.SetFieldFunction(cbm::kf::ZeroField(), EFieldType::Null);
  }

  // Tracking station property initialization
  auto CollectStations = [&](const auto* pIfs, EDetectorID detID) -> void {
    if (!fvbDetInGeometry[detID]) {
      return;
    }
    for (int iSt = 0; iSt < pIfs->GetNtrackingStations(); ++iSt) {
      fBuilder.AddLayer(GeoLayer<EDetectorID>{detID,  // ca::Tracking detector id scheme
                                              iSt,    // ca::Tracking station indexing
                                              pIfs->GetZref(iSt), pIfs->GetZmin(iSt), pIfs->GetZmax(iSt),
                                              std::max(std::fabs(pIfs->GetXmin(iSt)), std::fabs(pIfs->GetXmax(iSt))),
                                              std::max(std::fabs(pIfs->GetYmin(iSt)), std::fabs(pIfs->GetYmax(iSt)))});
    }
  };
  CollectStations(CbmMvdTrackingInterface::Instance(), EDetectorID::kMvd);
  CollectStations(CbmStsTrackingInterface::Instance(), EDetectorID::kSts);
  CollectStations(CbmMuchTrackingInterface::Instance(), EDetectorID::kMuch);
  CollectStations(CbmTrdTrackingInterface::Instance(), EDetectorID::kTrd);
  CollectStations(CbmTofTrackingInterface::Instance(), EDetectorID::kTof);

  // Retrieve target properties
  const auto* pTarget = cbm::kf::Target::Instance();
  fBuilder.SetTargetProperty(pTarget->GetX(), pTarget->GetY(), pTarget->GetZ(), pTarget->GetDz(), pTarget->GetRmax());

  // Init material map creator
  auto pMaterialFactory = std::make_shared<MaterialMapFactory>();
  pMaterialFactory->SetSafeMaterialInitialization(kMatCreatorSafeMode);
  pMaterialFactory->SetDoRadialProjection(pTarget->GetZ());
  pMaterialFactory->SetNraysPerDim(kMatCreatorNrays);
  fBuilder.SetMaterialMapFactory(pMaterialFactory);

  // Set the initialization flags back
  fbInitialized = true;
  LOG(info) << "ca::TrackingSetupBuilder: Tracking setup was initialized successfully";
}
catch (const std::exception& err) {
  fbInitialized = false;
  LOG(error) << "ca::TrackingSetupBuilder: Tracking setup was not initialized. Reason: " << err.what();
}

// ---------------------------------------------------------------------------------------------------------------------
//
TrackingSetupBuilder* TrackingSetupBuilder::Instance()
{
  namespace fs = boost::filesystem;
  if (fpInstance == nullptr) {
    std::lock_guard<std::mutex> lock(fMutex);
    fpInstance = new TrackingSetupBuilder{};
    fpInstance->CheckDetectorPresence();

    // Retrieve the geometry tag
    auto setupTag = CbmSetup::Instance()->GetProvider()->GetSetup().GetTag();
    if (setupTag.empty()) {
      throw std::logic_error("The setup tag in CbmSetup is not defined");
    }

    // Retrieve the data directory (the same as for the sink file)
    TString sinkName     = FairRootManager::Instance()->GetSink()->GetFileName();
    auto sinkPath        = fs::path{sinkName.Data()};
    std::string sDataDir = sinkPath.parent_path().string();
    if (sDataDir.empty()) {
      sDataDir = ".";
    }

    std::string sCacheFile = Form("%s/%s.mat.kf.bin", sDataDir.c_str(), setupTag.c_str());
    fpInstance->SetMaterialCacheFile(sCacheFile, CbmSetup::Instance()->GetHash());
  }
  return fpInstance;
}
