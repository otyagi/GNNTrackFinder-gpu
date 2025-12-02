/* Copyright (C) 2007-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina, Igor Kulakov, Sergey Gorbunov [committer], Maksym Zyzak */

/// \file  CaHit.h
/// \brief A generic hit for the CA tracker (header)
/// \date  2007-2023

#pragma once  // include this header only once per compilation unit

#include "CaSimd.h"

#include <boost/serialization/access.hpp>

#include <string>

#include <xpu/device.h>

namespace cbm::algo::ca
{
  struct CaHitTimeInfo {
    fscal fEventTimeMin{-std::numeric_limits<fscal>::max() / 2.};
    fscal fEventTimeMax{std::numeric_limits<fscal>::max() / 2.};
    fscal fMaxTimeBeforeHit{0.};  //< max event time for hits [0 .. hit] in the station hit array
    fscal fMinTimeAfterHit{0.};   //< min event time for hits [hit ... ] in the station hit array
  };

  // FIXME: Move typedefs to another header (potential problems with dependencies)
  using HitIndex_t    = unsigned int;  ///< Index of ca::Hit
  using HitKeyIndex_t = unsigned int;  ///< Index of the hit key (e.g. front / back cluster id for STS)

  /// \brief ca::Hit class describes a generic hit for the CA tracker
  ///
  class Hit {
    friend class boost::serialization::access;

   public:
    /// Default constructor
    Hit() = default;

    /// \brief Checks, if the hit is defined
    /// \return true  Hit is defined
    /// \return false Hit is undefined and will be skipped
    bool Check() const;

    ///-----------------------------------------------------------------------------
    /// setters

    /// Set the front key index
    void SetFrontKey(HitKeyIndex_t key) { fFrontKey = key; }

    /// Set the back key index
    void SetBackKey(HitKeyIndex_t key) { fBackKey = key; }

    /// Set the X coordinate
    void SetX(fscal x) { fX = x; }

    /// Set the Y coordinate
    void SetY(fscal y) { fY = y; }

    /// Set the Z coordinate
    void SetZ(fscal z) { fZ = z; }

    /// Set the time
    void SetT(fscal t) { fT = t; }

    /// Set the uncertainty of X coordinate
    void SetDx2(fscal dx2) { fDx2 = dx2; }

    /// Set the uncertainty of Y coordinate
    void SetDy2(fscal dy2) { fDy2 = dy2; }

    /// Set the X/Y covariance
    void SetDxy(fscal dxy) { fDxy = dxy; }

    /// Set the uncertainty of time
    void SetDt2(fscal dt2) { fDt2 = dt2; }

    /// Set the +/- range of uncertainty of X coordinate
    void SetRangeX(fscal rangeX) { fRangeX = rangeX; }

    /// Set the +/- range of uncertainty of Y coordinate
    void SetRangeY(fscal rangeY) { fRangeY = rangeY; }

    /// Set the +/- range of uncertainty of time
    void SetRangeT(fscal rangeT) { fRangeT = rangeT; }

    /// Set the hit id
    void SetId(HitIndex_t id) { fId = id; }

    /// Set the station index
    void SetStation(int station) { fStation = station; }

    ///-----------------------------------------------------------------------------
    /// getters

    /// Get the front key index
    XPU_D HitKeyIndex_t FrontKey() const { return fFrontKey; }

    /// Get the back key index
    XPU_D HitKeyIndex_t BackKey() const { return fBackKey; }

    /// Get the X coordinate
    XPU_D fscal X() const { return fX; }

    /// Get the Y coordinate
    XPU_D fscal Y() const { return fY; }

    /// Get the Z coordinate
    XPU_D fscal Z() const { return fZ; }

    /// Get the time
    XPU_D fscal T() const { return fT; }

    /// Get the uncertainty of X coordinate
    XPU_D fscal dX2() const { return fDx2; }

    /// Get the uncertainty of Y coordinate
    XPU_D fscal dY2() const { return fDy2; }

    /// Get the X/Y covariance
    XPU_D fscal dXY() const { return fDxy; }

    /// Get the uncertainty of time
    XPU_D fscal dT2() const { return fDt2; }

    /// Get the +/- range of uncertainty of X coordinate
    XPU_D fscal RangeX() const { return fRangeX; }

    /// Get the +/- range of uncertainty of Y coordinate
    XPU_D fscal RangeY() const { return fRangeY; }

    /// Get the +/- range of uncertainty of time
    XPU_D fscal RangeT() const { return fRangeT; }

    /// Get the hit id
    XPU_D HitIndex_t Id() const { return fId; }

    /// Get the station index
    XPU_D int Station() const { return fStation; }

    /// \brief Simple string representation of the hit class
    std::string ToString() const;

    /// \brief String representation of the hit class
    /// \param verbose  Verbosity level
    /// \param bHeader  If true, prints the header
    std::string ToString(int verbose, bool bHeader) const;

   private:
    ///-----------------------------------------------------------------------------
    /// data members


    /// NOTE: For STS f and b correspond to the indexes of the front and back clusters of the hit in a dataset. For other
    ///       tracking detectors (MVD, MuCh, TRD, TOF) f == b and corresponds to the index of the hit. Indexes f and b
    ///       do not intersect between different detector stations.
    HitKeyIndex_t fFrontKey{0};  ///< front hit key index
    HitKeyIndex_t fBackKey{0};   ///< back hit key index

    fscal fX{0.};       ///< measured X coordinate [cm]
    fscal fY{0.};       ///< measured Y coordinate [cm]
    fscal fZ{0.};       ///< fixed Z coordinate [cm]
    fscal fT{0.};       ///< measured time [ns]
    fscal fDx2{0.};     ///< rms^2 of uncertainty of X coordinate [cm2]
    fscal fDy2{0.};     ///< rms^2 of uncertainty of Y coordinate [cm2]
    fscal fDxy{0.};     ///< X/Y covariance [cm2]
    fscal fDt2{0.};     ///< measured uncertainty of time [ns2]
    fscal fRangeX{0.};  ///< +/- range of uncertainty of X coordinate [cm]
    fscal fRangeY{0.};  ///< +/- range of uncertainty of Y coordinate [cm]
    fscal fRangeT{0.};  ///< +/- range of uncertainty of time [ns]

    HitIndex_t fId{0};  ///< id of the hit
    int fStation{-1};   ///< index of station in the active stations array

   private:
    /// Serialization method, used to save ca::Hit objects into binary or text file in a defined order
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fFrontKey;
      ar& fBackKey;
      ar& fX;
      ar& fY;
      ar& fZ;
      ar& fT;
      ar& fDx2;
      ar& fDy2;
      ar& fDxy;
      ar& fDt2;
      ar& fRangeX;
      ar& fRangeY;
      ar& fRangeT;
      ar& fId;
      ar& fStation;
    }
  };

  // -------------------------------------------------------------------------------------------------------------------
  //
  inline bool Hit::Check() const
  {
    bool res = true;
    res &= std::isfinite(fX);
    res &= std::isfinite(fY);
    res &= std::isfinite(fZ);
    res &= std::isfinite(fT);
    res &= std::isfinite(fDx2);
    res &= std::isfinite(fDy2);
    res &= std::isfinite(fDxy);
    res &= std::isfinite(fDt2);
    res &= (fDx2 || fDy2 || fDxy || fDt2);  // TODO: research
    res &= (fT < 1.e+9);
    return res;
  }

}  // namespace cbm::algo::ca
