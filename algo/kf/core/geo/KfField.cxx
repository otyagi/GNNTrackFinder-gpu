/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfField.cxx
/// \brief  Magnetic field representation in KF (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  22.07.2024

#include "KfField.h"

#include <mutex>
#include <sstream>
#include <tuple>

using cbm::algo::kf::EFieldMode;
using cbm::algo::kf::Field;
using cbm::algo::kf::FieldFactory;
using cbm::algo::kf::detail::FieldBase;

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
std::string FieldBase<T, EFieldMode::Intrpl>::ToString(int indentLevel, int verbose) const
{
  constexpr char IndentChar = '\t';
  std::stringstream msg;
  std::string indent(indentLevel, IndentChar);
  msg << indent << "Field slices:";
  for (int iSlice = 0; iSlice < GetNofFieldSlices(); ++iSlice) {
    const auto& fldSlice = fvFieldSlices[iSlice];
    msg << '\n' << indent << iSlice << ") " << fldSlice.ToString(0, verbose);
  }
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
std::string FieldBase<T, EFieldMode::Orig>::ToString(int indentLevel, int /*verbose*/) const
{
  constexpr char IndentChar = '\t';
  std::stringstream msg;
  std::string indent(indentLevel, IndentChar);
  msg << indent << "Original field function";
  msg << indent << "Field slice z-positions: ";
  for (int iSlice = 0; iSlice < GetNofFieldSlices(); ++iSlice) {
    msg << "\n " << indent << iSlice << ") " << fvFieldSliceZ[iSlice];
  }
  return msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
void Field<T>::RemoveSlice(int iLayer)
{
  if (fFieldMode == EFieldMode::Orig) {
    foFldOrig->RemoveSlice(iLayer);
  }
  else if (fFieldMode == EFieldMode::Intrpl) {
    foFldIntrpl->RemoveSlice(iLayer);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
std::string Field<T>::ToString(int indentLevel, int verbose) const
{
  constexpr char IndentChar = '\t';
  std::stringstream msg;
  std::string indent(indentLevel, IndentChar);
  msg << indent << "Field near primary vertex:\n" << this->fPrimVertexField.ToString(indentLevel + 1, verbose) << '\n';
  msg << indent << "Field type: " << static_cast<int>(this->fFieldType) << '\n';
  msg << indent << "Field mode: " << static_cast<int>(this->fFieldMode) << '\n';
  if (fFieldMode == EFieldMode::Orig) {
    msg << foFldOrig->ToString(indentLevel, verbose);
  }
  else if (fFieldMode == EFieldMode::Intrpl) {
    msg << foFldIntrpl->ToString(indentLevel, verbose);
  }
  return msg.str();
}

namespace cbm::algo::kf
{
  template class Field<float>;
  template class Field<double>;
  template class Field<fvec>;
}  // namespace cbm::algo::kf


// ---------------------------------------------------------------------------------------------------------------------
//
void FieldFactory::AddSliceReference(double halfSizeX, double halfSizeY, double zRef)
{
  if (!fSliceReferences.emplace(halfSizeX, halfSizeY, zRef).second) {
    std::stringstream msg;
    msg << "FieldFactory::AddReference: attempt of adding another slice reference with zRef = " << zRef
        << "(halfSizeX = " << halfSizeX << ", halfSizeY = " << halfSizeY << ").\nThe next slice references were "
        << "added:";
    for (const auto& el : fSliceReferences) {
      msg << "\n\t- halfSizeX = " << el.fHalfSizeX << ", halfSizeY = " << el.fHalfSizeY << ", zRef = " << el.fRefZ;
    }
    throw std::logic_error(msg.str());
  }
}

