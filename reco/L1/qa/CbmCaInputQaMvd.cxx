/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaInputQaMvd.cxx
/// @date   10.09.2023
/// @brief  QA-task for CA tracking input from MVD (implementation)
/// @author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaMvd.h"

#include "CbmMvdTrackingInterface.h"

ClassImp(CbmCaInputQaMvd);

// ---------------------------------------------------------------------------------------------------------------------
//
CbmCaInputQaMvd::CbmCaInputQaMvd(int verbose, bool isMCUsed) : CbmCaInputQaBase("CbmCaInputQaMvd", verbose, isMCUsed)
{
  // Default parameters of task
  DefineParameters();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaMvd::DefineParameters()
{
  auto SetRange = [](std::array<double, 2>& range, double min, double max) {
    range[0] = min;
    range[1] = max;
  };
  // Hit errors
  SetRange(fRHitDx, -0.005, 0.005);  // [cm]
  SetRange(fRHitDy, -0.005, 0.005);  // [cm]
  SetRange(fRHitDu, -0.005, 0.005);  // [cm]
  SetRange(fRHitDv, -0.005, 0.005);  // [cm]
  SetRange(fRHitDt, -10., 10.);      // [ns]
  // Residuals
  SetRange(fRResX, -0.004, 0.004);
  SetRange(fRResY, -0.004, 0.004);
  SetRange(fRResU, -0.004, 0.004);
  SetRange(fRResV, -0.004, 0.004);
  SetRange(fRResT, -5.0, 5.0);
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmCaInputQaMvd::InitQa()
{
  // Specific initialization
  fpDetInterface = CbmMvdTrackingInterface::Instance();

  return CbmCaInputQaBase::InitQa();
}
