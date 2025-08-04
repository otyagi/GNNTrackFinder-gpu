/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfMath.h
/// @brief  Collection of generic mathematical methods
/// @since  30.07.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include <cstddef>

namespace cbm::algo::kf::math
{
  /// \brief Number of coefficients in a polynomial
  /// \param N  Degree of the polynomial
  /// \param M  Number of dimensions
  constexpr size_t NofPolCoefficients(size_t N, size_t M)
  {
    return M == 1 ? N + 1 : (NofPolCoefficients(N, M - 1) * (M + N)) / M;
  }

  /// \brief Horner's scheme for a 1D-polynomial estimation
  /// \tparam  T  Underlying data type
  /// \tparam  N  Degree of the polynomial
  /// \param   c  Pointer to the first element of the polynomial coefficients
  /// \param   x  Variable, for which the polynomial is to be estimated
  ///
  /// The polynomial coefficients are indexed as follows:
  ///
  /// P<N>(x) = x * (... * (x * (x * c[0] + c[1]) + c[2]) + ...) + c[N]
  ///         = c[0] * x^N + c[1] * x^(N-1) + ... + c[N - 1] * x + c[N]
  template<size_t N, typename T>
  constexpr T Horner(const T* c, const T& x)
  {
    if constexpr (N == 0) {
      return c[0];
    }
    else {
      return Horner<N - 1, T>(c, x) * x + c[N];
    }
  }

  /// \brief Horner's scheme for a multidvariable polynomial estimation
  /// \tparam  T  Underlying data type
  /// \tparam  N  Degree of the polynomial
  /// \param   c  Pointer to the first element of the polynomial coefficients
  /// \param  x1  First variable of the polynomial
  /// \param  xI  Other variables: x2, ..., xM
  ///
  /// Example of the polynomial coefficient indexing for N = 4, M = 3:
  ///
  /// P(x,y,z) =
  ///            c[0]  * x4 +
  ///
  ///            c[1]  * x3y +
  ///            c[2]  * x3z  + c[3]  * x3 +
  ///
  ///            c[4]  * x2y2 +
  ///            c[5]  * x2yz + c[6]  * x2y +
  ///            c[7]  * x2z2 + c[8]  * x2z + c[9]  * x2 +
  ///
  ///            c[10] * xy3  +
  ///            c[11] * xy2z + c[12] * xy2 +
  ///            c[13] * xyz2 + c[14] * xyz + c[15] * xy +
  ///            c[16] * xz3  + c[17] * xz2 + c[18] * xz + c[19] * x +
  ///
  ///            c[20] * y4   +
  ///            c[21] * y3z  + c[22] * y3  +
  ///            c[23] * y2z2 + c[24] * y2z + c[25] * y2 +
  ///            c[26] * yz3  + c[27] * yz2 + c[28] * yz + c[29] * y +
  ///            c[30] * z4   + c[31] * z3  + c[32] * z2 + c[33] * z + c[34];
  template<size_t N, typename T, typename... Args>
  constexpr T Horner(const T* c, const T& x1, Args... xI)
  {
    if constexpr (N == 0) {
      return c[0];
    }
    else {
      constexpr size_t M = sizeof...(Args) + 1;
      constexpr size_t k = NofPolCoefficients(N - 1, M);
      return Horner<N - 1, T>(c, x1, xI...) * x1 + Horner<N, T>(c + k, xI...);
    }
  }
}  // namespace cbm::algo::kf::math
