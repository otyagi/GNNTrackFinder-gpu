/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file KfMeasurementTime.cxx
/// \brief Implementation of the KfMeasurementTime class

#include "KfMeasurementTime.h"

#include <iomanip>
#include <sstream>  // for stringstream

namespace cbm::algo::kf
{
  //----------------------------------------------------------------------------------------------------------------------
  //
  template<typename DataT>
  std::string MeasurementTime<DataT>::ToString(int indentLevel) const
  {
    std::stringstream aStream{};
    // TODO: possibly it is better to place the indentChar into ca::Parameters (S.Zharko)
    constexpr char indentChar = '\t';
    std::string indent(indentLevel, indentChar);
    aStream << indent << "t:    " << std::setw(12) << std::setfill(' ') << T() << '\n';
    aStream << indent << "dt2:  " << std::setw(12) << std::setfill(' ') << Dt2() << '\n';
    aStream << indent << "ndfT: " << std::setw(12) << std::setfill(' ') << NdfT() << '\n';
    return aStream.str();
  }

  ///----------------------------------------------------------------------------------------------------------------------
  /// All necessary instantiations of the template class

  template class MeasurementTime<fvec>;
  template class MeasurementTime<float>;
  template class MeasurementTime<double>;

}  // namespace cbm::algo::kf
