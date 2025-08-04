/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file    CaWindowData.h
/// \author  Sergei Zharko <s.zharko@gsi.de>
/// \brief   Container for all data, which are processed within a single sub-timeslice (implementation)
/// \since   29.01.2024

#include "CaWindowData.h"

using cbm::algo::ca::WindowData;

// ---------------------------------------------------------------------------------------------------------------------
//
void WindowData::ResetHitData(int nHits)
{
  fvHits.reset(nHits);
  fvbHitSuppressed.reset(nHits, 0);
}
