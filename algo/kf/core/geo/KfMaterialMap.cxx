/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

#include "KfMaterialMap.h"

#include "AlgoFairloggerCompat.h"
#include "KfSimd.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

#include <fmt/format.h>

using cbm::algo::kf::fvec;
using cbm::algo::kf::MaterialMap;

// ---------------------------------------------------------------------------------------------------------------------
//
MaterialMap::MaterialMap(int nBins, float xyMax, float zRef, float zMin, float zMax)
  : fNbins(nBins)
  , fXYmax(xyMax)
  , fFactor(0.5 * fNbins / fXYmax)
  , fZref(zRef)
  , fZmin(zMin)
  , fZmax(zMax)
{
  this->CheckConsistency();
  fTable.resize(fNbins * fNbins);
}

// ---------------------------------------------------------------------------------------------------------------------
//
MaterialMap::MaterialMap(MaterialMap&& other) noexcept { this->Swap(other); }

// ---------------------------------------------------------------------------------------------------------------------
//
MaterialMap& MaterialMap::operator=(MaterialMap&& other) noexcept
{
  if (this != &other) {
    MaterialMap tmp(std::move(other));
    this->Swap(tmp);
  }
  return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MaterialMap::Add(const MaterialMap& other, float zTarg)
{
  // The function allows to add a material layer either from the left or from the right to the station
  // NOTE: A symmetry of x-y is assumed
  constexpr int nRays{3};                      // Number of rays in a bin in a dimension
  const bool bRadialRays{!std::isnan(zTarg)};  // Are rays radial (true, coming from target) or parallel (false)?
  const auto scaleFactor{bRadialRays ? ((other.fZref - zTarg) / (this->fZref - zTarg)) : 1.F};
  const auto binSize{2.F * scaleFactor * this->fXYmax / this->fNbins};  // Size of each bin dimension [cm]
  const auto stepSize{binSize / static_cast<float>(nRays)};             // Step between two neighboring rays [cm]

  // The coordinates of the first ray intersection with the other material layer [cm]
  float yBinOther{-this->fXYmax * scaleFactor + stepSize * 0.5F};

  // Loop over bins of the active (this)
  for (int iBinY{0}; iBinY < this->fNbins; ++iBinY) {
    float xBinOther{-this->fXYmax * scaleFactor + stepSize * 0.5F};
    for (int iBinX{0}; iBinX < this->fNbins; ++iBinX) {
      // Collect material using ray shooting
      float avgThickness{0};  // Collected average thickness
      for (int iRayY{0}; iRayY < nRays; ++iRayY) {
        for (int iRayX{0}; iRayX < nRays; ++iRayX) {
          avgThickness += other.GetThicknessX0(xBinOther + iRayX * stepSize, yBinOther + iRayY * stepSize);
        }
      }
      this->fTable[iBinX + this->fNbins * iBinY] += (avgThickness / (nRays * nRays));
      xBinOther += binSize;
    }
    yBinOther += binSize;
  }
  this->fZmin = std::min(this->fZmin, other.fZmin);
  this->fZmax = std::max(this->fZmax, other.fZmax);
}

// ---------------------------------------------------------------------------------------------------------------------
//
int MaterialMap::GetBin(float x, float y) const
{
  int i{static_cast<int>((x + fXYmax) * fFactor)};
  int j{static_cast<int>((y + fXYmax) * fFactor)};
  if (i < 0 || j < 0 || i >= fNbins || j >= fNbins) {
    return -1;
  }
  return i + j * fNbins;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MaterialMap::Rebin(int nGroups)
{
  if (nGroups < 1 && nGroups > fNbins) {
    int nGroupsSet{nGroups};
    nGroups = std::clamp(nGroups, 1, fNbins);
    LOG(warn) << "kf::MaterialMap::Rebin(): incorrect input parameter nGroups = " << nGroupsSet << " is set to "
              << nGroups;
  }

  int nBinsNew = static_cast<int>(static_cast<bool>(fNbins % nGroups)) + fNbins / nGroups;
  std::vector<float> table(nBinsNew * nBinsNew);
  for (int iX{0}; iX < nBinsNew; ++iX) {
    for (int iY{0}; iY < nBinsNew; ++iY) {
      float value{0.F};
      int nEntries{0};
      int iOldXmin{iX * nGroups};
      int iOldXmax{std::max((iX + 1) * nGroups, fNbins)};
      int iOldYmin{iY * nGroups};
      int iOldYmax{std::max((iY + 1) * nGroups, fNbins)};
      for (int iOldX{iOldXmin}; iOldX < iOldXmax; ++iOldX) {
        for (int iOldY{iOldYmin}; iOldY < iOldYmax; ++iOldY) {
          value += fTable[iOldX + fNbins * iOldY];
          ++nEntries;
        }
      }
      table[iX + nBinsNew * iY] = value / nEntries;
    }
  }
  fTable  = std::move(table);
  fFactor = fFactor / fNbins * nBinsNew;
  fNbins  = nBinsNew;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MaterialMap::Swap(MaterialMap& other) noexcept
{
  std::swap(fNbins, other.fNbins);
  std::swap(fXYmax, other.fXYmax);
  std::swap(fFactor, other.fFactor);
  std::swap(fZref, other.fZref);
  std::swap(fZmin, other.fZmin);
  std::swap(fZmax, other.fZmax);
  std::swap(fTable, other.fTable);  // Probably can cause segmentation violation (did not understand)
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string MaterialMap::ToString(int indentLevel, int verbose) const
{
  using fmt::format;
  using std::setw;
  std::stringstream msg;
  if (verbose > 0) {
    constexpr char indentCh = '\t';
    std::string indent(indentLevel, indentCh);
    msg << format("zRef = {:<12} cm, range [{:<12}, {:<12}] cm, nBins = {:<8}, XYmax = {:<12} cm", fZref, fZmin, fZmax,
                  fNbins, fXYmax);
    if (verbose > 1) {
      msg << indent << indentCh << "\nContent(rebinned to 10 bins):\n";
      msg.precision(3);
      auto mapTmp{*this};
      mapTmp.Rebin(fNbins / 10);
      msg << indent << indentCh << setw(15) << "y [cm] \\ x [cm]" << ' ';
      for (int iX{0}; iX < mapTmp.fNbins; ++iX) {
        float xRef{-mapTmp.fXYmax + (mapTmp.fXYmax * (2 * iX + 1)) / mapTmp.fNbins};
        msg << setw(10) << xRef << ' ';
      }
      msg << '\n';
      for (int iY{0}; iY < mapTmp.fNbins; ++iY) {
        float yRef{-mapTmp.fXYmax + (mapTmp.fXYmax * (2 * iY + 1)) / mapTmp.fNbins};
        msg << indent << indentCh << setw(15) << yRef << ' ';
        for (int iX{0}; iX < mapTmp.fNbins; ++iX) {
          msg << setw(10) << mapTmp.fTable[iX + mapTmp.fNbins * iY] << ' ';
        }
        msg << '\n';
      }
    }
  }
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MaterialMap::CheckConsistency() const
{
  if (fNbins < 1) {
    std::stringstream aStream;
    aStream << "MaterialMap: object cannot be initialized with non-positive nBins = " << fNbins;
    throw std::logic_error(aStream.str());
  }

  if (fXYmax < 0.) {
    std::stringstream aStream;
    aStream << "MaterialMap: object cannot be initialized with non-positive XYmax = " << fXYmax << " [cm]";
    throw std::logic_error(aStream.str());
  }

  if (!((fZmin <= fZref) && (fZref <= fZmax))) {
    std::stringstream aStream;
    aStream << "MaterialMap: object cannot be initialized with inconsistent Z: min " << fZmin << " ref " << fZref
            << " max " << fZmax << " [cm]";
    throw std::logic_error(aStream.str());
  }
}
