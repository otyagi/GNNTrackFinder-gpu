/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file KfMeasurementXy.cxx
/// \brief Implementation of the KfMeasurementXy class

#include "KfMeasurementXy.h"

#include <iomanip>
#include <sstream>  // for stringstream

namespace cbm::algo::kf
{

  //----------------------------------------------------------------------------------------------------------------------
  //
  template<typename DataT>
  std::string MeasurementXy<DataT>::ToString(int indentLevel) const
  {
    std::stringstream aStream{};
    // TODO: possibly it is better to place the indentChar into ca::Parameters (S.Zharko)
    constexpr char indentChar = '\t';
    std::string indent(indentLevel, indentChar);
    aStream << indent << "x:    " << std::setw(12) << std::setfill(' ') << X() << '\n';
    aStream << indent << "y:    " << std::setw(12) << std::setfill(' ') << Y() << '\n';
    aStream << indent << "dx2:  " << std::setw(12) << std::setfill(' ') << Dx2() << '\n';
    aStream << indent << "dy2:  " << std::setw(12) << std::setfill(' ') << Dy2() << '\n';
    aStream << indent << "dxy:  " << std::setw(12) << std::setfill(' ') << Dxy() << '\n';
    aStream << indent << "ndfX: " << std::setw(12) << std::setfill(' ') << NdfX() << '\n';
    aStream << indent << "ndfY: " << std::setw(12) << std::setfill(' ') << NdfY() << '\n';
    return aStream.str();
  }

  ///----------------------------------------------------------------------------------------------------------------------
  /// All necessary instantiations of the template class

  template class MeasurementXy<fvec>;
  template class MeasurementXy<float>;
  template class MeasurementXy<double>;

}  // namespace cbm::algo::kf
