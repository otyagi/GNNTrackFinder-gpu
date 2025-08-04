/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

#include "CaStation.h"

#include "AlgoFairloggerCompat.h"

#include <iomanip>
#include <sstream>

namespace cbm::algo::ca
{
  // --------------------------------------------------------------------------------------------------------------------
  //
  template<typename DataT>
  void Station<DataT>::CheckConsistency() const
  {
    /// Integer fields initialization checks

    if (type < 0) {
      std::stringstream msg;
      msg << "CaStation: station type was not initialized (type = " << type << ", type > 0)";
      throw std::logic_error(msg.str());
    }

    if (timeInfo != 0 && timeInfo != 1) {
      std::stringstream msg;
      msg << "CaStation: illegal time information flag (timeInfo = " << timeInfo << ", "
          << "0 [time information is not used] or 1 [time information is used] expected)";
      throw std::logic_error(msg.str());
    }

    if (fieldStatus != 0 && fieldStatus != 1) {
      std::stringstream msg;
      msg << "CaStation: illegal field status flag (fieldStatus = " << fieldStatus << ", "
          << "0 [station is outside the field] or 1 [station is inside the field] expected";
      throw std::logic_error(msg.str());
    }

    /// SIMD vector checks: all the words in a SIMD vector must be equal
    // TODO: SZh 06.06.2024: Do we still need these checks?
    kfutils::CheckSimdVectorEquality<DataT>(fZ, "CaStation::fZ");
    kfutils::CheckSimdVectorEquality<DataT>(Xmax, "CaStation::Xmax");
    kfutils::CheckSimdVectorEquality<DataT>(Ymax, "CaStation::Ymax");


    /// Inner and outer radia checks:
    ///  (i)  both Xmax and Ymax must be > 0

    if (this->GetXmax<float>() < 0. || this->GetYmax<float>() < 0.) {
      std::stringstream msg;
      msg << "CaStation: " << this->ToString() << " has incorrect sizes: "
          << "Xmax = " << this->GetXmax<float>() << " [cm], Ymax = " << this->GetYmax<float>()
          << " [cm] (0 < Xmax && 0 < Ymax expected)";
      throw std::logic_error(msg.str());
    }

    /// Check consistency of other members

    //materialInfo.CheckConsistency();
    // TODO: Temporarily switched off, because Much has RL = 0, which is actually incorrect, but really is not used.
    //       One should provide average radiation length values for each Much layer (S.Zharko)
    fieldSlice.CheckConsistency();
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  template<typename DataT>
  std::string Station<DataT>::ToString(int verbosityLevel, int indentLevel, bool isHeader) const
  {
    std::stringstream msg{};
    constexpr char indentChar = '\t';
    constexpr char columnSize = 15;
    std::string indent(indentLevel, indentChar);
    if (verbosityLevel == 0) {
      msg << "station type = " << type << ", z = " << this->GetZ<float>() << " cm";
    }
    else {
      if (isHeader) {
        verbosityLevel = 0;
        msg << indent;
        msg << std::setw(columnSize) << std::setfill(' ') << "type" << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << "time status" << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << "field status" << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << "geo layer ID" << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << "z [cm]" << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << "Xmax [cm]" << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << "Ymax [cm]";
      }
      else {
        msg << indent;
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetType() << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetTimeStatus() << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetFieldStatus() << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetGeoLayerID() << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetZ<float>() << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetXmax<float>() << ' ';
        msg << std::setw(columnSize) << std::setfill(' ') << this->GetYmax<float>();
      }
      if (verbosityLevel > 3) {
        msg << indent << "\nField slices:\n";
        msg << fieldSlice.ToString(indentLevel + 1, verbosityLevel) << '\n';
      }
    }
    return msg.str();
  }

  template class Station<fvec>;
  template class Station<float>;
  template class Station<double>;
}  // namespace cbm::algo::ca
