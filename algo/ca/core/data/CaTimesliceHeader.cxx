/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaTimesliceHeader.cxx
/// \brief  A structure to keep all the common information on the timeslice coming from tracking (implementation)
/// \since  15.02.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CaTimesliceHeader.h"

#include <sstream>

using cbm::algo::ca::TimesliceHeader;

// ---------------------------------------------------------------------------------------------------------------------
//
std::string TimesliceHeader::ToString() const
{
  std::stringstream msg;
  msg << "TimesliceHeader: start = " << fStart << " [ns], end = " << fEnd << " [ns]";
  return msg.str();
}
