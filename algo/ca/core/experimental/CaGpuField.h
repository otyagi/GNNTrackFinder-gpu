/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Igor Kulakov, Maksym Zyzak, Sergei Zharko */

/// \file CaGpuField.h
/// \brief Magnetic field classes
///
/// This is a temporary solution made to ensure compatibility with GPU
///

#pragma once  // include this header only once per compilation unit

#include "CaUtils.h"
#include "KfField.h"
#include "KfTrackParam.h"

#include <xpu/device.h>

namespace cbm::algo::ca
{
  class GpuFieldValue {
   public:
    float x;  //< x-component of the field
    float y;  //< y-component of the field
    float z;  //< z-component of the field

    GpuFieldValue() = default;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    GpuFieldValue(const kf::FieldValue<DataIn>& other)
      : x(kf::utils::simd::Cast<DataIn, float>(other.GetBx()))
      , y(kf::utils::simd::Cast<DataIn, float>(other.GetBy()))
      , z(kf::utils::simd::Cast<DataIn, float>(other.GetBz()))
    {
    }

    /// Combines the magnetic field with another field value object using weight
    /// \param B  other field value to combine with
    /// \param w  weight from 0 to 1

    XPU_D void Combine(const GpuFieldValue& B, const bool w)
    {
      if (w) {
        x = B.x;
        y = B.y;
        z = B.z;
      }
    }

    /// Is the field value zero?
    XPU_D bool IsZeroField() const
    {
      //     constexpr auto e = constants::misc::NegligibleFieldkG;
      //     float e = 1.e-4;
      return (x * x + y * y + z * z <= 1.e-8);
    }
  };

  class GpuFieldSlice {
   public:
    /// Default constructor
    GpuFieldSlice() = default;

    static constexpr int kPolDegree{5};                                         ///< Approximation polynomial degree
    static constexpr int kNofCoeff{((kPolDegree + 1) * (kPolDegree + 2)) / 2};  ///< Number of coefficients

    /// \brief Array of the approximation coefficients [<monomial>]
    using CoeffArray_t = std::array<float, kNofCoeff>;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    GpuFieldSlice(const kf::FieldSlice<DataIn>& other) : z(kf::utils::simd::Cast<DataIn, float>(other.GetZref()))
    {
      for (size_t i = 0; i < kMaxNFieldApproxCoefficients; i++) {
        cx[i] = kf::utils::simd::Cast<DataIn, float>(other.GetBx()[i]);
        cy[i] = kf::utils::simd::Cast<DataIn, float>(other.GetBy()[i]);
        cz[i] = kf::utils::simd::Cast<DataIn, float>(other.GetBz()[i]);
      }
    }

    /// Gets field value from (x, y) fvec point
    /// \param x  x-coordinate of input
    /// \param y  y-coordinate of input
    /// \return B  the FieldValue output
    XPU_D GpuFieldValue GetFieldValue_old(float x, float y) const;

    //
    XPU_D GpuFieldValue GetFieldValue(float x, float y) const
    {
      using cbm::algo::kf::math::Horner;
      GpuFieldValue B;
      B.x = Horner<kPolDegree>(cx.cbegin(), x, y);
      B.y = Horner<kPolDegree>(cy.cbegin(), x, y);
      B.z = Horner<kPolDegree>(cz.cbegin(), x, y);
      return B;
    }
    //

    /// Gets field value for the intersection with a straight track
    /// \param t  straight track
    /// \return B  the FieldValue output
    XPU_D GpuFieldValue GetFieldValueForLine(const kf::TrackParamBase<float>& t) const;

   public:
    // NOTE: We don't use an initialization of arrays here because we cannot be sure
    //       if the underlying type (fvec) has a default constructor, but
    //       we are sure, that it can be initialized with a float. (S.Zharko)
    static constexpr auto kMaxNFieldApproxCoefficients = 21;  //constants::size::MaxNFieldApproxCoefficients;

    CoeffArray_t cx;  //fBx;  ///< Approximation coefficients for the x-component of the field
    CoeffArray_t cy;  //fBy;  ///< Approximation coefficients for the y-component of the field
    CoeffArray_t cz;  //fBz;  ///< Approximation coefficients for the z-component of the field

    float z;  ///< z coordinate of the slice
  };

  XPU_D inline GpuFieldValue GpuFieldSlice::GetFieldValue_old(float x, float y) const
  {
    float x2 = x * x;
    float y2 = y * y;
    float xy = x * y;

    float x3  = x2 * x;
    float y3  = y2 * y;
    float xy2 = x * y2;
    float x2y = x2 * y;

    float x4   = x3 * x;
    float y4   = y3 * y;
    float xy3  = x * y3;
    float x2y2 = x2 * y2;
    float x3y  = x3 * y;

    float x5   = x4 * x;
    float y5   = y4 * y;
    float xy4  = x * y4;
    float x2y3 = x2 * y3;
    float x3y2 = x3 * y2;
    float x4y  = x4 * y;

    GpuFieldValue B;

    B.x = cx[0] + cx[1] * x + cx[2] * y + cx[3] * x2 + cx[4] * xy + cx[5] * y2 + cx[6] * x3 + cx[7] * x2y + cx[8] * xy2
          + cx[9] * y3 + cx[10] * x4 + cx[11] * x3y + cx[12] * x2y2 + cx[13] * xy3 + cx[14] * y4 + cx[15] * x5
          + cx[16] * x4y + cx[17] * x3y2 + cx[18] * x2y3 + cx[19] * xy4 + cx[20] * y5;

    B.y = cy[0] + cy[1] * x + cy[2] * y + cy[3] * x2 + cy[4] * xy + cy[5] * y2 + cy[6] * x3 + cy[7] * x2y + cy[8] * xy2
          + cy[9] * y3 + cy[10] * x4 + cy[11] * x3y + cy[12] * x2y2 + cy[13] * xy3 + cy[14] * y4 + cy[15] * x5
          + cy[16] * x4y + cy[17] * x3y2 + cy[18] * x2y3 + cy[19] * xy4 + cy[20] * y5;

    B.z = cz[0] + cz[1] * x + cz[2] * y + cz[3] * x2 + cz[4] * xy + cz[5] * y2 + cz[6] * x3 + cz[7] * x2y + cz[8] * xy2
          + cz[9] * y3 + cz[10] * x4 + cz[11] * x3y + cz[12] * x2y2 + cz[13] * xy3 + cz[14] * y4 + cz[15] * x5
          + cz[16] * x4y + cz[17] * x3y2 + cz[18] * x2y3 + cz[19] * xy4 + cz[20] * y5;
    return B;
  }

  XPU_D inline GpuFieldValue GpuFieldSlice::GetFieldValueForLine(const kf::TrackParamBase<float>& t) const
  {
    float dz = z - t.GetZ();
    return GetFieldValue(t.GetX() + t.GetTx() * dz, t.GetY() + t.GetTy() * dz);
  }

  class GpuFieldRegion {
   public:
    GpuFieldRegion() = default;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    GpuFieldRegion(const kf::FieldRegion<DataIn>& other)
      : cx0(kf::utils::simd::Cast<DataIn, float>(other.cx0))
      , cx1(kf::utils::simd::Cast<DataIn, float>(other.cx1))
      , cx2(kf::utils::simd::Cast<DataIn, float>(other.cx2))
      , cy0(kf::utils::simd::Cast<DataIn, float>(other.cy0))
      , cy1(kf::utils::simd::Cast<DataIn, float>(other.cy1))
      , cy2(kf::utils::simd::Cast<DataIn, float>(other.cy2))
      , cz0(kf::utils::simd::Cast<DataIn, float>(other.cz0))
      , cz1(kf::utils::simd::Cast<DataIn, float>(other.cz1))
      , cz2(kf::utils::simd::Cast<DataIn, float>(other.cz2))
      , z0(kf::utils::simd::Cast<DataIn, float>(other.z0))
      //     , fUseOriginalField(other.fUseOriginalField)
      , fIsZeroField(other.fIsZeroField)
    {
    }

    /// Gets the field vector and writes it into B pointer
    /// \param x  x-coordinate of the point to calculate the field
    /// \param y  y-coordinate of the point to calculate the field
    /// \param z  z-coordinate of the point to calculate the field
    /// \return    the magnetic field value
    XPU_D GpuFieldValue Get([[maybe_unused]] float x, [[maybe_unused]] float y, float z) const
    {
      GpuFieldValue B;
      float dz  = z - z0;
      float dz2 = dz * dz;
      B.x       = cx0 + cx1 * dz + cx2 * dz2;
      B.y       = cy0 + cy1 * dz + cy2 * dz2;
      B.z       = cz0 + cz1 * dz + cz2 * dz2;
      return B;
    }

    /// Is the field region empty?
    XPU_D bool IsZeroField() const { return fIsZeroField; }

    /// Interpolates the magnetic field by three nodal points and sets the result to this FieldRegion object
    /// The result is a quadratic interpolation of the field as a function of z
    /// \param b0   field value in the first nodal point
    /// \param b0z  z-coordinate of the first nodal point
    /// \param b1   field value in the second nodal point
    /// \param b1z  z-coordinate of the second nodal point
    /// \param b2   field value in the third nodal point
    /// \param b2z  z-coordinate of the third nodal point
    /// TODO: does the sequence of b0z, b1z and b2z matter? If yes, probalby we need an assertion (S.Zharko)
    XPU_D void Set(const GpuFieldValue& b0, float b0z, const GpuFieldValue& b1, float b1z, const GpuFieldValue& b2,
                   float b2z);

    /// Interpolates the magnetic field by thwo nodal points and sets the result to this FieldRegion object.
    /// The result is a linear interpolation of the field as a function of z
    /// \param b0   field value in the first nodal point
    /// \param b0z  z-coordinate of the first nodal point
    /// \param b1   field value in the second nodal point
    /// \param b1z  z-coordinate of the second nodal point
    /// TODO: does the sequence of b0z and b1z matter? If yes, probalby we need an assertion (S.Zharko)
    XPU_D void Set(const GpuFieldValue& b0, float b0z, const GpuFieldValue& b1, float b1z)
    {
      fIsZeroField = (b0.IsZeroField() && b1.IsZeroField());
      z0           = b0z;
      float dzi    = 1. / (b1z - b0z);  //TODO
      cx0          = b0.x;
      cy0          = b0.y;
      cz0          = b0.z;
      cx1          = (b1.x - b0.x) * dzi;
      cy1          = (b1.y - b0.y) * dzi;
      cz1          = (b1.z - b0.z) * dzi;
      cx2          = 0.f;
      cy2          = 0.f;
      cz2          = 0.f;
    }

    /// Shifts the coefficients to new central point
    /// \param z  z-coordinate of the new central point
    XPU_D void Shift(float z)
    {
      float dz    = z - z0;
      float cx2dz = cx2 * dz;
      float cy2dz = cy2 * dz;
      float cz2dz = cz2 * dz;
      z0          = z;
      cx0 += (cx1 + cx2dz) * dz;
      cy0 += (cy1 + cy2dz) * dz;
      cz0 += (cz1 + cz2dz) * dz;
      cx1 += cx2dz + cx2dz;
      cy1 += cy2dz + cy2dz;
      cz1 += cz2dz + cz2dz;
    }


   public:
    // TODO: Probably it's better to have arrays instead of separate fvec values? (S.Zharko)
    // Bx(z) = cx0 + cx1*(z-z0) + cx2*(z-z0)^2
    float cx0;
    float cx1;
    float cx2;

    // By(z) = cy0 + cy1*(z-z0) + cy2*(z-z0)^2
    float cy0;
    float cy1;
    float cy2;

    // Bz(z) = cz0 + cz1*(z-z0) + cz2*(z-z0)^2
    float cz0;
    float cz1;
    float cz2;

    float z0;  ///< z-coordinate of the field region central point

    bool fIsZeroField;  //{false};  ///< Is the field region empty?
  };

  XPU_D inline void GpuFieldRegion::Set(const GpuFieldValue& b0, float b0z, const GpuFieldValue& b1, float b1z,
                                        const GpuFieldValue& b2, float b2z)
  {
    fIsZeroField = (b0.IsZeroField() && b1.IsZeroField() && b2.IsZeroField());

    z0        = b0z;
    float dz1 = b1z - b0z;
    float dz2 = b2z - b0z;
    float det = 1. / (dz1 * dz2 * (b2z - b1z));  //TODO

    float w21 = -dz2 * det;
    float w22 = dz1 * det;
    float w11 = -dz2 * w21;
    float w12 = -dz1 * w22;

    float db1 = b1.x - b0.x;
    float db2 = b2.x - b0.x;
    cx0       = b0.x;
    cx1       = db1 * w11 + db2 * w12;
    cx2       = db1 * w21 + db2 * w22;

    db1 = b1.y - b0.y;
    db2 = b2.y - b0.y;
    cy0 = b0.y;
    cy1 = db1 * w11 + db2 * w12;
    cy2 = db1 * w21 + db2 * w22;

    db1 = b1.z - b0.z;
    db2 = b2.z - b0.z;
    cz0 = b0.z;
    cz1 = db1 * w11 + db2 * w12;
    cz2 = db1 * w21 + db2 * w22;
  }

}  // namespace cbm::algo::ca
