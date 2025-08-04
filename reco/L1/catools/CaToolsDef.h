/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CaToolsDef.h
/// @brief  Definitions for ca::tools namespace
/// @since  16.09.2023
/// @author S.Zharko <s.zharko@gsi.de>

#ifndef CaToolsDef_h
#define CaToolsDef_h 1

#include "CaDefs.h"

namespace cbm::ca::tools
{
  namespace constants = cbm::algo::ca::constants;
  namespace phys      = cbm::algo::ca::constants::phys;
  namespace clrs      = cbm::algo::ca::constants::clrs;
  namespace ca        = cbm::algo::ca;
}  // namespace cbm::ca::tools

#endif  // CaToolsDef_h
