/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfFieldRegion.cxx
/// \brief  Magnetic flux density interpolation along the track vs. z-coordinate (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  22.07.2024
///

#include "KfFieldRegion.h"

#include "KfUtils.h"

#include <mutex>
#include <sstream>

#include <fmt/format.h>

namespace cbm::algo::kf
{
  using detail::FieldRegionBase;
  FieldFn_t GlobalField::fgOriginalField{&GlobalField::UndefField};
  EFieldType GlobalField::fgOriginalFieldType{EFieldType::Null};
  bool GlobalField::fgForceUseOfOriginalField{false};

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldRegionBase<T, EFieldMode::Intrpl>::Set(const FieldValue<T>& b0, const T& z0, const FieldValue<T>& b1,
                                                   const T& z1, const FieldValue<T>& b2, const T& z2)
  {
    fZfirst = z0;

    auto dz1 = z1 - z0;
    auto dz2 = z2 - z0;
    auto det = utils::simd::One<T>() / (dz1 * dz2 * (z2 - z1));
    auto w21 = -dz2 * det;
    auto w22 = dz1 * det;
    auto w11 = -dz2 * w21;
    auto w12 = -dz1 * w22;

    for (int iD = 0; iD < 3; ++iD) {
      auto db1    = b1.GetComponent(iD) - b0.GetComponent(iD);
      auto db2    = b2.GetComponent(iD) - b0.GetComponent(iD);
      auto& coeff = fCoeff[iD];
      coeff[0]    = b0.GetComponent(iD);
      coeff[1]    = db1 * w11 + db2 * w12;
      coeff[2]    = db1 * w21 + db2 * w22;
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  FieldValue<T> FieldRegionBase<T, EFieldMode::Intrpl>::Get(const T& x, const T& y, const T& z) const
  {
    if (GlobalField::IsUsingOriginalFieldForced()) {
      return GlobalField::GetFieldValue(GlobalField::fgOriginalField, x, y, z);
    }
    auto dz = z - this->fZfirst;
    return FieldValue<T>{this->fCoeff[0][0] + dz * (this->fCoeff[0][1] + dz * this->fCoeff[0][2]),
                         this->fCoeff[1][0] + dz * (this->fCoeff[1][1] + dz * this->fCoeff[1][2]),
                         this->fCoeff[2][0] + dz * (this->fCoeff[2][1] + dz * this->fCoeff[2][2])};
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  std::tuple<T, T, T>
  FieldRegionBase<T, EFieldMode::Intrpl>::GetDoubleIntegrals(const T& /*x1*/, const T& /*y1*/, const T& z1,  //
                                                             const T& /*x2*/, const T& /*y2*/, const T& z2) const
  {
    // double integral of the field along z

    if (GlobalField::IsUsingOriginalFieldForced()) {
      // TODO: implement the double integral for the original field
    }
    auto fld = *this;
    fld.Shift(z1);
    T dz  = z2 - z1;
    T dz2 = dz * dz;
    T c0  = dz2 * T(1. / 2.);
    T c1  = dz2 * dz * T(1. / 6.);
    T c2  = dz2 * dz2 * T(1. / 12.);
    return {c0 * fld.fCoeff[0][0] + c1 * fld.fCoeff[0][1] + c2 * fld.fCoeff[0][2],
            c0 * fld.fCoeff[1][0] + c1 * fld.fCoeff[1][1] + c2 * fld.fCoeff[1][2],
            c0 * fld.fCoeff[2][0] + c1 * fld.fCoeff[2][1] + c2 * fld.fCoeff[2][2]};
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldRegionBase<T, EFieldMode::Intrpl>::Shift(const T& z)
  {
    auto dz = z - fZfirst;
    for (int iD = 0; iD < 3; ++iD) {
      auto& coeff = fCoeff[iD];
      auto c2dz   = coeff[2] * dz;
      coeff[0] += (coeff[1] + c2dz) * dz;
      coeff[1] += (2 * c2dz);
    }
    fZfirst = z;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldRegionBase<T, EFieldMode::Intrpl>::CheckConsistency() const
  {
    // Check SIMD data vectors for consistent initialization
    for (int iD = 0; iD < 3; ++iD) {
      for (int iC = 0; iC < 3; ++iC) {
        utils::CheckSimdVectorEquality(fCoeff[iD][iC], fmt::format("FieldRegion::fCoeff[{}][{}]", iD, iC).c_str());
      }
    }
    utils::CheckSimdVectorEquality(fZfirst, "FieldRegion::fZfirst");
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  std::string FieldRegionBase<T, EFieldMode::Intrpl>::ToString(int indentLevel, int) const
  {
    constexpr char IndentChar = '\t';
    std::stringstream msg;
    std::string indent(indentLevel, IndentChar);
    using fmt::format;
    auto Cnv = [&](const auto& val) { return utils::simd::Cast<T, Literal_t<T>>(val); };  // alias for the conversion fn
    const auto& coeff = this->fCoeff;
    msg << indent << format("Field Region: dz = z - ({})", Cnv(this->fZfirst));
    msg << '\n'
        << indent << IndentChar
        << format("Bx(dz) = {:>12} + {:>12} dz + {:>12} dz2", Cnv(coeff[0][0]), Cnv(coeff[0][1]), Cnv(coeff[0][2]));
    msg << '\n'
        << indent << IndentChar
        << format("Bx(dz) = {:>12} + {:>12} dz + {:>12} dz2", Cnv(coeff[1][0]), Cnv(coeff[1][1]), Cnv(coeff[1][2]));
    msg << '\n'
        << indent << IndentChar
        << format("Bx(dz) = {:>12} + {:>12} dz + {:>12} dz2", Cnv(coeff[2][0]), Cnv(coeff[2][1]), Cnv(coeff[2][2]));
    if (GlobalField::IsUsingOriginalFieldForced()) {
      msg << indent
          << "\nWARNING: the GlobalField::ForceUseOfOriginalField() is enabled, so the magnetic field interpolation "
          << indent
          << "\nis replaced with the global magnetic function for debugging purposes. If you see this message and are "
          << indent
          << "\nnot sure, what is going on, please call GlobalField::ForceUseOfOriginalField(false) and re-run the "
             "routine";
    }
    return msg.str();
  }


  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldRegionBase<T, EFieldMode::Orig>::CheckConsistency() const
  {
    // Check SIMD data vectors for consistent initialization
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  std::string FieldRegionBase<T, EFieldMode::Orig>::ToString(int indentLevel, int) const
  {
    constexpr char IndentChar = '\t';
    std::stringstream msg;
    std::string indent(indentLevel, IndentChar);
    msg << indent << "Field region: created from the original field function";
    return msg.str();
  }


  namespace detail
  {
    template class FieldRegionBase<float, EFieldMode::Intrpl>;
    template class FieldRegionBase<double, EFieldMode::Intrpl>;
    template class FieldRegionBase<fvec, EFieldMode::Intrpl>;
    template class FieldRegionBase<float, EFieldMode::Orig>;
    template class FieldRegionBase<double, EFieldMode::Orig>;
    template class FieldRegionBase<fvec, EFieldMode::Orig>;
  }  // namespace detail


  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldRegion<T>::Shift(const T& z)
  {
    if (fFieldMode == EFieldMode::Intrpl) {
      foFldIntrpl->Shift(z);
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  std::string FieldRegion<T>::ToString(int indentLevel, int) const
  {
    constexpr char IndentChar = '\t';
    std::stringstream msg;
    std::string indent(indentLevel, IndentChar);
    msg << indent << "FieldType: " << static_cast<int>(fFieldType) << '\n';
    msg << indent << "FieldMode: " << static_cast<int>(fFieldMode) << '\n';
    msg << indent
        << (fFieldMode == EFieldMode::Intrpl ? foFldIntrpl->ToString(indentLevel) : foFldOrig->ToString(indentLevel));
    return msg.str();
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldRegion<T>::CheckConsistency() const
  {
    // Check SIMD data vectors for consistent initialization

    if (fFieldMode == EFieldMode::Intrpl) {
      foFldIntrpl->CheckConsistency();
    }
    else {
      foFldOrig->CheckConsistency();
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  std::tuple<double, double, double> GlobalField::UndefField(double, double, double)
  {
    assert(!GlobalField::IsUsingOriginalFieldForced()
           && "cbm::algo::kf::GlobalField: The original globa magnetic field is required by "
              "kf::defs::dbg::ForceOriginalField = true. Please provide it with the "
              "kf::GlobalField::SetGlobalField() function.");
    return std::make_tuple(defs::Undef<double>, defs::Undef<double>, defs::Undef<double>);
  }

  template class FieldRegion<float>;
  template class FieldRegion<double>;
  template class FieldRegion<fvec>;
}  // namespace cbm::algo::kf
