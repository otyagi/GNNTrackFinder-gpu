/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaTimesliceHeader.h
/// \brief  A structure to keep all the common information on the timeslice coming from tracking
/// \since  15.02.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include <string>

namespace cbm::algo::ca
{
  /// \class TimesliceHeader
  /// \brief Structure for keeping the current information on the timeslice
  class TimesliceHeader {
   public:
    /// \brief Accesses the end of timeslice [ns]
    float& End() { return fEnd; }
    float End() const { return fEnd; }

    /// \brief Accesses the start of timeslice [ns]
    float& Start() { return fStart; }
    float Start() const { return fStart; }

    /// \brief String representation of the contents
    std::string ToString() const;

    /// \brief Converts time from ns to tau
    float ConvToTau(float time) const { return (time - fStart) / (fEnd - fStart); }

    /// \brief Converts time from tau to ns
    float ConvToNs(float tau) const { return fStart + tau * (fEnd - fStart); }

   private:
    float fStart;  ///< Start of timeslice
    float fEnd;    ///< End of timeslice
  };
}  // namespace cbm::algo::ca
