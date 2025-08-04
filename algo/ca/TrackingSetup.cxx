/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingSetup.h
/// \date   19.04.2024
/// \brief  A detector setup interface used for tracking input data initialization (source)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "TrackingSetup.h"

#include "Definitions.h"

using cbm::algo::TrackingSetup;
using fles::Subsystem;

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingSetup::Init()
{
  if (fbUseSts) {
    fSts.SetContext(this->GetContext());
    fSts.Init();
  }
  if (fbUseTrd) {
    fTrd.SetContext(this->GetContext());
    fTrd.Init();
  }
  if (fbUseTof) {
    fTof.SetContext(this->GetContext());  // can be nullptr
    fTof.Init();
  }
}
