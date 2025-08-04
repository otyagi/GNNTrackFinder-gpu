/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfIMaterialMapFactory.h
/// \brief  Interface to the material map creator
/// \since  28.08.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

namespace cbm::algo::kf
{
  class MaterialMap;

  /// \class IMaterialMapFactory
  /// \brief Interface to the material map creator
  class IMaterialMapFactory {
   public:
    /// \brief Destructor
    virtual ~IMaterialMapFactory() = default;

    /// \brief Generates a material budget map
    /// \param zRef      Reference z-coordinate of the material layer [cm]
    /// \param zMin      z-coordinate of the material layer lower boundary [cm]
    /// \param zMax      z-coordinate of the material layer upper boundary [cm]
    /// \param xyMax     Transverse size of the material layer [cm]
    /// \param nBinsDim  Number of bins in the x(y) axis
    ///
    /// generate a material map, collecting the material between [zMin, zMax]
    /// with radial rays from (0,0,targetZ) through the XY-bins at z == zRef.
    ///
    /// It creates a map with [nBinsDim x nBinsDim] bins, of a size of [+-xyMax, +-xyMax]
    /// shooting fNraysBinPerDim x fNraysBinPerDim through each bin
    ///
    /// The calculated radiation thickness is a projection of the rad.thickness along the ray onto the Z axis.
    /// RadThick = sum of (dZ / radLength) over ray trajectory pieces
    ///
    /// When doRadialProjection==false the rays are shoot horizontally along the Z axis
    ///
    virtual MaterialMap GenerateMaterialMap(double zRef, double zMin, double zMax, double xyMax, int nBinsDim) = 0;
  };
}  // namespace cbm::algo::kf
