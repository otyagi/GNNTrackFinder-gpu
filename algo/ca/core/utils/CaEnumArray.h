/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CaEnumArray.h
/// @brief  Implementation of cbm::algo::ca::EnumArray class
/// @since  02.05.2023
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include <boost/serialization/array.hpp>
#include <boost/serialization/base_object.hpp>

#include <array>

namespace cbm::algo::ca
{
  /// \enum  EDummy
  /// \brief Dummy enum, representing an array with zero elements
  ///
  /// This enumeration is to be used as the default template parameter
  enum class EDummy
  {
    END
  };

  /// \class cbm::algo::ca::EnumArray
  /// \brief Class of arrays, which can be accessed by an enum class entry as an index
  /// \note  The enum-array must contain an entry END, which represents the number of the enumeration entries and
  ///        is used, as an array size
  /// \tparam  E  The enum class type
  /// \tparam  T  Type of data in the underlying array
  template<class E, class T>
  class EnumArray : public std::array<T, static_cast<std::size_t>(E::END)> {
    using U   = typename std::underlying_type<E>::type;  ///< Underlying type of enumeration
    using Arr = std::array<T, static_cast<std::size_t>(E::END)>;

   public:
    /// \brief Mutable access operator, indexed by enum entry
    T& operator[](const E entry)
    {
      return std::array<T, static_cast<std::size_t>(E::END)>::operator[](static_cast<U>(entry));
    }

    /// \brief Mutable access operator, indexed by underlying index type
    T& operator[](U index) { return std::array<T, static_cast<std::size_t>(E::END)>::operator[](index); }

    /// \brief Constant access operator, indexed by enum entry
    const T& operator[](const E& entry) const
    {
      return std::array<T, static_cast<std::size_t>(E::END)>::operator[](static_cast<U>(entry));
    }

    /// \brief Constant access operator, indexed by underlying index type
    /// \param index  Index of the element
    const T& operator[](U index) const { return std::array<T, static_cast<std::size_t>(E::END)>::operator[](index); }

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
}  // namespace cbm::algo::ca
