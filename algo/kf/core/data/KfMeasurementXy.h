/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Sergei Zharko */

/// \file KfMeasurementXy.h
/// \brief Definition of the KfMeasurementXy class

#pragma once  // include this header only once per compilation unit


#include "KfDefs.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>

#include <string>

namespace cbm::algo::kf
{
  /// \brief The class describes a 2D - measurement (x, y) in XY coordinate system
  ///
  /// The measurement has a finite resolution, i.e. the measurement is not a point, but a distribution
  /// with a certain rms.
  /// The measurement components may be used in the chi2 calculation or not
  /// The measurement may be a SIMD vector of values, when DataT is fvec type
  ///
  template<typename DataT>
  class alignas(VcMemAlign) MeasurementXy {
   public:
    friend class boost::serialization::access;

    /// default constructor
    MeasurementXy() = default;

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
    MeasurementXy(DataT x, DataT y, DataT dx2, DataT dy2, DataT dxy, DataT ndfX, DataT ndfY)
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

    void SetX(DataT x) { fX = x; }
    void SetY(DataT y) { fY = y; }
    void SetDx2(DataT dx2) { fDx2 = dx2; }
    void SetDy2(DataT dy2) { fDy2 = dy2; }
    void SetDxy(DataT dxy) { fDxy = dxy; }
    void SetNdfX(DataT ndfX) { fNdfX = ndfX; }
    void SetNdfY(DataT ndfY) { fNdfY = ndfY; }
    void SetCov(DataT dx2, DataT dxy, DataT dy2)
    {
      fDx2 = dx2;
      fDxy = dxy;
      fDy2 = dy2;
    }

    DataT X() const { return fX; }
    DataT Y() const { return fY; }
    DataT Dx2() const { return fDx2; }
    DataT Dy2() const { return fDy2; }
    DataT Dxy() const { return fDxy; }
    DataT NdfX() const { return fNdfX; }
    DataT NdfY() const { return fNdfY; }

    ///------------------------------
    /// references, to ease assignment to SIMD vector components when DataT has fvec type

    DataT& X() { return fX; }
    DataT& Y() { return fY; }
    DataT& Dx2() { return fDx2; }
    DataT& Dy2() { return fDy2; }
    DataT& Dxy() { return fDxy; }
    DataT& NdfX() { return fNdfX; }
    DataT& NdfY() { return fNdfY; }

    ///------------------------------
    /// Methods for debugging

    /// String representation of class contents
    /// \param indentLevel      number of indent characters in the output
    std::string ToString(int indentLevel = 0) const;

    /// Checks, if all fields are finite
    bool IsFinite() const
    {
      return (utils::IsFinite(X()) && utils::IsFinite(Y()) && utils::IsFinite(Dx2()) && utils::IsFinite(Dy2())
              && utils::IsFinite(Dxy()) && utils::IsFinite(NdfX()) && utils::IsFinite(NdfY()));
    }

    /// Checks, if some fields are undefined
    bool IsUndefined() const
    {
      return (utils::IsUndefined(X()) || utils::IsUndefined(Y()) || utils::IsUndefined(Dx2())
              || utils::IsUndefined(Dy2()) || utils::IsUndefined(Dxy()) || utils::IsUndefined(NdfX())
              || utils::IsUndefined(NdfY()));
    }

    /// Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fX;
      ar& fY;
      ar& fDx2;
      ar& fDy2;
      ar& fDxy;
      ar& fNdfX;
      ar& fNdfY;
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

    DataT fX{0.};    ///< x coordinate of the measurement
    DataT fY{0.};    ///< y coordinate of the measurement
    DataT fDx2{1.};  ///< rms^2 of the x coordinate measurement
    DataT fDy2{1.};  ///< rms^2 of the y coordinate measurement
    DataT fDxy{0.};  ///< covariance of the x and y coordinate measurements

    /// number of degrees of freedom (used for chi2 calculation)
    /// if ndf == 1, the measurement is used in the chi2 calculation
    /// if ndf == 0, the measurement is not used in the chi2 calculation

    DataT fNdfX = 0.;  ///< ndf for the x coordinate measurement
    DataT fNdfY = 0.;  ///< ndf for the y coordinate measurement

  } _fvecalignment;


  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename TdataA>
  template<typename TdataB, bool TDoAllA, bool TDoAllB>
  inline void MeasurementXy<TdataA>::CopyBase(const int ia, const MeasurementXy<TdataB>& Tb, const int ib)
  {
    auto copy = [&](TdataA& a, const TdataB& b) {
      utils::VecCopy<TdataA, TdataB, TDoAllA, TDoAllB>::CopyEntries(a, ia, b, ib);
    };

    copy(fX, Tb.X());
    copy(fY, Tb.Y());
    copy(fDx2, Tb.Dx2());
    copy(fDy2, Tb.Dy2());
    copy(fDxy, Tb.Dxy());
    copy(fNdfX, Tb.NdfX());
    copy(fNdfY, Tb.NdfY());
  }  // CopyBase

}  // namespace cbm::algo::kf
