/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfSetup.cxx
/// @brief  Setup representation for the Kalman-filter framework (source)
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "KfSetup.h"

#include <sstream>
#include <typeinfo>

using cbm::algo::kf::Setup;

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename T>
std::string Setup<T>::ToString(int verbosity, int indentLevel) const
{
  std::stringstream msg;
  if (verbosity > 0) {
    constexpr char indentCh = '\t';
    std::string indent(indentLevel, indentCh);
    msg << indent << "\nFloating-point type: " << typeid(T).name();
    msg << indent << "\nTARGET:\n" << fTarget.ToString(indentLevel + 1, verbosity);
    msg << indent << "\nMODULE INDEXING SCHEME:\n" << fModuleIndexMap.ToString(indentLevel + 1);
    msg << indent << "\nMATERIAL LAYERS:\n";
    for (const auto& layer : fvMaterialLayers) {
      msg << layer.ToString(indentLevel + 1, verbosity) << '\n';
    }
    msg << indent << "\nMAGNETIC FIELD:\n" << fField.ToString(indentLevel + 1, verbosity);
  }
  return msg.str();
}

namespace cbm::algo::kf
{
  template class Setup<float>;
  template class Setup<double>;
  template class Setup<fvec>;
}  // namespace cbm::algo::kf
