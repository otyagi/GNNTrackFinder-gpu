/* Copyright (C) 2012-2015 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Michael Deveaux, Philipp Sitzmann [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                  CbmMvdSensorTask source file              -----
// -----                  Created 02.02.2012 by M. Deveaux            -----
// -------------------------------------------------------------------------

#include "CbmMvdSensorTask.h"

#include "TClonesArray.h"

// -----   Default constructor   -------------------------------------------
CbmMvdSensorTask::CbmMvdSensorTask()
  : CbmMvdSensorPlugin()
  , fInputBuffer(nullptr)
  , fOutputBuffer(nullptr)
  , fSensor(nullptr)
{
  fPluginIDNumber = -1;
}
// -------------------------------------------------------------------------
// -----  constructor   -------------------------------------------
CbmMvdSensorTask::CbmMvdSensorTask(const char* name)
  : CbmMvdSensorPlugin(name)
  , fInputBuffer(nullptr)
  , fOutputBuffer(nullptr)
  , fSensor(nullptr)
{
  fPluginIDNumber = -1;
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMvdSensorTask::~CbmMvdSensorTask()
{
  fInputBuffer->Delete();

  fOutputBuffer->Delete();
}
// -------------------------------------------------------------------------

ClassImp(CbmMvdSensorTask)
