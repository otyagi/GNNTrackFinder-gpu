/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfFieldValue.cxx
/// \brief  Magnetic flux density vector representation (source)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  13.08.2024

#include "KfFieldValue.h"

#include <sstream>

namespace cbm::algo::kf
{

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  void FieldValue<T>::CheckConsistency() const
  {
    // Check SIMD data vectors for consistent initialization
    utils::CheckSimdVectorEquality(fB[0], "FieldValue::x");
    utils::CheckSimdVectorEquality(fB[1], "FieldValue::y");
    utils::CheckSimdVectorEquality(fB[2], "FieldValue::z");

    // TODO: Any other checks? (S.Zharko)
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  std::string FieldValue<T>::ToString(int indentLevel) const
  {
    constexpr char IndentChar = '\t';
    std::stringstream msg;
    std::string indent(indentLevel, IndentChar);
    msg << indent << "B = {" << fB[0] << ", " << fB[1] << ", " << fB[2] << "} [kG]";
    return msg.str();
  }

  template class FieldValue<float>;
  template class FieldValue<double>;
  template class FieldValue<fvec>;
}  // namespace cbm::algo::kf
