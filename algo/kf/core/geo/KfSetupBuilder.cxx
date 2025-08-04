/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfSetupBuilder.h
/// @brief  A base KF-Setup initialization class (source)
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "KfSetupBuilder.h"

#include <sstream>

#include <fmt/format.h>

using cbm::algo::kf::SetupBuilder;

// ---------------------------------------------------------------------------------------------------------------------
//
void SetupBuilder::Init()
{
  LOG(info) << "kf::SetupBuilder initilization: ...";

  if (!fbIfFieldFunctionSet) {
    throw std::runtime_error("kf::SetupBuilder: no field is provided");
  }
  if (fbIfSetFromSetup) {
    LOG(info) << "kf::SetupBuilder initilization: done";
    fbReady = true;
    return;
  }
  else {
    if (fGeoLayers.size() == 0) {
      throw std::runtime_error("kf::SetupBuilder: no geometry layers initialized");
    }
    if (!fpMaterialMapFactory.get()) {
      throw std::runtime_error("kf::SetupBuilder: no material map factory provided");
    }
    if (!fbIfTargetSet) {
      throw std::runtime_error("kf::SetupBuilder: target properties were not set");
    }
  }

  bool bMaterialLoaded{false};
  if (!fsMaterialCacheFile.empty()) {
    bMaterialLoaded = this->LoadMaterial();
  }

  // Add material to the target:
  if (!bMaterialLoaded) {
    // Material budget
    double zMin{fTarget.GetZ() + kTargetCenterOffset};
    double zMax{fTarget.GetZ() + fTarget.GetDz() * kTargetMaterialOffset};
    double rMax{fTarget.GetR() * kTargetMaterialTransverseSizeMargin};
    fTarget.SetMaterial(
      fpMaterialMapFactory->GenerateMaterialMap(fTarget.GetZ(), zMin, zMax, rMax, kTargetMaterialMapNofBins));
  }

  // Init geometry layers
  {
    // Estimate acceptance slope
    double acceptanceSlope = -1.;  // arbitrary negative value
    for (const auto& layer : fGeoLayers) {
      double tCurr = std::max(std::fabs(layer.fXmax), std::fabs(layer.fYmax)) / (layer.fZref - fTarget.GetZ());
      assert(tCurr > 0);
      acceptanceSlope = std::max(acceptanceSlope, tCurr);
    }

    if (!bMaterialLoaded) {
      fvMaterial.clear();
      fvMaterial.reserve(fGeoLayers.size());
    }
    double zLast{fTarget.GetZ() + fTarget.GetDz() * kTargetMaterialOffset};
    for (auto layerIt = fGeoLayers.cbegin(); layerIt != fGeoLayers.cend(); ++layerIt) {
      double z1 = layerIt->fZmax;
      double z2 = z1;
      if (std::next(layerIt) != fGeoLayers.cend()) {
        z2 = std::next(layerIt)->fZmin;
      }
      double zNew{0.5 * (z1 + z2)};
      double xyMax{acceptanceSlope * (layerIt->fZref - fTarget.GetZ())};
      if (!bMaterialLoaded) {
        auto material{fpMaterialMapFactory->GenerateMaterialMap(layerIt->fZref, zLast, zNew, xyMax, fMatMapNofBins)};
        fvMaterial.push_back(std::move(material));
      }

      // Note: square material maps to follow the material budget map regions
      fFieldFactory.AddSliceReference(xyMax, xyMax, layerIt->fZref);
      fModuleIndexFactory.AddComponent<int>(layerIt->fDetID, layerIt->fLocID, layerIt->fZref);
      zLast = zNew;
    }
  }

  if (!bMaterialLoaded && !fsMaterialCacheFile.empty()) {
    this->StoreMaterial();
  }

  fbIfGeoLayersInit = true;
  fbReady           = true;
  LOG(info) << "kf::SetupBuilder initialization: done";
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string SetupBuilder::InitStatusMsg() const
{
  std::stringstream msg;
  msg << "kf::SetupBuilder initialization: failed:\n";
  msg << fmt::format("\n - {<30} {}", "target property set:", fbIfTargetSet);
  msg << fmt::format("\n - {<30} {}", "geo layers added:", fbIfGeoLayersInit);
  msg << fmt::format("\n - {<30} {}", "field function set:", fbIfFieldFunctionSet);
  msg << fmt::format("\n - {<30} {}", "set from setup:", fbIfSetFromSetup);
  msg << fmt::format("\n - {<30} {}", "material map creator set:", bool(fpMaterialMapFactory.get()));
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool SetupBuilder::LoadMaterial()
{
  std::ifstream ifs(fsMaterialCacheFile, std::ios::binary);
  if (!ifs) {  // File does not exist yet
    return false;
  }
  try {
    boost::archive::binary_iarchive ia(ifs);
    MaterialMap targetMat;
    size_t refHash;
    ia >> refHash;
    if (refHash != fGeoHash) {
      LOG(warn) << "kf::SetupBuilder::LoadMaterial: reference hash from input file \"" << fsMaterialCacheFile
                << "\" "
                   "diverges from one, obtained from the actual detector setup geometry. Material budget will be "
                   "re-generated, and a new file will be created";
      return false;
    }
    ia >> targetMat;
    ia >> fvMaterial;
    fTarget.SetMaterial(targetMat);
  }
  catch (const std::exception& err) {
    LOG(warn) << "kf::SetupBuilder::LoadMaterial: input file \"" << fsMaterialCacheFile
              << "\" has inconsistent format "
                 "or was corrupted. The material maps will be re-generated";
    return false;
  }
  LOG(info) << "kf::SetupBuilder::LoadMaterial: the material maps were loaded from cache file \"" << fsMaterialCacheFile
            << "\"";
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SetupBuilder::Reset()
{
  fbReady              = false;
  fbIfTargetSet        = false;
  fbIfGeoLayersInit    = false;
  fbIfFieldFunctionSet = false;
  fbIfSetFromSetup     = false;
  fFieldFactory.Reset();
  fModuleIndexFactory.Reset();
  fvMaterial.clear();
  fGeoLayers.clear();
  fpMaterialMapFactory = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SetupBuilder::SetTargetProperty(double x, double y, double z, double dz, double r)
{
  fbReady = false;
  fTarget.SetX(x);
  fTarget.SetY(y);
  fTarget.SetZ(z);
  fTarget.SetDz(dz);
  fTarget.SetR(r);
  fFieldFactory.SetTarget(x, y, z);
  fbIfTargetSet = true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SetupBuilder::Store(const Setup<double>& setup, const std::string& fileName)
{
  std::ofstream ofs(fileName, std::ios::binary);
  if (!ofs) {
    std::stringstream msg;
    msg << "kf::SetupBuilder::Store: failed opening file \"" << fileName << "\" to store the setup";
    throw std::runtime_error(msg.str());
  }
  boost::archive::binary_oarchive oa(ofs);
  oa << setup;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void SetupBuilder::StoreMaterial() const
{
  std::ofstream ofs(fsMaterialCacheFile, std::ios::binary);
  if (!ofs) {
    std::stringstream msg;
    msg << "kf::SetupBuilder::Store: failed opening cache file \"" << fsMaterialCacheFile << "\" to store the setup";
    throw std::runtime_error(msg.str());
  }
  boost::archive::binary_oarchive oa(ofs);
  oa << fGeoHash;
  oa << fTarget.GetMaterial();
  oa << fvMaterial;
}
