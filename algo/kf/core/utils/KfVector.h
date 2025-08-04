/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfVector.h
/// \brief  std::vector with an additional utility set
/// \since  30.03.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "AlgoFairloggerCompat.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
//#include <boost/stacktrace.hpp>

#include <exception>
#include <sstream>
#include <vector>

namespace cbm::algo::kf
{
  /// \class Vector
  /// \brief A std::vector with additional debugging utility set
  template<class T, class Allocator = std::allocator<T>>
  class Vector : public std::vector<T, Allocator> {
   public:
    using BaseVector_t = std::vector<T, Allocator>;

    /// \brief Generic constructor from parameters
    template<typename... Args>
    Vector(Args... values) : BaseVector_t(values...)
    {
    }

    /// \brief Copy constructor
    Vector(const Vector& other) : BaseVector_t(other) {}

    /// \brief Move constructor
    Vector(Vector&& other) : BaseVector_t(std::move(other)) {}

    /// \brief Copy assignment operator
    //Vector& operator=(const Vector& other) { return BaseVector_t::operator=(other); }
    Vector& operator=(const Vector& other) = default;

    /// \brief Move assignment operator
    //Vector& operator=(Vector&& other) { return BaseVector_t::operator=(std::move(other)); }
    Vector& operator=(Vector&& other) = default;

    /// \brief Access operator overload
    template<bool CheckBoundaries = false>
    T& at(std::size_t pos)
    {
      if constexpr (CheckBoundaries) {
        if (pos >= this->size()) {
          std::stringstream msg;
          msg << "kf::Vector::operator[]: accessing element with index out of boundaries (pos = " << pos
              << ", size = " << this->size() << "). "
              << "Stack trace:\n";
          //<< boost::stacktrace::stacktrace();
          throw std::runtime_error(msg.str());
        }
      }
      return BaseVector_t::operator[](pos);
    }

    /// \brief Constant access operator overload
    template<bool CheckBoundaries = false>
    const T& at(std::size_t pos) const
    {
      if constexpr (CheckBoundaries) {
        if (pos >= this->size()) {
          std::stringstream msg;
          msg << "kf::Vector::operator[]: accessing element with index out of boundaries (pos = " << pos
              << ", size = " << this->size() << "). "
              << "Stack trace:\n";
          //<< boost::stacktrace::stacktrace();
          throw std::runtime_error(msg.str());
        }
      }
      return BaseVector_t::operator[](pos);
    }

   private:
    /// \brief Serialization method
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<BaseVector_t>(*this);
    }
  };
}  // namespace cbm::algo::kf
