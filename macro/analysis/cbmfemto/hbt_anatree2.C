/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */

#ifndef __CLING__
#include "CbmAnaTreeSource.h"
#include "CbmFieldMap.h"
#include "FairFileSource.h"
#include "FairParRootFileIo.h"
#include "FemtoBasicAna.h"
#include "HalCbmAnalysisManager.h"
#include "HalCbmSource.h"
#include "TFile.h"
#include "data/CbmDefs.h"

#include <FairLogger.h>
#include <FairRun.h>
#include <FairRunAna.h>

#include <RtypesCore.h>
#include <TStopwatch.h>
#include <TString.h>
#include <TTimer.h>

#include <cstdio>
#include <iostream>

#include <Hal/AnalysisManager.h>
#include <Hal/Source.h>
#include <Hal/TaskManager.h>


using namespace std;
#endif
#include "hbt_common.C"

#define KRONOS

/**
 * analysis macro done via FairRoot
 * @param task
 */

void hbt_anatree2(TString inFile = "3000.analysistree.root", TString outFile = "test2.root")
{
  HalCbmAnalysisManager* manager = new HalCbmAnalysisManager();
  manager->SetSource(new HalCbmSource(inFile));
  manager->SetOutput(outFile);

  Hal::FemtoBasicAna* task = MakeAna(kTRUE);

  manager->AddTask(task);

  TStopwatch timer;
  timer.Start();

  manager->Init();
  manager->Run(0, 1000);
  Double_t rtime = timer.RealTime(), ctime = timer.CpuTime();
  printf("RealTime=%f seconds, CpuTime=%f seconds\n", rtime, ctime);
}
