/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file KfMeasurementU.cxx
/// \brief Implementation of the KfMeasurementU class

#include "KfMeasurementU.h"

#include <iomanip>
#include <sstream>

namespace cbm::algo::kf
{

  //----------------------------------------------------------------------------------------------------------------------
  //
  template<typename DataT>
  std::string MeasurementU<DataT>::ToString(int indentLevel) const
  {
    std::stringstream aStream{};
    // TODO: possibly it is better to place the indentChar into ca::Parameters (S.Zharko)
    constexpr char indentChar = '\t';
    std::string indent(indentLevel, indentChar);
    aStream << indent << "cos(phi):    " << std::setw(12) << std::setfill(' ') << CosPhi() << '\n';
    aStream << indent << "sin(phi):    " << std::setw(12) << std::setfill(' ') << SinPhi() << '\n';
    aStream << indent << "u:           " << std::setw(12) << std::setfill(' ') << U() << '\n';
    aStream << indent << "du2:         " << std::setw(12) << std::setfill(' ') << Du2() << '\n';
    aStream << indent << "ndf:         " << std::setw(12) << std::setfill(' ') << Ndf() << '\n';
    return aStream.str();
  }

  ///----------------------------------------------------------------------------------------------------------------------
  /// All necessary instantiations of the template class

  template class MeasurementU<fvec>;
  template class MeasurementU<float>;
  template class MeasurementU<double>;

}  // namespace cbm::algo::kf
