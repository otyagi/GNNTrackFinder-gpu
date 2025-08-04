/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */

#ifndef __CLING__
#include "EventAna.h"
#include "EventImpactParameterCut.h"
#include "EventPhiCut.h"
#include "EventVertexCut.h"
#include "Femto1DCF.h"
#include "FemtoBasicAna.h"
#include "FemtoConst.h"
#include "FemtoCorrFuncKt.h"
#include "FemtoWeightGenerator.h"
#include "HalCbmBasicFemtoPairCut.h"
#include "HalCbmBasicTrackCuts.h"
#include "HalCbmFullEvent.h"
#include "HalCbmHbtFullEvent.h"
#include "TrackPdgCut.h"
#include "TwoTrackAna.h"

#include <RtypesCore.h>
#include <TMath.h>
#include <TString.h>

#include <Hal/CutMonitorX.h>
#include <Hal/CutMonitorXY.h>
#include <Hal/CutsAndMonitors.h>
#include <Hal/Std.h>

using namespace std;
#endif


void SetEventCut(Hal::EventAna* task)
{
  Hal::EventVertexCut vtx;
  task->AddCut(vtx, "im");
  Hal::CutMonitorXY vtxM(vtx.CutName("im"), vtx.Z(), vtx.CutName("im"), vtx.Rt());
  vtxM.SetXaxis(100, 0, 1);
  vtxM.SetYaxis(200, 0, 2);
  task->AddCutMonitor(vtxM);
  Hal::EventImpactParameterCut b;
  Hal::EventPhiCut phi;
  Hal::CutMonitorX phiM(phi.CutName("im"), 0);
  phiM.SetXaxis(100, -TMath::TwoPi(), TMath::TwoPi());
  Hal::CutMonitorX bM(b.CutName("im"), 0);
  bM.SetXaxis(100, 0, 15);
  task->AddCut(b, "im");
  task->AddCut(phi, "im");
  task->AddCutMonitor(phiM);
  task->AddCutMonitor(bM);
}

void SetTrackCuts(Hal::EventAna* task, Bool_t mc = kTRUE)
{
  HalCbmBasicTrackCuts basic;
  basic.SetCharge(-1);
  basic.SetM2(-0.1, 0.1);
  basic.SetPt(0.1, 10);
  basic.SetOptionForAllCuts("re");
  basic.MakeCutMonitors();  // must be called after SetOptionForAllCuts
  task->AddCutsAndMonitors(basic);
  Hal::TrackPdgCut pid;
  pid.SetMinAndMax(-211);
  if (mc) {
    task->AddCut(pid, "im");
  }
}

void SetPairCuts(Hal::TwoTrackAna* task)
{
  HalCbmBasicFemtoPairCut combinedCut;
  combinedCut.SetR(25);
  combinedCut.SetDeltaPhiStarCut(-0.01, 0.01);
  combinedCut.SetDeltaEtaStarCut(-0.01, 0.01);
  combinedCut.SetDeltaEtaStarAxis(100, -0.1, 0.1);
  combinedCut.SetDeltaPhiStarAxis(100, -0.1, 0.1);
  combinedCut.SetOptionForAllCuts("re");
  combinedCut.MakeCutMonitors();
  task->AddCutsAndMonitors(combinedCut);
}

Hal::FemtoBasicAna* MakeAna(Bool_t speedUp)
{
  Hal::FemtoBasicAna* task = new Hal::FemtoBasicAna();
  task->SetCorrFctn(Hal::FemtoCorrFuncKt(Hal::Femto1DCF("cf", 100, 0, 1, Hal::Femto::EKinematics::kLCMS), {0, 10}));
  task->SetOption(Hal::TwoTrackAna::BackgroundOptionMixed());
  task->SetWeight(Hal::FemtoWeightGenerator());
  task->SetPdg(211);
  task->SetMixSize(5);
  if (speedUp) {
    /**
       * speed up analysis by using compression and special HBT format
       * the sts exit distance and phi* are calculated once per track
       */
    task->SetFormat(new HalCbmFullEvent(), Hal::EFormatDepth::kNonBuffered);
    task->SetFormat(new HalCbmHbtFullEvent(), Hal::EFormatDepth::kBuffered);
    task->SetFormatOption(Hal::EventAna::EFormatOption::kCompress);
  }
  else {
    task->SetFormat(new HalCbmFullEvent());
  }
  SetEventCut(task);
  SetTrackCuts(task);
  SetPairCuts(task);

  return task;
}
