/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov[committer] */

/// \file   CbmKfOriginalField.h
/// \brief  Thread-safe representation of the magnetic field (header)
/// \since  17.10.2023
/// \author S.Gorbunov

#pragma once  // include this header only once per compilation unit

#include "FairField.h"
#include "FairRunAna.h"

#include <mutex>
#include <tuple>

namespace cbm::kf
{
  /// \class OriginalField
  /// \brief Thread-safe representation of the magnetic field in CBM
  class OriginalField {
   public:
    /// \brief Returns magnetic field flux density in a spatial point
    /// \param x  x-coordinate of the point [cm]
    /// \param y  y-coordinate of the point [cm]
    /// \param z  z-coordinate of the point [cm]
    /// \return  tuple(Bx, By, Bz) of the magnetic field flux density components [kG]
    std::tuple<double, double, double> operator()(double x, double y, double z) const
    {
      assert(FairRunAna::Instance());
      assert(FairRunAna::Instance()->GetField());
      double pos[3] = {x, y, z};
      double B[3]   = {0., 0., 0.};
      // protect the field access
      // TODO: make CbmField thread-safe
      static std::mutex mymutex;
      mymutex.lock();
      FairRunAna::Instance()->GetField()->GetFieldValue(pos, B);
      mymutex.unlock();
      return std::tuple(B[0], B[1], B[2]);
    }
  };

  /// \class ZeroField
  class ZeroField {
   public:
    /// \brief Returns magnetic field flux density in a spatial point
    /// \param x  x-coordinate of the point [cm]
    /// \param y  y-coordinate of the point [cm]
    /// \param z  z-coordinate of the point [cm]
    /// \return  tuple(Bx, By, Bz) of the magnetic field flux density components [kG]
    std::tuple<double, double, double> operator()(double, double, double) const { return std::tuple(0., 0., 0.); }
  };
}  // namespace cbm::kf
