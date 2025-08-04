/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmGenerateMaterialMaps.h
/// \brief  A FAIR-task to generate material maps (header)
/// \since  18.02.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmGenerateMaterialMaps.h"

#include "CbmKfTarget.h"
#include "CbmTrackingDetectorInterfaceBase.h"
#include "CbmTrackingDetectorInterfaceInit.h"
#include "KfRootUtils.h"
#include "TFile.h"
#include "TH2F.h"

// ---------------------------------------------------------------------------------------------------------------------
//
CbmGenerateMaterialMaps::CbmGenerateMaterialMaps() : CbmGenerateMaterialMaps("CbmGenerateMaterialMaps", 0) {}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmGenerateMaterialMaps::CbmGenerateMaterialMaps(const char* name, int verbose) : FairTask(name, verbose) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmGenerateMaterialMaps::Finish() {}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmGenerateMaterialMaps::Init()
{
  // ----- Init configuration
  if (!fsUserConfig.empty()) {
    fConfig = cbm::algo::yaml::ReadFromFile<Config>(fsUserConfig);
  }
  else {
    LOG(info) << fName << ": configuration file was not provided. Using default settings";
  }

  fTargetZ = cbm::kf::Target::Instance()->GetZ();

  fpMaterialFactory = std::make_unique<::kf::tools::MaterialMapFactory>();
  if (!fConfig.fbParallelRays) {
    fpMaterialFactory->SetDoRadialProjection(fTargetZ);
  }
  fpMaterialFactory->SetSafeMaterialInitialization(true);
  fpMaterialFactory->SetNraysPerDim(fConfig.fNofRays);

  if (fConfig.fbTrackingStations) {
    std::set<MaterialSlice> mSlice;
    // Loop over stations
    auto* pIfs = CbmTrackingDetectorInterfaceInit::Instance();
    if (pIfs) {
      for (const auto* pIf : pIfs->GetActiveInterfaces()) {
        auto detName = pIf->GetDetectorName();
        LOG(info) << fName << ": Generating material budget map for " << detName;
        int nSt = pIf->GetNtrackingStations();
        for (int iSt = 0; iSt < nSt; ++iSt) {
          MaterialSlice slice;
          slice.fName  = detName + Form("_station_%d", iSt);
          slice.fRefZ  = pIf->GetZref(iSt);
          slice.fMinZ  = pIf->GetZmin(iSt);
          slice.fMaxZ  = pIf->GetZmax(iSt);
          slice.fMaxXY = std::max(std::fabs(pIf->GetXmax(iSt)), std::fabs(pIf->GetYmax(iSt)));
          mSlice.insert(slice);
        }
      }
      double zLast = fTargetZ + 1.;
      // Run material helper
      for (auto it = mSlice.begin(); it != mSlice.end(); ++it) {
        LOG(info) << "Creating material map for " << it->fName;
        double z1   = it->fMaxZ;
        double z2   = z1;
        auto itNext = std::next(it, 1);
        if (itNext != mSlice.end()) {
          z2 = itNext->fMinZ;
        }
        double zRef  = it->fRefZ;
        double zNew  = 0.5 * (z1 + z2);
        double xyMax = kXYoffset * it->fMaxXY;
        int nBins    = static_cast<int>(std::ceil(2. * xyMax / fConfig.fPitch));
        if (nBins < 1) {
          LOG(fatal) << fName << ": selected pitch " << fConfig.fPitch << " gives wrong number of bins = " << nBins;
        }
        if (nBins > fConfig.fMaxNofBins) {
          nBins = fConfig.fMaxNofBins;
        }
        fmMaterial[it->fName] = std::move(fpMaterialFactory->GenerateMaterialMap(zRef, zLast, zNew, xyMax, nBins));
        zLast                 = zNew;
      }
    }
    else {
      LOG(error) << fName << ": tracking detector interfaces are not defined, so the materials maps cannot be "
                 << "generated for the tracking stations";
    }
  }

  // Loop over user-defined slices
  for (const auto& slice : fConfig.fvUserSlices) {
    LOG(info) << "Creating material map for " << slice.fName;
    if (fmMaterial.find(slice.fName) != fmMaterial.end()) {
      LOG(warn) << fName << ": Material for slice " << slice.fName << " was already prepared. "
                << "Please, check your configuration file";
      continue;
    }
    double xyMax = kXYoffset * slice.fMaxXY;
    double zMin  = slice.fMinZ;
    double zMax  = slice.fMaxZ;
    double zRef  = slice.fRefZ;

    int nBins = static_cast<int>(std::ceil(2. * xyMax / fConfig.fPitch));

    if (nBins < 1) {
      LOG(fatal) << fName << ": selected pitch " << fConfig.fPitch << " gives wrong number of bins = " << nBins;
    }
    if (nBins > fConfig.fMaxNofBins) {
      nBins = fConfig.fMaxNofBins;
    }
    fmMaterial[slice.fName] = fpMaterialFactory->GenerateMaterialMap(zRef, zMin, zMax, xyMax, nBins);
  }

  WriteMaterialMaps();
  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmGenerateMaterialMaps::ReInit() { return kSUCCESS; }

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmGenerateMaterialMaps::WriteMaterialMaps()
{
  auto f = TFile(fsOutputFile.c_str(), "RECREATE");
  f.cd();
  for (const auto& [name, material] : fmMaterial) {
    double zMin       = material.GetZmin();
    double zMax       = material.GetZmax();
    std::string title = Form("Material thickness in X_{0}, z region [%.2f,%.2f] cm (%s)", zMin, zMax, name.c_str());
    if (fConfig.fbParallelRays) {
      title += "; horizontal projection";
    }
    else {
      title += Form("; radial projection from z=%f cm", fTargetZ);
    }
    auto* h = ::kf::tools::RootUtils::ToHistogram(material, name.c_str(), title.c_str());
    h->SetDirectory(&f);
  }
  f.Write();
}
