/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfMaterialMapFactory.h
/// \brief  Utility to generate material budget map from the TGeoNavigator representation of the Setup (implementation)
/// \author Sergey Gorbunov <se.gorbunov@gsi.de>
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \date   29.08.2024

#pragma once

#include "KfIMaterialMapFactory.h"
#include "Rtypes.h"
#include "TObject.h"

class TGeoNavigator;
namespace cbm::algo::kf
{
  class MaterialMap;
}

namespace kf::tools
{
  /// \class MaterialMapFactory
  /// \brief An utility class to create a material budget map from the TGeo
  class MaterialMapFactory : public cbm::algo::kf::IMaterialMapFactory {
   public:
    /// \brief Constructor from parameters
    /// \param verbose  Verbosity level (0 - only warning/error output)
    MaterialMapFactory(int verbose = 0);

    /// \brief Destructor
    ~MaterialMapFactory();

    /// \brief Project rays radially from the targetZ througth the XY-bins at a reference z.
    /// \param targetZ  z-coordinate of the target [cm]
    void SetDoRadialProjection(double targetZ)
    {
      fDoRadialProjection = true;
      fTargetZ            = targetZ;
    }

    /// \brief Project rays horisontally along the Z axis (default)
    void SetDoHorisontalProjection() { fDoRadialProjection = false; }

    /// \brief Shoots nRaysDim * nRaysDim rays for each bin in the map
    void SetNraysPerDim(int nRaysDim) { fNraysBinPerDim = (nRaysDim > 0) ? nRaysDim : 1; }


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
    cbm::algo::kf::MaterialMap GenerateMaterialMap(double zRef, double zMin, double zMax, double xyMax,
                                                   int nBinsDim) override;

    /// \brief Enables safe mode of the material initialization
    void SetSafeMaterialInitialization(bool val = true) { fDoSafeInitialization = val; }

   private:
    /// \brief Initializes the necessary amount of threads in TGeoManager
    void InitThreads();

    /// \brief Cleans up the TGeoManager: threadIds, create a default navigator
    void CleanUpThreads();

    /// \brief Gets navigator for current thread, creates it if it does not exist
    /// \param iThread  Index of the thread
    /// \throw std::runtime_error  If the TGeoNavigator is not found
    TGeoNavigator* GetCurrentNavigator(int iThread);

   private:
    static constexpr double kMinRadLength = 0.3;  ///< Minimal radiational length allowed [cm]

    std::vector<TGeoNavigator*> fNavigators{};  ///< list of created navigators
    double fTargetZ{0.};                        ///< z of the target for the radial projection
    int fNthreadsOld{0};                        ///< number of threads in TGeoManager before the helper was created
    int fNthreads{0};                           ///< number of threads
    int fNraysBinPerDim{3};                     ///< shoot fNraysBinPerDim * fNraysBinPerDim rays in each map bin
    int fVerbose{0};                            ///< verbosity level
    bool fDoRadialProjection{false};            ///< if project rays horizontally along the Z axis (special mode)
    bool fDoSafeInitialization{false};          ///< performs slow but safe initialization
                                                ///< to get around the crashes in TGeoVoxelFinder
  };

}  // namespace kf::tools
