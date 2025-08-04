/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmBasicFemtoPairCut.h"

#include "HalCbmDeltaPhiDeltaThetaCut.h"
#include "HalCbmStsSepCut.h"

#include <iostream>

#include <Hal/CutMonitorX.h>
#include <Hal/CutMonitorXY.h>

HalCbmBasicFemtoPairCut::HalCbmBasicFemtoPairCut()
  : fDeltaEtaAx("HalCbmDeltaPhiDeltaThetaStarCut", 1, 100, -0.1, 0.1)
  , fDeltaPhiStarAx("HalCbmDeltaPhiDeltaThetaStarCut", 0, 100, -0.1, 0.1)
  , fStsExitSepAx("HalCbmStsExitSepCut", 0, 100, 0, 10)
{
  AddCut(HalCbmStsExitSepCut());
  AddCut(HalCbmDeltaPhiDeltaThetaStarCut());
  SetSeparationMonitorAxis(100, 0, 10);
  SetDeltaPhiStarAxis(100, -0.1, 0.1);
  SetDeltaEtaStarAxis(100, -0.1, 0.1);
}

void HalCbmBasicFemtoPairCut::SetDeltaPhiStarCut(Double_t min, Double_t max)
{
  GetDeltaPhiEtaStarCut()->SetMinMax(min, max, 0);
}

void HalCbmBasicFemtoPairCut::SetDeltaEtaStarCut(Double_t min, Double_t max)
{
  GetDeltaPhiEtaStarCut()->SetMinMax(min, max, 1);
}

void HalCbmBasicFemtoPairCut::SetR(Double_t R) { GetDeltaPhiEtaStarCut()->SetR(R); }

void HalCbmBasicFemtoPairCut::SetStsExitSeparationCut(Double_t min, Double_t max)
{
  GetStsExitCut()->SetMinMax(min, max);
}
/*
void HalCbmBasicFemtoPairCut::CreateBasicMonitors()
{
  TString opt    = "";
  Int_t step     = 0;
  TString params = GetGlobalCutOption();
  if (Hal::Std::FindParam(params, "re")) { opt = "re"; }
  if (Hal::Std::FindParam(params, "im")) {
    opt  = "im";
    step = 1;
  }
  Hal::CutMonitorX exitM(GetStsExitCut()->CutName(opt), step);
  exitM.SetXaxis(fStsExitSepAx.GetNBins(), fStsExitSepAx.GetMin(), fStsExitSepAx.GetMax());
  AddCutMonitor(exitM);
  Hal::CutMonitorXY phiM(GetDeltaPhiEtaStarCut()->CutName(opt), 0 + step, GetDeltaPhiEtaStarCut()->CutName(opt),
                         1 + step);
  phiM.SetXaxis(fDeltaPhiStarAx.GetNBins(), fDeltaPhiStarAx.GetMin(), fDeltaPhiStarAx.GetMax());
  phiM.SetYaxis(fDeltaEtaAx.GetNBins(), fDeltaEtaAx.GetMin(), fDeltaEtaAx.GetMax());
  AddCutMonitor(phiM);
}
*/
HalCbmBasicFemtoPairCut::~HalCbmBasicFemtoPairCut() {}

void HalCbmBasicFemtoPairCut::AddAllCutMonitorRequests(Option_t* opt)
{
  TString option = opt;
  std::cout << "ENABLED ALL" << std::endl;
  if (Hal::Std::FindParam(option, "all")) {
    std::cout << "ENABLED ALL" << std::endl;
    AddCutMonitorRequest(fStsExitSepAx);
    AddCutMonitorRequest(fDeltaPhiStarAx, fDeltaEtaAx);
    return;
  }
  if (Hal::Std::FindParam(option, "exit")) AddCutMonitorRequest(fStsExitSepAx);
  if (Hal::Std::FindParam(option, "phistar")) AddCutMonitorRequest(fDeltaPhiStarAx, fDeltaEtaAx);
}
