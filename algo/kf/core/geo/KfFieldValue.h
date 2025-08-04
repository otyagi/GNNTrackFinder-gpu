/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfFieldValue.h
/// \brief  Magnetic flux density vector representation
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  13.08.2024

#pragma once

#include "KfDefs.h"
#include "KfMath.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

#include <array>
#include <functional>
#include <string>
#include <tuple>

namespace cbm::algo::kf
{

  /// \class  FieldValue
  /// \brief  Magnetic flux density vector
  /// \tparam T  Underlying data-type (float, double, fvec)
  template<typename T>
  class alignas(VcMemAlign) FieldValue {
    template<typename>
    friend class FieldValue;

   public:
    /// \brief Default constructor
    /// \note  By default field is zero
    FieldValue() = default;

    /// \brief Constructor from components
    /// \tparam I  Data type of the input parameters
    /// \param bx  x-component of magnetic field vector [kG]
    /// \param by  y-component of magnetic field vector [kG]
    /// \param bz  z-component of magnetic field vector [kG]
    template<typename I>
    FieldValue(const I& bx, const I& by, const I& bz)
      : fB({utils::simd::Cast<I, T>(bx), utils::simd::Cast<I, T>(by), utils::simd::Cast<I, T>(bz)})
    {
    }

    /// \brief Copy constructor
    /// \tparam I  Underlying data-type of the other object
    template<typename I>
    FieldValue(const FieldValue<I>& other) : fB(utils::simd::Cast<I, T, 3>(other.fB))
    {
    }

    /// \brief Destructor
    ~FieldValue() = default;

    /// \brief Copy assignment operator
    FieldValue& operator=(const FieldValue& other) = default;

    /// \brief Constructor from components
    /// \tparam I  Data type of the input parameters
    /// \param bx  x-component of magnetic field vector [kG]
    /// \param by  y-component of magnetic field vector [kG]
    /// \param bz  z-component of magnetic field vector [kG]
    template<typename I>
    void Set(const I& bx, const I& by, const I& bz)
    {
      fB[0] = utils::simd::Cast<I, T>(bx);
      fB[1] = utils::simd::Cast<I, T>(by);
      fB[2] = utils::simd::Cast<I, T>(bz);
    }

    /// \brief Combines the current magnetic field value with another one using a mask
    /// \param other Other flux value
    /// \param mask  Mask to perform a combination
    void SetSimdEntries(const FieldValue& other, const kf::utils::masktype<T>& mask)
    {
      for (size_t iD = 0; iD < fB.size(); ++iD) {
        this->fB[iD] = kf::utils::iif(mask, other.fB[iD], this->fB[iD]);
      }
    }

    /// \brief Gets magnetic flux density x, y, z-components [kG]
    /// \return  tuple (Bx, By, Bz)
    [[gnu::always_inline]] std::tuple<T, T, T> Get() const { return std::make_tuple(fB[0], fB[1], fB[2]); }

    /// \brief Gets squared absolute magnetic flux [kG2]
    [[gnu::always_inline]] T GetAbsSq() const { return fB[0] * fB[0] + fB[1] * fB[1] + fB[2] * fB[2]; }

    /// \brief Gets absolute magnetic flux [kG]
    [[gnu::always_inline]] T GetAbs() const { return std::sqrt(this->GetAbsSq()); }

    /// \brief Gets component by index
    /// \param iD  index of the component (i = 0: x, 1: y, 2: z)
    [[gnu::always_inline]] T GetComponent(int iD) const { return fB[iD]; }

    /// \brief Gets magnetic flux x-component [kG]
    [[gnu::always_inline]] T GetBx() const { return fB[0]; }

    /// \brief Gets magnetic flux density y-component [kG]
    [[gnu::always_inline]] T GetBy() const { return fB[1]; }

    /// \brief Gets magnetic flux density z-component [kG]
    [[gnu::always_inline]] T GetBz() const { return fB[2]; }

    /// \brief Checks, if the field value is zero (negligible)
    [[gnu::always_inline]] bool IsZero() const
    {
      auto bZero = this->GetAbsSq() <= defs::MinField<T> * defs::MinField<T>;
      if constexpr (std::is_same_v<T, fvec>) {
        return bZero.isFull();
      }
      else {
        return bZero;
      }
    }

    /// Consistency checker
    void CheckConsistency() const;

    // TODO (?): Provide setters/accessors

    /// \brief Sets magnetic flux density components to the field function
    /// \param bx x-component of the magnetic flux density [kG]
    /// \param by y-component of the magnetic flux density [kG]
    /// \param bz z-component of the magnetic flux density [kG]
    /// \param i  index of the SIMD entry (ignored, if T is not a SIMD vector)
    [[gnu::always_inline]] void SetSimdEntry(double bx, double by, double bz, size_t i)
    {
      utils::simd::SetEntry(fB[0], bx, i);  //B.x[i] = bx;
      utils::simd::SetEntry(fB[1], by, i);  //B.y[i] = by;
      utils::simd::SetEntry(fB[2], bz, i);  //B.z[i] = bz;
    }

    /// \brief String representation of the field
    std::string ToString(int indentLevel = 0) const;

   private:
    /// Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fB;
    }

    /// \brief Magnetic flux vector [kG]
    GeoVector_t<T> fB{
      {utils::simd::Cast<float, T>(0.F), utils::simd::Cast<float, T>(0.F), utils::simd::Cast<float, T>(0.F)}};
  };

}  // namespace cbm::algo::kf
