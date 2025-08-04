/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CaEnumArray.h
/// @brief  Array indexed by a generic enum class
/// @since  22.05.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include <boost/serialization/array.hpp>
#include <boost/serialization/base_object.hpp>

#include <array>

namespace cbm::core
{
  /// \class cbm::algo::ca::EnumArray
  /// \brief Class of arrays, which can be accessed by an enum class entry as an index
  /// \note  The enum-array must contain an entry kEND, which represents the number of the enumeration entries and
  ///        is used, as an array size
  /// \tparam  E  The enum class type
  /// \tparam  T  Type of data in the underlying array
  template<class E, class T>
  class EnumArray : public std::array<T, static_cast<std::size_t>(E::END)> {
    using U   = typename std::underlying_type<E>::type;  ///< Underlying type of enumeration
    using Arr = std::array<T, static_cast<std::size_t>(E::END)>;

   public:
    /// \brief Mutable access operator, indexed by enum entry
    /// \param entry  Enum entry for the element
    T& operator[](const E& entry) { return Arr::operator[](static_cast<U>(entry)); }

    /// \brief Mutable access operator, indexed by underlying index type
    /// \param index  Index of the element
    T& operator[](U index) { return Arr::operator[](index); }

    /// \brief Constant access operator, indexed by enum entry
    /// \param entry  Enum entry for the element
    const T& operator[](const E& entry) const { return Arr::operator[](static_cast<U>(entry)); }

    /// \brief Constant access operator, indexed by underlying index type
    /// \param index  Index of the element
    const T& operator[](U index) const { return Arr::operator[](index); }

    /// \brief Convertion operator to the base array class
    operator Arr&() const { return *this; }

   private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<Arr>(*this);
    }
  };
}  // namespace cbm::core
