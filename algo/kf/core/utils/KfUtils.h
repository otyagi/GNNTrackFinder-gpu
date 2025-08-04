/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaUtils.h
/// \brief  Compile-time constants definition for the CA tracking algorithm
/// \since  02.06.2022
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "KfDefs.h"
#include "KfSimd.h"

#include <array>
#include <sstream>

/// Namespace contains compile-time constants definition for the CA tracking algorithm
///
namespace cbm::algo::kf::utils
{

  template<typename T>
  using scaltype = typename std::conditional<std::is_same<T, fvec>::value, fscal, T>::type;

  template<typename T>
  using masktype = typename std::conditional<std::is_same<T, fvec>::value, fmask, bool>::type;

  inline fvec iif(const fmask& m, const fvec& t, const fvec& f) { return Vc::iif(m, t, f); }
  inline fvec fabs(const fvec& v) { return Vc::abs(v); }

  template<typename T>
  inline T iif(bool b, T t, T f)
  {
    return b ? t : f;
  }

  template<typename T>
  inline T fabs(const T& v)
  {
    return std::fabs(v);
  }

  template<typename T>
  inline bool isFull(const T& m)
  {
    if constexpr (std::is_same_v<T, fmask>) {
      return m.isFull();
    }
    else {
      return m;
    }
  }

  template<typename T>
  inline T max(const T& a, const T& b)
  {
    if constexpr (std::is_same_v<T, fvec>) {
      return Vc::max(a, b);
    }
    else {
      return std::max(a, b);
    }
  }

  /// tell the CPU that the bool condition is likely to be true
#if defined(__GNUC__) && __GNUC__ - 0 >= 3
  __attribute__((always_inline)) inline bool IsLikely(bool b) { return __builtin_expect(!!(b), 1); }
  __attribute__((always_inline)) inline bool IsUnlikely(bool b) { return __builtin_expect(!!(b), 0); }
#else
  inline bool IsLikely(bool b) { return b; }
  inline bool IsUnlikely(bool b) { return b; }
#endif

  /// \brief Checks whether a variable of a particular type defined
  /// \param val Value to be checked
  template<typename T>
  inline bool IsUndefined(const T& val)
  {
    if constexpr (std::is_same_v<T, float>) {
      return std::isnan(val);
    }
    else if constexpr (std::is_same_v<T, double>) {
      return std::isnan(val);
    }
    else if constexpr (std::is_same_v<T, fscal>) {
      return std::isnan(val);
    }
    else if constexpr (std::is_same_v<T, fvec>) {
      return isnan(val).isNotEmpty();
    }
    else {
      return val == cbm::algo::kf::defs::Undef<T>;
    }
  }

  /// \brief Checks whether a variable of a particular type is finite
  /// \param val  Value to check
  template<typename T>
  inline bool IsFinite(const T& val)
  {
    if constexpr (std::is_same_v<T, float>) {
      return std::isfinite(val);
    }
    else if constexpr (std::is_same_v<T, double>) {
      return std::isfinite(val);
    }
    else if constexpr (std::is_same_v<T, fscal>) {
      return std::isfinite(val);
    }
    else if constexpr (std::is_same_v<T, fvec>) {
      return isfinite(val).isFull();
    }
    else {
      return val != cbm::algo::kf::defs::Undef<T>;
    }
  }

  /// \brief Stingstream output operation for simd data
  template<typename DataT>
  void PrintSIMDmsg(std::stringstream& msg, DataT& v);

  /// \brief Checks, if a SIMD vector horizontally equal
  template<typename DataT>
  inline void CheckSimdVectorEquality(DataT /*v*/, const char* /*name*/)
  {
  }

  /// \brief Checks, if a SIMD vector horizontally equal
  /// \note  Throws std::logic_error, if check fails
  /// TODO: Find this method in the VC!
  template<>
  void CheckSimdVectorEquality(fvec v, const char* name);

  template<typename TdataA, typename TdataB, bool TDoAllA, bool TDoAllB>
  class VecCopySpec {
   public:
    [[gnu::always_inline]] static void CopyEntries(TdataA& a, int ia, const TdataB& b, int ib);
  };

  template<typename TdataA, typename TdataB>
  class VecCopySpec<TdataA, TdataB, true, true> {
   public:
    [[gnu::always_inline]] static void CopyEntries(TdataA& a, int, const TdataB& b, int) { a = b; }
  };

  template<typename TdataA, typename TdataB>
  class VecCopySpec<TdataA, TdataB, true, false> {
   public:
    [[gnu::always_inline]] static void CopyEntries(TdataA& a, int, const TdataB& b, int ib) { a = b[ib]; }
  };

  template<typename TdataA, typename TdataB>
  class VecCopySpec<TdataA, TdataB, false, true> {
   public:
    [[gnu::always_inline]] static void CopyEntries(TdataA& a, int ia, const TdataB& b, int) { a[ia] = b; }
  };

  template<typename TdataA, typename TdataB>
  class VecCopySpec<TdataA, TdataB, false, false> {
   public:
    [[gnu::always_inline]] static void CopyEntries(TdataA& a, int ia, const TdataB& b, int ib) { a[ia] = b[ib]; }
  };


  /// \brief Copies all/one SIMD entries from one class to the other class
  /// \details It helps to work with templates when classes might be SIMD or scalar
  /// \details If one of the classes is scalar, its SIMD element index is ignored
  ///
  /// \tparam TdataA  Type of the first class
  /// \tparam TdataB  Type of the second class
  /// \tparam TDoAllA  If true, all entries of the first class must be set
  /// \tparam TDoAllB  If true, all entries of the second class must be used
  /// \param a  First class
  /// \param ia  Index of SIMD vector element of the first class
  /// \param b  Second class
  /// \param ib  Index of SIMD vector element of the second class
  template<typename TdataA, typename TdataB, bool TDoAllA, bool TDoAllB>
  class VecCopy {
   public:
    static void CopyEntries(TdataA& a, int ia, const TdataB& b, int ib)
    {
      constexpr bool allA{TDoAllA || !std::is_same<TdataA, fvec>::value};
      constexpr bool allB{TDoAllB || !std::is_same<TdataB, fvec>::value};
      VecCopySpec<TdataA, TdataB, allA, allB>::CopyEntries(a, ia, b, ib);
    }
  };

  /// \struct EnumClassHash
  /// \brief  Hash for unordered_map with enum class keys
  struct EnumClassHash {
    template<typename T>
    int operator()(T t) const
    {
      return static_cast<int>(t);
    }
  };
}  // namespace cbm::algo::kf::utils

/// Namespace contains compile-time constants definition for SIMD operations
/// in the CA tracking algorithm
///
namespace cbm::algo::kf::utils::simd
{
  /// \brief Converts a value of type DataT to type DataOut
  /// \details This function is a generic template that provides a flexible way for fvec/float/double conversions
  /// \tparam DataT Input value data type
  /// \tparam DataOut Converted value data type
  /// \param val Input value of type DataT to be converted
  /// \return Return val as DataOut
  template<typename DataT, typename DataOut>
  inline DataOut Cast(const DataT& val)
  {
    return static_cast<DataOut>(val);
  }

  /// \brief Converts a value of type DataT to type DataOut
  /// \details This specialization extracts the first element of the input SIMD float vector (fvec) and returns it as a float
  /// \param val Input value of type fvec to be converted
  /// \return Return val[0] as float
  template<>
  inline float Cast(const fvec& val)
  {
    return val[0];
  }

  /// \brief Converts a value of type DataT to type DataOut
  /// \details This specialization extracts the first element of the input SIMD float vector (fvec) and returns it as a double
  /// \param val Input value of type fvec to be converted
  /// \return Return val[0] as double
  template<>
  inline double Cast(const fvec& val)
  {
    return static_cast<double>(val[0]);
  }

  /// \brief Converts a value of type DataT at a specific index to type DataOut
  /// \details This function is a generic template that provides a flexible way for fvec/float/double conversions
  /// \tparam DataT Input value data type
  /// \tparam DataOut Converted value data type
  /// \param val Input value of type DataT to be converted
  /// \param i The index indicating which element to convert, ignored for non-SIMD DataT
  /// \return Return val as DataOut
  template<typename DataT, typename DataOut>
  inline DataOut Cast(const DataT& val, size_t /*i*/)
  {
    return static_cast<DataOut>(val);
  }

  template<typename DataT, typename DataOut, size_t N>
  inline std::array<DataOut, N> Cast(const std::array<DataT, N>& arr, size_t i)
  {
    std::array<DataOut, N> res;
    for (size_t iEl = 0; iEl < arr.size(); ++iEl) {
      res[iEl] = Cast<DataT, DataOut>(arr[iEl], i);
    }
    return res;
  }

  template<typename DataT, typename DataOut, size_t N>
  inline std::array<DataOut, N> Cast(const std::array<DataT, N>& arr)
  {
    std::array<DataOut, N> res;
    for (size_t iEl = 0; iEl < arr.size(); ++iEl) {
      res[iEl] = Cast<DataT, DataOut>(arr[iEl]);
    }
    return res;
  }

  /// \brief Converts a value of type DataT at a specific index to type DataOut
  /// \details This specialization extracts the element at the specified index of the input SIMD float vector (fvec) and returns it as a float
  /// \param val Input value of type fvec to be converted
  /// \param i The index indicating which element to convert
  /// \return Return val[i] as float
  template<>
  inline float Cast(const fvec& val, size_t i)
  {
    return val[i];
  }

  /// \brief Converts a value of type DataT at a specific index to type DataOut
  /// \details This specialization extracts the element at the specified index of the input SIMD float vector (fvec) and returns it as a double
  /// \param val Input value of type fvec to be converted
  /// \param i The index indicating which element to convert.
  /// \return Return val[i] as double
  template<>
  inline double Cast(const fvec& val, size_t i)
  {
    return static_cast<double>(val[i]);
  }

  /// \brief Returns the number of elements available for a given data type
  /// \details This function is a template that provides a default implementation returning 1
  /// \tparam DataT The data type for which the size is determined
  /// \return Return 1
  template<typename DataT>
  inline size_t Size()
  {
    return 1;
  }

  /// \brief Returns the number of elements available for a given data type
  /// \details This specialization returns the size of the SIMD float vector (fvec) using its static member function `size()`
  /// \return Return number of elements in the SIMD
  template<>
  inline size_t Size<fvec>()
  {
    return fvec::size();
  }

  /// \brief Returns a value of the data type set to one
  /// \details This function is a template that provides a default implementation returning 1
  /// \tparam DataT The data type for which the value is generated
  /// \return Return 1 as DataT
  template<typename DataT>
  inline DataT One()
  {
    return static_cast<DataT>(1);
  }

  /// \brief Returns a value of the data type set to one
  /// \details This specialization returns a SIMD float vector (fvec) with all elements set to one using its static member function `One()`
  /// \return Return SIMD vector of 1
  template<>
  inline fvec One<fvec>()
  {
    return fvec::One();
  }

  /// \brief Sets a value at a specific index in the output data
  /// \details This function is a template that provides a default implementation for setting a value at a specific index in the output data
  /// \tparam DataT The data type of the output data
  /// \tparam DataIn The data type of the input data
  /// \param out Reference to the output data where the value is to be set
  /// \param in The input data from which the value is obtained
  /// \param i The index at which the value is set, ignored for non-SIMD DataT or if both DataT and DataIn are fvec
  template<typename DataT, typename DataIn>
  inline void SetEntry(DataT& out, DataIn in, size_t /*i*/)
  {
    out = static_cast<DataT>(in);
  }

  /// \brief Sets a value at a specific index in the output data
  /// \details This specialization sets a value at the specified index in the output SIMD float vector (fvec) using the input float value
  /// \param out Reference to the output float vector (fvec) where the value is to be set
  /// \param in The input non-SIMD value from which the value is obtained
  /// \param i The index at which the value is set
  template<typename DataIn>
  inline void SetEntry(fvec& out, DataIn in, size_t i)
  {
    out[i] = in;
  }
}  // namespace cbm::algo::kf::utils::simd


// TODO: SZh 06.06.2024:
//       There is nothing to do with SIMD in the funcitons below. Thus, we should move them into KfMath.h
/// Namespace contains compile-time constants definition for SIMD operations
/// in the CA tracking algorithm
///
namespace cbm::algo::kf::utils::math
{

  void CholeskyFactorization(const double a[], const int n, int nn, double u[], int* nullty, int* ifault);

  void SymInv(const double a[], const int n, double c[], double w[], int* nullty, int* ifault);

}  // namespace cbm::algo::kf::utils::math
