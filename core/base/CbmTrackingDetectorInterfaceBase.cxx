/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// @file   CbmTrackingDetectorInterfaceBase.h
/// @brief  Base abstract class for tracking detector interface to L1 (implementation of Checker)
/// @since  31.05.2022
/// @author S.Zharko (s.zharko@gsi.de)

#include "CbmTrackingDetectorInterfaceBase.h"

#include <Logger.h>

#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TString.h>

#include <iomanip>
#include <limits>
#include <set>
#include <vector>

#include <fmt/format.h>

// ---------------------------------------------------------------------------------------------------------------------
//
bool CbmTrackingDetectorInterfaceBase::Check() const
{
  bool res = true;
  LOG(info) << ToString();
  std::stringstream msg;
  msg << "Errors in the detector interface initialization for " << this->GetDetectorName() << ":\n";

  // Number of stations
  if (this->GetNtrackingStations() < 1) {
    msg << "\t- Number of stations is less then 1(" << this->GetNtrackingStations() << ")";
    res = false && res;
  }
  else {
    // Station individual parameters check
    for (int iSt = 0; iSt < this->GetNtrackingStations(); ++iSt) {
      std::string prefix = std::string("\t- Station ") + std::to_string(iSt) + " has ";

      {  // Position along Z-axis
        double z0 = this->GetZmin(iSt);
        double z1 = this->GetZref(iSt);
        double z2 = this->GetZmax(iSt);
        if (!std::isfinite(z0) || !std::isfinite(z1) || !std::isfinite(z2) || !(z0 < z1 && z1 < z2)) {
          msg << prefix << " wrong Z position (" << z0 << " < " << z1 << " < " << z2 << " cm)\n";
          res = false && res;
        }
      }

      {  // Size along X-axis
        auto xMax = this->GetXmax(iSt);
        auto xMin = this->GetXmin(iSt);
        if (!(xMax > xMin) || std::isnan(xMax) || std::isnan(xMin)) {
          msg << prefix << " zero, negative or NaN X-size (xMin = " << xMin << ", xMax = " << xMax << " cm)\n";
          res = false && res;
        }
      }

      {  // Size along Y-axis
        auto yMax = this->GetYmax(iSt);
        auto yMin = this->GetYmin(iSt);
        if (!(yMax > yMin) || std::isnan(yMax) || std::isnan(yMin)) {
          msg << prefix << " zero, negative or NaN Y-size (yMin = " << yMin << ", yMax = " << yMax << " cm)\n";
          res = false && res;
        }
      }
    }

    {  // Position along beam axis
      std::vector<double> zPositions(this->GetNtrackingStations());
      for (int iSt = 0; iSt < this->GetNtrackingStations(); ++iSt) {
        zPositions[iSt] = this->GetZref(iSt);
      }
      std::set<double> zPositionSet(zPositions.begin(), zPositions.end());
      if (zPositions.size() != zPositionSet.size()) {
        msg << "\t- Some of stations have the same z position component:\n";
        for (size_t iSt = 0; iSt < zPositions.size(); ++iSt) {
          msg << "\t\tstation " << iSt << ", z = " << zPositions[iSt] << " cm\n";
        }
        res = false && res;
      }
    }
  }

  if (!res) {
    LOG(error) << msg.str()
               << "\033[4mErrors above mean that the CA tracking cannot be used with the current version of "
               << this->GetDetectorName() << " setup. Please, check if the " << this->GetDetectorName()
               << " setup parameters and the corresponding tracking detector interface are initialized properly\033[0m";
  }

  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string CbmTrackingDetectorInterfaceBase::ToString() const
{
  // TODO: Add verbosity level, probably distribute properties into several tables
  using fmt::format;
  std::stringstream table;
  table << format(
    "\n|{:>5}|{:>9}|{:>17}|{:>17}|{:>17}|{:>17}|{:>17}|{:>17}|{:>9}|{:>9}|{:>9}|{:>9}|{:>9}|{:>9}|{:>9}|\n", "st.No",
    "z_ref[cm]", "x_min(active)[cm]", "x_max(active)[cm]", "y_min(active)[cm]", "y_max(active)[cm]",
    "z_min(active)[cm]", "z_max(active)[cm]", "x_min[cm]", "x_max[cm]", "y_min[cm]", "y_max[cm]", "z_min[cm]",
    "z_max[cm]", "time info");
  table << format("|{0:->5}|{0:->9}|{0:->17}|{0:->17}|{0:->17}|{0:->17}|{0:->17}|{0:->17}|{0:->9}|{0:->9}"
                  "|{0:->9}|{0:->9}|{0:->9}|{0:->9}|{0:->9}|\n",
                  "");
  for (int iSt = 0; iSt < GetNtrackingStations(); ++iSt) {
    table << format("|{:>5d}|{:>9.2f}|{:>17.2f}|{:>17.2f}|{:>17.2f}|{:>17.2f}|{:>17.2f}|{:>17.2f}|{:>9.2f}|{:>9.2f}|{:>"
                    "9.2f}|{:>9.2f}|{:>9.2f}|"
                    "{:>9.2f}|{:>9}|\n",
                    iSt, GetZref(iSt), GetActiveXmin(iSt), GetActiveXmax(iSt), GetActiveYmin(iSt), GetActiveYmax(iSt),
                    GetActiveZmin(iSt), GetActiveZmax(iSt), GetXmin(iSt), GetXmax(iSt), GetYmin(iSt), GetYmax(iSt),
                    GetZmin(iSt), GetZmax(iSt), IsTimeInfoProvided(iSt));
  }
  return table.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::vector<TString> CbmTrackingDetectorInterfaceBase::CollectNodes(const TString& detector, const TString& component,
                                                                    const TString& path, TGeoNode* pNode)
{
  std::vector<TString> vPaths;

  if (!pNode) {
    return vPaths;
  }

  TString nodePath = path + (path.IsNull() ? "" : "/") + pNode->GetName();

  if (TString(pNode->GetName()).Contains(component, TString::kIgnoreCase)) {
    if (nodePath.Contains(detector, TString::kIgnoreCase)) {
      vPaths.push_back(nodePath);
    }
  }

  for (int iNode = 0; iNode < pNode->GetNdaughters(); ++iNode) {
    TGeoNode* pDaughter = pNode->GetDaughter(iNode);
    auto daughterPaths  = CollectNodes(detector, component, nodePath, pDaughter);
    vPaths.insert(vPaths.end(), daughterPaths.begin(), daughterPaths.end());
  }
  return vPaths;
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmTrackingDetectorInterfaceBase::VolumeInfo CbmTrackingDetectorInterfaceBase::ReadVolume(const TString& path)
{
  VolumeInfo res;
  if (!gGeoManager->cd(path)) {
    std::stringstream msg;
    msg << "Node with path " << path << " is not found in the geo manager";
    throw std::runtime_error(msg.str());
  }

  auto* matrix{gGeoManager->GetCurrentMatrix()};
  const double* translation{matrix->GetTranslation()};
  auto* volume{gGeoManager->GetCurrentVolume()};
  auto* shape{volume->GetShape()};
  auto GetDimension = [&](int iD) {
    double min;
    double max;
    shape->GetAxisRange(iD + 1, min, max);
    return std::make_pair<double, double>(min + translation[iD], max + translation[iD]);
  };
  std::tie(res.fXmin, res.fXmax) = GetDimension(0);
  std::tie(res.fYmin, res.fYmax) = GetDimension(1);
  std::tie(res.fZmin, res.fZmax) = GetDimension(2);
  return res;
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string CbmTrackingDetectorInterfaceBase::VolumeInfo::ToString() const
{
  std::stringstream msg;
  msg << "zRef = " << 0.5 * (fZmin + fZmax) << "zMin = " << fZmin << ", zMax = " << fZmax << ", xMin = " << fXmin
      << ", xMax = " << fXmax << ", yMin = " << fYmin << ", yMax = " << fYmax;
  return msg.str();
}
