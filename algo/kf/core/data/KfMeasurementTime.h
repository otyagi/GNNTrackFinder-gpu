/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Sergei Zharko */

/// \file KfMeasurementTime
/// \brief Definition of the KfMeasurementTime class

#pragma once  // include this header only once per compilation unit


#include "KfDefs.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>

#include <string>

namespace cbm::algo::kf
{
  /// \brief The class describes a time measurement
  ///
  /// The measurement has a finite resolution, i.e. the measurement is not a value, but a distribution
  /// with a certain rms.
  /// The measurement may be used in the chi2 calculation or not
  /// The measurement may be a SIMD vector of values, when DataT is fvec type
  ///
  template<typename DataT>
  class alignas(VcMemAlign) MeasurementTime {

   public:
    friend class boost::serialization::access;

    /// default constructor
    MeasurementTime() = default;

    /// constructor
    /// \param t        time coordinate of the measurement
    /// \param dt2      rms^2 of the time coordinate measurement
    /// \param ndfT     number of degrees of freedom for the time coordinate measurement
    ///                 if ndfT == 1, the measurement is used in fit and in the chi2 calculation
    ///                 if ndfT == 0, the measurement is used neither in fit nor in the chi2 calculation
    MeasurementTime(DataT t, DataT dt2, DataT ndfT) : fT(t), fDt2(dt2), fNdfT(ndfT) {}

    ///------------------------------
    /// Setters and getters

    void SetT(DataT t) { fT = t; }
    void SetDt2(DataT dt2) { fDt2 = dt2; }
    void SetNdfT(DataT ndfT) { fNdfT = ndfT; }

    DataT T() const { return fT; }
    DataT Dt2() const { return fDt2; }
    DataT NdfT() const { return fNdfT; }


    ///------------------------------
    /// Methods for debugging

    /// String representation of class contents
    /// \param indentLevel      number of indent characters in the output
    std::string ToString(int indentLevel = 0) const;

    /// Checks, if all fields are finite
    bool IsFinite() const { return (utils::IsFinite(T()) && utils::IsFinite(Dt2()) && utils::IsFinite(NdfT())); }

    /// Checks, if some fields are undefined
    bool IsUndefined() const
    {
      return (utils::IsUndefined(T()) || utils::IsUndefined(Dt2()) || utils::IsUndefined(NdfT()));
    }

    /// Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fT;
      ar& fDt2;
      ar& fNdfT;
    }

   private:
    ///------------------------------
    /// Data members

    DataT fT{0};     ///< time coordinate of the measurement
    DataT fDt2{1.};  ///< rms^2 of the time coordinate measurement

    /// number of degrees of freedom (used for chi2 calculation)
    /// if ndf == 1, the measurement is used in fit and in the chi2 calculation
    /// if ndf == 0, the measurement is used neither in fit nor in the chi2 calculation

    DataT fNdfT{0};  ///< ndf for the time coordinate measurement

  } _fvecalignment;

}  // namespace cbm::algo::kf
