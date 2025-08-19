/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Igor Kulakov, Sergei Zharko, Grigory Kozlov [committer] */

/// \file GpuStation.h
///
/// \brief Contains a set of geometry parameters for a particular station
///
/// A temporary simplified version of the CaStation class, intended for compatibility with GPU code

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaGpuField.h"
#include "CaStation.h"

#include <xpu/device.h>

namespace cbm::algo::ca
{
  enum class EDetectorID;

  /// Structure Station
  /// Contains a set of geometry parameters for a particular station
  ///
  class GpuStation {
   public:
    int type;         //  TODO: replace with L1DetectorID
    int timeInfo;     ///< flag: if time information can be used
    int fieldStatus;  ///< flag: 1 - station is INSIDE the field, 0 - station is OUTSIDE the field
    float fZ;         ///< z position of station     [cm]
    float Xmax;       ///< min radius of the station [cm]
    float Ymax;       ///< max radius of the station [cm]
    EDetectorID fDetectorID;

    GpuFieldSlice fieldSlice;  ///< Magnetic field near the station

    GpuStation() = default;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    GpuStation(const Station<DataIn>& other)
      : type(other.GetType())
      , timeInfo(other.GetTimeStatus())
      , fieldStatus(other.GetFieldStatus())
      , fZ(other.template GetZ<float>())
      , Xmax(other.template GetXmax<float>())
      , Ymax(other.template GetYmax<float>())
      , fieldSlice(other.fieldSlice)
    {
    }

    /// \brief Gets type of the station
    int GetType() const { return type; }

    /// \brief Gets time-measurement flag
    int GetTimeStatus() const { return timeInfo; }

    /// \brief Gets field status flag
    int GetFieldStatus() const { return fieldStatus; }

    /// \brief Gets z-position of the station
    XPU_D float GetZ() const { return fZ; }

    /// \brief Gets limit of the station size in x-axis direction
    XPU_D float GetXmax() const { return Xmax; }

    /// \brief Gets limit of the station size in x-axis direction
    XPU_D float GetXmin() const { return -Xmax; }

    /// \brief Gets limit of the station size in y-axis direction
    XPU_D float GetYmax() const { return Ymax; }

    /// \brief Gets limit of the station size in y-axis direction
    XPU_D float GetYmin() const { return -Ymax; }

    /// \brief Gets detectorID
    XPU_D EDetectorID GetDetectorID() const { return fDetectorID; }
  };

}  // namespace cbm::algo::ca
