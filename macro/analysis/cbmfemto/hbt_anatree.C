/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
/**
 * example macro for pi-pi analysis
 */
#ifndef __CLING__
#include "CbmAnaTreeSource.h"
#include "CbmFieldMap.h"
#include "FairFileSource.h"
#include "FairParRootFileIo.h"
#include "FemtoBasicAna.h"
#include "HalCbmAnalysisManager.h"
#include "HalCbmTaskManager.h"
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

/**
 * macro for calcution of final CF
 */

#define KRONOS

/**
 * analysis macro done via FairRoot
 * @param task
 */

void hbt_anatree(TString inFile = "3000.analysistree.root", TString outFile = "test1.root")
{
  FairRunAna* ana = new FairRunAna();
  //  inFile = "/home/daniel/temp/00001.mini.root";
  FairSource* source = new CbmAnaTreeSource(inFile);
  ana->SetSource(source);
  ana->SetOutputFile(outFile);

  FairRuntimeDb* rtdb = ana->GetRuntimeDb();
  FairLogger* log     = FairLogger::GetLogger();
  log->SetColoredLog(kTRUE);
  log->SetLogVerbosityLevel("high");

  Hal::FemtoBasicAna* task = MakeAna(kTRUE);

  auto* fairtask = new HalCbmTaskManager();
  fairtask->AddTask(task);

  ana->AddTask(fairtask);

  TStopwatch timer;
  timer.Start();

  ana->Init();
  ana->Run(0, 1000);
  Double_t rtime = timer.RealTime(), ctime = timer.CpuTime();
  printf("RealTime=%f seconds, CpuTime=%f seconds\n", rtime, ctime);
}
