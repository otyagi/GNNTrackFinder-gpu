/* Copyright (C) 2021-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file CaGpuParameters.h
/// \brief Parameter container for the GPU CA library

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaGpuField.h"
#include "CaGpuMaterialMap.h"
#include "CaGpuStation.h"
#include "CaMeasurementXy.h"
#include "CaParameters.h"

#include <xpu/device.h>

namespace cbm::algo::ca
{
  enum class EDetectorID;

  class GpuParameters {

    using DetectorID_t = std::underlying_type_t<EDetectorID>;

   public:
    /// \brief Default constructor
    GpuParameters() = default;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    GpuParameters(const Parameters<DataIn>& other)
      : maxSlopePV(0.1)
      , maxQp(0.1)
      , maxDZ(0.1)
      , particleMass(0.13957)
      , doubletChi2Cut(5)
      , tripletChi2Cut(5)
      , tripletFinalChi2Cut(5)
      , fNStations(0)
      , fMisalignmentX(other.GetMisalignmentX())
      , fMisalignmentY(other.GetMisalignmentY())
      , fMisalignmentT(other.GetMisalignmentT())
      , primaryFlag(true)
      , isTargetField(false)
    {
      fTargetPos[0] = kf::utils::simd::Cast<DataIn, float>(other.GetTargetPositionX());
      fTargetPos[1] = kf::utils::simd::Cast<DataIn, float>(other.GetTargetPositionY());
      fTargetPos[2] = kf::utils::simd::Cast<DataIn, float>(other.GetTargetPositionZ());
      for (size_t i = 0; i < (size_t) other.GetNstationsActive(); i++) {
        fStations[i]                 = other.GetStation(i);
        auto [detSystemId, iStLocal] = other.GetActiveSetup().GetIndexMap().template GlobalToLocal<EDetectorID>(i);
        fStations[i].fDetectorID     = detSystemId;
      }
    }

    /// \brief Destructor
    ~GpuParameters() = default;

   public:
    ca::GpuFieldValue targB;                     ///< Magnetic field in the target region
    ca::MeasurementXy<float> targetMeasurement;  ///< Measurement XY in the target region

    /// \brief Gets Station
    XPU_D const ca::GpuStation& GetStation(int iStation) const { return fStations[iStation]; }

    /// \brief Sets Station
    void SetStation(int iStation, const ca::GpuStation& station) { fStations[iStation] = station; }

    /// \brief Gets X component of target position
    XPU_D float GetTargetPositionX() const { return fTargetPos[0]; }

    /// \brief Gets Y component of target position
    XPU_D float GetTargetPositionY() const { return fTargetPos[1]; }

    /// \brief Gets Z component of target position
    XPU_D float GetTargetPositionZ() const { return fTargetPos[2]; }

    /// \brief Gets misalignment of the detector systems in X, squared rms
    XPU_D float GetMisalignmentXsq(EDetectorID detId) const
    {
      auto iDet = static_cast<DetectorID_t>(detId);
      return fMisalignmentX[iDet] * fMisalignmentX[iDet];
    }

    /// \brief Gets misalignment of the detector systems in Y, squared rms
    XPU_D float GetMisalignmentYsq(EDetectorID detId) const
    {
      auto iDet = static_cast<DetectorID_t>(detId);
      return fMisalignmentY[iDet] * fMisalignmentY[iDet];
    }

    /// \brief Gets miscalibration of the detector systems in Time, squared rms
    XPU_D float GetMisalignmentTsq(EDetectorID detId) const
    {
      auto iDet = static_cast<DetectorID_t>(detId);
      return fMisalignmentT[iDet] * fMisalignmentT[iDet];
    }

    std::array<ca::GpuStation, constants::gpu::MaxNofStations> fStations;  ///< Array of stations

    std::array<float, 3> fTargetPos;  ///< Target position

    float maxSlopePV;           ///< Max slope to primary vertex
    float maxQp;                ///< Max Q/p
    float maxDZ;                ///< Max DZ
    float particleMass;         ///< Particle mass
    float doubletChi2Cut;       ///< Doublet chi2 cut
    float tripletChi2Cut;       ///< Triplet chi2 cut
    float tripletFinalChi2Cut;  ///< Triplet final chi2 cut

    /// misalignment of the detector systems in X
    std::array<float, constants::size::MaxNdetectors> fMisalignmentX;

    /// misalignment of the detector systems in Y
    std::array<float, constants::size::MaxNdetectors> fMisalignmentY;

    /// miscalibration of the detector systems in Time
    std::array<float, constants::size::MaxNdetectors> fMisalignmentT;

    size_t fNStations;  ///< Number of stations

    bool primaryFlag;    ///< Primary flag
    bool isTargetField;  ///< Is target field
  };
}  // namespace cbm::algo::ca
