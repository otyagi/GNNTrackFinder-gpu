/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaInputQaTrd.cxx
/// @date   13.01.2023
/// @brief  QA-task for CA tracking input from TRD detector (implementation)
/// @author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaTrd.h"

#include "CbmTrdTrackingInterface.h"

ClassImp(CbmCaInputQaTrd);

// ---------------------------------------------------------------------------------------------------------------------
//
CbmCaInputQaTrd::CbmCaInputQaTrd(int verbose, bool isMCUsed) : CbmCaInputQaBase("CbmCaInputQaTrd", verbose, isMCUsed)
{
  // Default parameters of task
  DefineParameters();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaTrd::DefineParameters()
{
  auto SetRange = [](std::array<double, 2>& range, double min, double max) {
    range[0] = min;
    range[1] = max;
  };
  // Hit errors
  SetRange(fRHitDx, 0.0000, 5.00);    // [cm]
  SetRange(fRHitDy, 0.0000, 5.00);    // [cm]
  SetRange(fRHitDu, 0.0000, 5.00);    // [cm]
  SetRange(fRHitDv, 0.0000, 5.00);    // [cm]
  SetRange(fRHitDt, 0.0000, 100.00);  // [ns]
  // Residuals
  SetRange(fRResX, -10.00, 10.00);
  SetRange(fRResY, -10.00, 10.00);
  SetRange(fRResU, -2.00, 2.00);
  SetRange(fRResV, -10.00, 10.00);
  SetRange(fRResT, -200.0, 200.0);
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmCaInputQaTrd::InitQa()
{
  // Specific initialization
  fpDetInterface = CbmTrdTrackingInterface::Instance();

  return CbmCaInputQaBase::InitQa();
}
