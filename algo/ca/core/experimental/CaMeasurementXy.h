/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Sergei Zharko */

/// \file CaMeasurementXy.h
/// \brief Definition of the CaMeasurementXy class
/// Temporary class for compatibility with GPU code
/// Should be replaced by KfMeasurementXy

#pragma once  // include this header only once per compilation unit


#include "CaDefs.h"
#include "CaSimd.h"
#include "KfUtils.h"

#include <string>

#include <xpu/device.h>

namespace cbm::algo::ca
{

  /// \brief The class describes a 2D - measurement (x, y) in XY coordinate system
  ///
  /// The measurement has a finite resolution, i.e. the measurement is not a point, but a distribution
  /// with a certain rms.
  /// The measurement components may be used in the chi2 calculation or not
  /// The measurement may be a SIMD vector of values, when DataT is fvec type
  ///
  template<typename DataT>
  class MeasurementXy {

   public:
    friend class boost::serialization::access;

    /// default constructor
    XPU_D MeasurementXy() = default;

    /// constructor
    /// \param x        x coordinate of the measurement
    /// \param y        y coordinate of the measurement
    /// \param dx2      rms^2 of the x coordinate measurement
    /// \param dy2      rms^2 of the y coordinate measurement
    /// \param dxy      covariance of the x and y coordinate measurements
    /// \param ndfX     number of degrees of freedom for the x coordinate measurement
    ///                 if ndfX == 1, the measurement is used in fit and in the chi2 calculation
    ///                 if ndfX == 0, the measurement is used in fit, but not used in the chi2 calculation
    /// \param ndfY     number of degrees of freedom for the y coordinate measurement
    ///                 if ndfY == 1, the measurement is used in fit and in the chi2 calculation
    ///                 if ndfY == 0, the measurement is used in fit, but not used in the chi2 calculation
    XPU_D MeasurementXy(DataT x, DataT y, DataT dx2, DataT dy2, DataT dxy, DataT ndfX, DataT ndfY)
      : fX(x)
      , fY(y)
      , fDx2(dx2)
      , fDy2(dy2)
      , fDxy(dxy)
      , fNdfX(ndfX)
      , fNdfY(ndfY)
    {
    }


    /// Set all SIMD entries from all SIMD entries of the other class
    /// It works for scalar and fvec types,
    /// except of the case when DataT is scalar and TdataB is fvec.
    template<typename DataTb>
    void Set(const MeasurementXy<DataTb>& m)
    {
      CopyBase<DataTb, true, true>(0, m, 0);
    }

    /// Set all SIMD entries from one SIMD entry of the other class
    /// It also works when DataT is scalar
    void Set(const MeasurementXy<fvec>& m, const int im) { CopyBase<fvec, true, false>(0, m, im); }

    /// Set one SIMD entry from one SIMD entry of the other class
    /// It only works when DataT is fvec, TdataB is scalar
    template<typename DataTb>
    void SetOneEntry(const int i, const MeasurementXy<DataTb>& m)
    {
      CopyBase<DataTb, false, true>(i, m, 0);
    }

    /// Set one SIMD entry from one SIMD entry of the other class
    /// It only works when DataT is fvec, TdataB is fvec
    void SetOneEntry(const int i, const MeasurementXy<fvec>& m, const int im)
    {
      CopyBase<fvec, false, false>(i, m, im);
    }


    ///------------------------------
    /// Setters and getters

    XPU_D void SetX(DataT x) { fX = x; }
    XPU_D void SetY(DataT y) { fY = y; }
    XPU_D void SetDx2(DataT dx2) { fDx2 = dx2; }
    XPU_D void SetDy2(DataT dy2) { fDy2 = dy2; }
    XPU_D void SetDxy(DataT dxy) { fDxy = dxy; }
    XPU_D void SetNdfX(DataT ndfX) { fNdfX = ndfX; }
    XPU_D void SetNdfY(DataT ndfY) { fNdfY = ndfY; }
    XPU_D void SetCov(DataT dx2, DataT dxy, DataT dy2)
    {
      fDx2 = dx2;
      fDxy = dxy;
      fDy2 = dy2;
    }

    XPU_D DataT X() const { return fX; }
    XPU_D DataT Y() const { return fY; }
    XPU_D DataT Dx2() const { return fDx2; }
    XPU_D DataT Dy2() const { return fDy2; }
    XPU_D DataT Dxy() const { return fDxy; }
    XPU_D DataT NdfX() const { return fNdfX; }
    XPU_D DataT NdfY() const { return fNdfY; }

    ///------------------------------
    /// references, to ease assignment to SIMD vector components when DataT has fvec type

    XPU_D DataT& X() { return fX; }
    XPU_D DataT& Y() { return fY; }
    XPU_D DataT& Dx2() { return fDx2; }
    XPU_D DataT& Dy2() { return fDy2; }
    XPU_D DataT& Dxy() { return fDxy; }
    XPU_D DataT& NdfX() { return fNdfX; }
    XPU_D DataT& NdfY() { return fNdfY; }

    ///------------------------------
    /// Methods for debugging

    /// Checks, if all fields are finite
    bool IsFinite() const
    {
      return (kf::utils::IsFinite(X()) && kf::utils::IsFinite(Y()) && kf::utils::IsFinite(Dx2())
              && kf::utils::IsFinite(Dy2()) && kf::utils::IsFinite(Dxy()) && kf::utils::IsFinite(NdfX())
              && kf::utils::IsFinite(NdfY()));
    }

    /// Checks, if some fields are undefined
    bool IsUndefined() const
    {
      return (kf::utils::IsUndefined(X()) || kf::utils::IsUndefined(Y()) || kf::utils::IsUndefined(Dx2())
              || kf::utils::IsUndefined(Dy2()) || kf::utils::IsUndefined(Dxy()) || kf::utils::IsUndefined(NdfX())
              || kf::utils::IsUndefined(NdfY()));
    }

   private:
    /// \brief Copies all/one entries from the other class
    /// \tparam TdataB  Type of the other class
    /// \tparam TDoAllA  If true, all entries of the current class must be set
    /// \tparam TDoAllB  If true, all entries of the other class must be used
    /// \param ia  Index of SIMD vector element of the current class
    /// \param Tb  Other class
    /// \param ib  Index of SIMD vector element of the other class
    template<typename TdataB, bool TDoAllA, bool TDoAllB>
    void CopyBase(const int ia, const MeasurementXy<TdataB>& Tb, const int ib);

   private:
    ///------------------------------
    /// Data members

    // Initializing parameters with NANs spoils the track fit where
    // the masked-out SIMD entries are suppressed by a multication by 0.
    // Therefore, we initialize the data members here with finite numbers.
    // For the numerical safety, with some reasonable numbers.

    DataT fX;    ///< x coordinate of the measurement
    DataT fY;    ///< y coordinate of the measurement
    DataT fDx2;  ///< rms^2 of the x coordinate measurement
    DataT fDy2;  ///< rms^2 of the y coordinate measurement
    DataT fDxy;  ///< covariance of the x and y coordinate measurements

    /// number of degrees of freedom (used for chi2 calculation)
    /// if ndf == 1, the measurement is used in the chi2 calculation
    /// if ndf == 0, the measurement is not used in the chi2 calculation

    DataT fNdfX;  ///< ndf for the x coordinate measurement
    DataT fNdfY;  ///< ndf for the y coordinate measurement
  };


  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename TdataA>
  template<typename TdataB, bool TDoAllA, bool TDoAllB>
  inline void MeasurementXy<TdataA>::CopyBase(const int ia, const MeasurementXy<TdataB>& Tb, const int ib)
  {
    auto copy = [&](TdataA& a, const TdataB& b) {
      kf::utils::VecCopy<TdataA, TdataB, TDoAllA, TDoAllB>::CopyEntries(a, ia, b, ib);
    };

    copy(fX, Tb.X());
    copy(fY, Tb.Y());
    copy(fDx2, Tb.Dx2());
    copy(fDy2, Tb.Dy2());
    copy(fDxy, Tb.Dxy());
    copy(fNdfX, Tb.NdfX());
    copy(fNdfY, Tb.NdfY());
  }  // CopyBase

}  // namespace cbm::algo::ca
