/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmEventTriggers.h
/// \brief  A structure to store different triggers in parallel to the CbmEvent (implementation)
/// \since  25.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmEventTriggers.h"

#include <sstream>

// ---------------------------------------------------------------------------------------------------------------------
//
std::string CbmEventTriggers::ToString() const
{
  std::stringstream msg;
  msg << "CbmEventTriggers: Lambda " << Test(ETrigger::Lambda) << ", Ks " << Test(ETrigger::Ks);
  return msg.str();
}

#ifndef NO_ROOT
ClassImp(CbmEventTriggers)
#endif
