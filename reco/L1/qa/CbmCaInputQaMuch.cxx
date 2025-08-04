/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaMuch.cxx
/// \date   13.01.2023
/// \brief  QA-task for CA tracking input from MuCh detector (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaMuch.h"

#include "CbmMuchTrackingInterface.h"

ClassImp(CbmCaInputQaMuch);

// ---------------------------------------------------------------------------------------------------------------------
//
CbmCaInputQaMuch::CbmCaInputQaMuch(int verbose, bool isMCUsed) : CbmCaInputQaBase("CbmCaInputQaMuch", verbose, isMCUsed)
{
  // Default parameters of task
  DefineParameters();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaMuch::DefineParameters()
{
  auto SetRange = [](std::array<double, 2>& range, double min, double max) {
    range[0] = min;
    range[1] = max;
  };
  // Hit errors
  SetRange(fRHitDx, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDy, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDu, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDv, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDt, 0.0000, 0.15);  // [ns]
  // Residuals
  SetRange(fRResX, -2.00, 2.00);
  SetRange(fRResY, -4.00, 4.00);
  SetRange(fRResU, -2.00, 2.00);
  SetRange(fRResV, -4.00, 4.00);
  SetRange(fRResT, -5.00, 5.00);
  SetRange(fRangeDzHitPoint, -50., 50.);
}


// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmCaInputQaMuch::InitQa()
{
  // Specific initialization
  fpDetInterface = CbmMuchTrackingInterface::Instance();

  return CbmCaInputQaBase::InitQa();
}
