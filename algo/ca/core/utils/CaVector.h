/* Copyright (C) 2021-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Sergei Zharko */

/// \file CaVector.h
/// \author Sergey Gorbunov
/// \date 2021-06-16

//#pragma once  // include this header only once per compilation unit (not robust)

#ifndef CA_CORE_CaVector_h
#define CA_CORE_CaVector_h 1

#include "AlgoFairloggerCompat.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>

namespace cbm::algo::ca
{
  /// \class Vector
  ///
  /// ca::Vector class is a wrapper around std::vector.
  /// It does the following:
  /// 1. gives names to vectors for better debugging
  /// 2. controls the out-of-range access in debug mode
  /// 3. supresses methods that are currently not controlled
  /// 4. warns when slow memory operations are called,
  ///    i.e. when the preallocated capacity is reached and the entire vector should be copied to a new place
  /// 5. blocks usage of boolean vectors, as they have a special
  ///    space-optimized but slow implementation in std. (Use ca::Vector<char> instead).
  ///
  template<class T>
  class Vector : private std::vector<T> {
    friend class boost::serialization::access;

   public:
    typedef std::vector<T> Tbase;

    /// \brief Generic constructor from vairadic parameter list
    template<typename... Tinput>
    Vector(Tinput... value) : Tbase(value...)
    {
    }

    /// \brief Generic constructor from vairadic parameter list including the name of the vector
    template<typename... Tinput>
    Vector(const char* name, Tinput... value) : Tbase(value...)
                                              , fName(name)
    {
    }

    /// \brief Constructor to make initializations like ca::Vector<int> myVector {"MyVector", {1, 2, 3}}
    Vector(const std::string& name, std::initializer_list<T> init) : Tbase(init), fName(name) {}

    /// \brief Copy constructor
    Vector(const Vector& v) : Tbase() { *this = v; }

    /// \brief Move constructor
    Vector(Vector&& v) noexcept : Tbase(std::move(v)), fName(std::move(v.fName)) {}

    /// \brief Copy assignment operator
    Vector& operator=(const Vector& v)
    {
      if (this != &v) {
        fName = v.fName;
        Tbase::reserve(v.capacity());  // make sure that the capacity is transmitted
        Tbase::assign(v.begin(), v.end());
      }
      return *this;
    }

    /// \brief Move assignment operator
    Vector& operator=(Vector&& v) noexcept
    {
      if (this != &v) {
        std::swap(fName, v.fName);
        Tbase::swap(v);
      }
      return *this;
    }

    /// \brief Swap operator
    void swap(Vector& v) noexcept
    {
      if (this != &v) {
        std::swap(fName, v.fName);
        Tbase::swap(v);
      }
    }

    /// \brief Sets the name of the vector
    /// \param s  Name of the vector
    void SetName(const std::string& s) { fName = s; }

    /// \brief Sets the name of the vector
    /// \param s  Name of the vector (string stream)
    void SetName(const std::basic_ostream<char>& s)
    {
      // helps to set a composed name in a single line via:
      // SetName(std::stringstream()<<"my name "<<..whatever..);
      fName = dynamic_cast<const std::stringstream&>(s).str();
    }

    /// \brief  Gets name of the vector
    std::string GetName() const
    {
      std::string s = "ca::Vector<";
      s += fName + "> ";
      return s;
    }

    /// \brief Clears vector and resizes it to the selected size with selected values
    /// \param count  New size of the vector
    /// \param value  Variadic list of the parameters to pass to the base std::vector::resize function
    template<typename... Tinput>
    void reset(std::size_t count, Tinput... value)
    {
      // does the same as Tbase::assign(), but works with the default T constructor too
      // (no second parameter)
      Tbase::clear();
      Tbase::resize(count, value...);
    }

    /// \brief Enlarges the vector to the new size
    /// \param count  New size of the vector
    /// \param value  Value of the new elements
    template<typename... Tinput>
    void enlarge(std::size_t count, Tinput... value)
    {
      if (count < Tbase::size()) {
        LOG(fatal) << "ca::Vector \"" << fName << "\"::enlarge(" << count
                   << "): the new size is smaller than the current one " << Tbase::size() << ", something goes wrong.";
        assert(count >= Tbase::size());
      }
      if ((!Tbase::empty()) && (count > Tbase::capacity())) {
        LOG(warning) << "ca::Vector \"" << fName << "\"::enlarge(" << count << "): allocated capacity of "
                     << Tbase::capacity() << " is reached, the vector of size " << Tbase::size()
                     << " will be copied to the new place.";
      }
      Tbase::resize(count, value...);
    }

    /// \brief Reduces the vector to a given size
    /// \param count  Size of the new vector
    void shrink(std::size_t count)
    {
      if (count > Tbase::size()) {
        LOG(fatal) << "ca::Vector \"" << fName << "\"::shrink(" << count
                   << "): the new size is bigger than the current one " << Tbase::size() << ", something goes wrong.";
        assert(count < Tbase::size());
      }
      Tbase::resize(count);
    }

    /// \brief Reserves a new size for the vector
    /// \param count  New size of the vector
    void reserve(std::size_t count)
    {
      if (!Tbase::empty()) {
        LOG(fatal) << "ca::Vector \"" << fName << "\"::reserve(" << count << "): the vector is not empty; "
                   << " it will be copied to the new place.";
        assert(Tbase::empty());
      }
      Tbase::reserve(count);
    }

    /// \brief Pushes back a value to the vector
    /// \param value  New value
    /// \note  Raises a warning, if the vector re-alocates memory
    template<typename Tinput>
    void push_back(Tinput value)
    {
      if (Tbase::size() >= Tbase::capacity()) {
        LOG(warning) << "ca::Vector \"" << fName << "\"::push_back(): allocated capacity of " << Tbase::capacity()
                     << " is reached, re-allocate and copy.";
      }
      Tbase::push_back(value);
    }

    /// \brief Pushes back a value to the vector without testing for the memory re-alocation
    /// \param value  New value
    template<typename Tinput>
    void push_back_no_warning(Tinput value)
    {
      Tbase::push_back(value);
    }

    /// \brief Creates a parameter in the end of the vector
    /// \param value  Variadic list of the parameters, which are passed to the element constructor
    template<typename... Tinput>
    void emplace_back(Tinput&&... value)
    {
      if (Tbase::size() >= Tbase::capacity()) {
        LOG(warning) << "ca::Vector \"" << fName << "\"::emplace_back(): allocated capacity of " << Tbase::capacity()
                     << " is reached, re-allocate and copy.";
      }
      Tbase::emplace_back(value...);
    }

    /// \brief Mutable access to the element by its index
    /// \param pos  Index of the element
    T& operator[](std::size_t pos)
    {
      if (pos >= Tbase::size()) {
        LOG(fatal) << "ca::Vector \"" << fName << "\": trying to access element " << pos
                   << " outside of the vector of the size of " << Tbase::size();
        assert(pos < Tbase::size());
      }
      return Tbase::operator[](pos);
    }

    /// \brief Constant access to the element by its index
    /// \param pos  Index of the element
    const T& operator[](std::size_t pos) const
    {
      if (pos >= Tbase::size()) {
        LOG(fatal) << "ca::Vector \"" << fName << "\": trying to access element " << pos
                   << " outside of the vector of the size of " << Tbase::size();
        assert(pos < Tbase::size());
      }
      return Tbase::operator[](pos);
    }

    /// \brief Mutable access to the last element of the vector
    T& back()
    {
      if (Tbase::size() == 0) {
        LOG(fatal) << "ca::Vector \"" << fName << "\": trying to access element of an empty vector";
        assert(Tbase::size() > 0);
      }
      return Tbase::back();
    }

    /// \brief Constant access to the last element of the vector
    const T& back() const
    {
      if (Tbase::size() == 0) {
        LOG(fatal) << "ca::Vector \"" << fName << "\": trying to access element of an empty vector";
        assert(Tbase::size() > 0);
      }
      return Tbase::back();
    }

    using Tbase::begin;
    using Tbase::capacity;
    using Tbase::cbegin;
    using Tbase::cend;
    using Tbase::clear;
    using Tbase::end;
    using Tbase::insert;  //TODO:: make it private
    using Tbase::pop_back;
    using Tbase::rbegin;
    using Tbase::reserve;
    using Tbase::shrink_to_fit;
    using Tbase::size;
    using typename Tbase::iterator;

   private:
    std::string fName{"no name"};  ///< Name of the vector
    using Tbase::assign;           // use reset() instead
    using Tbase::at;
    using Tbase::resize;

    /// \brief Serialization function for the vector
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<Tbase>(*this);
      ar& fName;
    }
  };

  ///
  /// std::vector<bool> has a special implementation that is space-optimized
  /// and therefore slow and not thread-safe.
  /// That is why one should use ca::Vector<char> instead.
  ///
  template<>
  class Vector<bool> {
  };
}  // namespace cbm::algo::ca

#endif  // CA_CORE_CaVector_h
