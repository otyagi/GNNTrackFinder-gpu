/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfTarget.cxx
/// @brief  A target layer in the KF-setup (implementation)
/// @since  19.07.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "KfTarget.h"

#include <sstream>

using cbm::algo::kf::MaterialMap;
using cbm::algo::kf::Target;

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
Target<T>::Target(double x, double y, double z, double dz, double r)
  : fX(utils::simd::Cast<double, T>(x))
  , fY(utils::simd::Cast<double, T>(y))
  , fZ(utils::simd::Cast<double, T>(z))
  , fDz(utils::simd::Cast<double, T>(dz))
  , fR(utils::simd::Cast<double, T>(r))
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
void Target<T>::SetMaterial(const MaterialMap& material)
{
  if (material.IsUndefined()) {
    throw std::logic_error("Target:ReceiveMaterial(): attempt to pass an undefined instance of the material map");
  }
  fMaterial = material;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
void Target<T>::SetMaterial(MaterialMap&& material)
{
  if (material.IsUndefined()) {
    throw std::logic_error("Target:ReceiveMaterial(): attempt to pass an undefined instance of the material map");
  }
  fMaterial = std::move(material);
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
std::string Target<T>::ToString(int indentLevel, int verbose) const
{
  constexpr char IndentChar = '\t';
  std::stringstream msg;
  std::string indent(indentLevel, IndentChar);
  auto Cnv = [&](const auto& val) { return utils::simd::Cast<T, Literal_t<T>>(val); };  // alias for the conversion fn
  msg << indent << "position: {" << Cnv(fX) << ", " << Cnv(fY) << ", " << Cnv(fZ) << "} [cm]\n";
  msg << indent << "half-thickness: " << Cnv(fDz) << " [cm]\n";
  msg << indent << "material:\n" << indent << IndentChar << "  " << fMaterial.ToString(verbose);
  return msg.str();
}


namespace cbm::algo::kf
{
  template class Target<float>;
  template class Target<double>;
  template class Target<fvec>;
}  // namespace cbm::algo::kf
