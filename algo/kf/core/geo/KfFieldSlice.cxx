/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfFieldSlice.h
/// \brief  A class for a magnetic field approximation on a transverse plane (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  13.08.2024

#include "KfFieldSlice.h"

#include <iomanip>
#include <sstream>

#include <fmt/format.h>

using cbm::algo::kf::FieldFn_t;
using cbm::algo::kf::FieldSlice;
using cbm::algo::kf::FieldValue;

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
FieldSlice<T>::FieldSlice(const FieldFn_t& fieldFn, double xMax, double yMax, double zRef) : fZref(zRef)
{
  // SLE initialization
  // Augmented matrix N x (N + 3), where N - number of coefficients, defining the polynomial
  //
  std::array<std::array<double, kNofCoeff + 3>, kNofCoeff> A = {{}};

  double dx = xMax / kNofCoeff / 10.;
  double dy = yMax / kNofCoeff / 10.;

  if (dx > 1.) {
    dx = 1.;
  }
  if (dy > 1.) {
    dy = 1.;
  }

  // Point and field arrays to access the field
  std::array<double, 3> field;

  // Monomial values for each coefficient
  std::array<double, kNofCoeff> m = {{}};

  // Fill the augmented matrix
  for (double x = -xMax; x <= xMax; x += dx) {
    for (double y = -yMax; y <= yMax; y += dy) {
      std::tie(field[0], field[1], field[2]) = fieldFn(x, y, zRef);
      // Fill the monomial array
      {
        m[kNofCoeff - 1] = 1.;
        for (int i = kNofCoeff - 2; i >= kNofCoeff - 1 - kPolDegree; --i) {
          m[i] = y * m[i + 1];
        }
        double xFactor = x;
        int k          = ((kPolDegree - 1) * (kPolDegree + 2)) / 2;  // index of the monomial vector element
        for (int i = kPolDegree; i > 0; --i) {                       // loop over y powers
          for (int j = kNofCoeff - 1; j >= kNofCoeff - i; --j) {     // loop over x powers
            m[k--] = xFactor * m[j];
          }
          xFactor *= x;
        }
      }

      {
        double w = 1.;  // / (r2 + 1.);
        for (int i = 0; i < kNofCoeff; ++i) {
          // Fill the left part
          for (int j = 0; j < kNofCoeff; ++j) {
            A[i][j] += w * m[i] * m[j];
          }
          // Fill the right part
          for (int j = 0; j < 3; ++j) {
            A[i][kNofCoeff + j] += w * field[j] * m[i];
          }
        }
      }
    }
  }

  // SLE solution using Gaussian elimination
  {
    for (int k = 0; k < kNofCoeff - 1; ++k) {
      for (int j = k + 1; j < kNofCoeff; ++j) {
        double factor = A[j][k] / A[k][k];
        for (int i = 0; i < kNofCoeff + 3; ++i) {
          A[j][i] -= factor * A[k][i];
        }
      }
    }
    for (int k = kNofCoeff - 1; k > 0; --k) {
      for (int j = k - 1; j >= 0; --j) {
        double factor = A[j][k] / A[k][k];
        for (int i = k; i < kNofCoeff + 3; ++i) {
          A[j][i] -= factor * A[k][i];
        }
      }
    }
  }

  for (int j = 0; j < kNofCoeff; ++j) {
    fBx[j] = utils::simd::Cast<double, T>(A[j][kNofCoeff] / A[j][j]);
    fBy[j] = utils::simd::Cast<double, T>(A[j][kNofCoeff + 1] / A[j][j]);
    fBz[j] = utils::simd::Cast<double, T>(A[j][kNofCoeff + 2] / A[j][j]);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
std::string FieldSlice<T>::ToString(int indentLevel, int verbose) const
{
  using fmt::format;
  constexpr char indentChar = '\t';
  std::stringstream msg;
  std::string indent(indentLevel, indentChar);
  auto Cnv = [&](const auto& val) { return utils::simd::Cast<T, Literal_t<T>>(val); };  // alias for the conversion fn

  if (verbose > 0) {
    msg << indent << format("FieldSlice: zRef = {} cm", Cnv(fZref));
    if (verbose > 1) {
      std::array<std::string, kNofCoeff> m;
      {
        m[kNofCoeff - 1] = "";
        for (int i = kNofCoeff - 2; i >= kNofCoeff - 1 - kPolDegree; --i) {
          m[i] = format("y{}", kNofCoeff - i - 1);
        }
        int k = ((kPolDegree - 1) * (kPolDegree + 2)) / 2;  // index of the monomial vector element
        for (int i = kPolDegree; i > 0; --i) {              // loop over y powers
          std::string x = format("x{}", kPolDegree - i + 1);
          for (int j = kNofCoeff - 1; j >= kNofCoeff - i; --j) {  // loop over x powers
            m[k--] = x + m[j];
          }
        }
        m[kNofCoeff - 1] = "1";
      }
      msg << '\n' << indent << format("{:>15} {:>15} {:>15} {:>15}", "Monomial", "Bx", "By", "Bz");
      for (int i = 0; i < kNofCoeff; ++i) {
        msg << '\n' << indent << format("{:>15} {:>15} {:>15} {:>15}", m[i], Cnv(fBx[i]), Cnv(fBy[i]), Cnv(fBz[i]));
      }
    }
  }
  return msg.str();
}

template<typename T>
void FieldSlice<T>::CheckConsistency() const
{
  // Check SIMD data vectors for consistent initialization
  for (int i = 0; i < kNofCoeff; ++i) {
    utils::CheckSimdVectorEquality(fBx[i], "FieldSlice: cx");
    utils::CheckSimdVectorEquality(fBy[i], "FieldSlice: cy");
    utils::CheckSimdVectorEquality(fBz[i], "FieldSlice: cz");
  }
  utils::CheckSimdVectorEquality(fZref, "FieldSlice: z");
}

namespace cbm::algo::kf
{
  template class FieldSlice<float>;
  template class FieldSlice<double>;
  template class FieldSlice<fvec>;
}  // namespace cbm::algo::kf
