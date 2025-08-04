/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfFieldSlice.h
/// \brief  A class for a magnetic field approximation on a transverse plane (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  13.08.2024

#pragma once

#include "KfDefs.h"
#include "KfFieldValue.h"
#include "KfMath.h"
#include "KfTrackParam.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <array>
#include <functional>
#include <string>

namespace cbm::algo::kf
{
  /// \class FieldSlice
  /// \brief A magnetic field approximation on the two-dimensional plane
  /// \tparam T Underlying data-type
  template<typename T>
  class alignas(VcMemAlign) FieldSlice {
    template<typename>
    friend class FieldSlice;

    static constexpr int kPolDegree{5};                                         ///< Approximation polynomial degree
    static constexpr int kNofCoeff{((kPolDegree + 1) * (kPolDegree + 2)) / 2};  ///< Number of coefficients

    /// \brief Array of the approximation coefficients [<monomial>]
    using CoeffArray_t = std::array<T, kNofCoeff>;

   public:
    /// \brief Default constructor
    FieldSlice() = default;

    /// \brief Constructor from parameters
    /// \param fieldFn  Magnetic field function (point xyz, field xyz)
    /// \param xMax     Half-size of the slice in x-direction [cm]
    /// \param yMax     Half-size of the slice in y-direction [cm]
    /// \param zRef     Reference z-coordinate of the slice [cm]
    FieldSlice(const FieldFn_t& fieldFn, double xMax, double yMax, double zRef);

    /// \brief Copy constructor
    /// \tparam I The underlying data type of the source object
    template<typename I>
    FieldSlice(const FieldSlice<I>& other)
      : fBx(utils::simd::Cast<I, T, kNofCoeff>(other.fBx))
      , fBy(utils::simd::Cast<I, T, kNofCoeff>(other.fBy))
      , fBz(utils::simd::Cast<I, T, kNofCoeff>(other.fBz))
      , fZref(utils::simd::Cast<I, T>(other.fZref))
    {
    }

    /// \brief Destructor
    ~FieldSlice() = default;

    /// \brief Copy assignment operator
    FieldSlice& operator=(const FieldSlice& other) = default;

    /// \brief Gets field value at a point on the transverse plane
    /// \param x  x-coordinate of the point [cm]
    /// \param y  y-coordinate of the point [cm]
    /// \return  Field value [kG]
    constexpr FieldValue<T> GetFieldValue(const T& x, const T& y) const
    {
      using math::Horner;
      return FieldValue<T>(Horner<kPolDegree>(fBx.cbegin(), x, y),  //
                           Horner<kPolDegree>(fBy.cbegin(), x, y),  //
                           Horner<kPolDegree>(fBz.cbegin(), x, y));
    }

    /// \brief Gets field value for the intersection with a straight track
    /// \param trackParam  Track parameter set
    /// \return  Field value [kG]
    constexpr FieldValue<T> GetFieldValueForLine(const TrackParam<T>& trkPar) const
    {
      T dz = fZref - trkPar.GetZ();
      return GetFieldValue(trkPar.GetX() + trkPar.GetTx() * dz, trkPar.GetY() + trkPar.GetTy() * dz);
    }

    /// \brief Gets approximation coefficients for the x-component of the field
    const CoeffArray_t& GetBx() const { return fBx; }

    /// \brief Gets approximation coefficients for the y-component of the field
    const CoeffArray_t& GetBy() const { return fBy; }

    /// \brief Gets approximation coefficients for the z-component of the field
    const CoeffArray_t& GetBz() const { return fBz; }

    /// \brief Gets reference z-coordinate of the slice [cm]
    const T& GetZref() const { return fZref; }

    /// \brief String representation of the class content
    /// \param intdentLevel  Indent level
    /// \param verbose       Verbosity level
    std::string ToString(int indentLevel = 0, int verbose = 1) const;

    /// Consistency checker
    void CheckConsistency() const;

   private:
    /// \brief Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fBx;
      ar& fBy;
      ar& fBz;
      ar& fZref;
    }

    CoeffArray_t fBx{{T(0.)}};  ///< Approximation coefficients for the x-component of the field
    CoeffArray_t fBy{{T(0.)}};  ///< Approximation coefficients for the y-component of the field
    CoeffArray_t fBz{{T(0.)}};  ///< Approximation coefficients for the z-component of the field
    T fZref{defs::Undef<T>};    ///< Reference z of the coefficients [cm]
  };
}  // namespace cbm::algo::kf
