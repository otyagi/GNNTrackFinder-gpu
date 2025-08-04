/* Copyright (C) 2007-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov, Sergey Gorbunov, Andrey Lebedev, Sergei Zharko, Grigory Kozlov [committer] */

/// \file GpuMaterialMap.h
/// \brief A map of station thickness in units of radiation length (X0) to the specific point in XY plane
///
/// A simplified version of the KfMaterialMap class, intended for compatibility with GPU code

#pragma once  // include this header only once per compilation unit

#include "KfMaterialMap.h"

#include <xpu/device.h>

namespace cbm::algo::ca
{
  /// \class MaterialMap
  /// \brief A map of station thickness in units of radiation length (X0) to the specific point in XY plane
  class GpuMaterialMap {
   public:
    /// \brief Default constructor
    GpuMaterialMap() = default;

    /// \brief Copy constructor
    GpuMaterialMap(const kf::MaterialMap& other, int binStart)
      : fNbins(other.GetNbins())
      , fBinStart(binStart)
      , fXYmax(other.GetXYmax())
      , fZref(other.GetZref())
      , fZmin(other.GetZmin())
      , fZmax(other.GetZmax())
    {
      fFactor = 0.5 * fNbins / fXYmax;
    }

    /// \brief Destructor
    ~GpuMaterialMap() noexcept = default;

    /// \brief Gets number of bins (rows or columns) of the material table
    XPU_D int GetNbins() const { return fNbins; }

    /// \brief Gets radius in cm of the material table
    XPU_D float GetXYmax() const { return fXYmax; }

    /// \brief Gets reference Z of the material in cm
    XPU_D float GetZref() const { return fZref; }

    /// \brief Gets minimal Z of the collected material in cm
    XPU_D float GetZmin() const { return fZmin; }

    /// \brief Gets maximal Z of the collected material in cm
    XPU_D float GetZmax() const { return fZmax; }

    /// \brief Get bin index for (x,y). Returns -1 when outside of the map
    XPU_D int GetBin(float x, float y) const
    {
      int i = static_cast<int>((x + fXYmax) * fFactor);
      int j = static_cast<int>((y + fXYmax) * fFactor);
      if (i < 0 || j < 0 || i >= fNbins || j >= fNbins) {
        return -1;
      }
      //TODO
      return fBinStart + i + j * fNbins;
    }

   private:
    int fNbins;     ///< Number of rows (== N columns) in the material budget table
    int fBinStart;  ///< Start index of the material budget table
    float fXYmax;   ///< Size of the station in x and y dimensions [cm]
    float fFactor;  ///< Util. var. for the conversion of point coordinates to row/column id
    float fZref;    ///< Reference Z of the collected material [cm]
    float fZmin;    ///< Minimal Z of the collected material [cm]
    float fZmax;    ///< Minimal Z of the collected material [cm]
  };

}  // namespace cbm::algo::ca
