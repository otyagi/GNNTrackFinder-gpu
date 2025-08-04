/* Copyright (C) 2012-2015 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Michael Deveaux, Philipp Sitzmann [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                  CbmMvdSensorBuffer source file              -----
// -----                  Created 02.02.2012 by M. Deveaux            -----
// -------------------------------------------------------------------------
#include "CbmMvdSensorBuffer.h"

#include "TClonesArray.h"

// -----   Default constructor   -------------------------------------------
CbmMvdSensorBuffer::CbmMvdSensorBuffer()
  : CbmMvdSensorPlugin()
  , fBuffer(nullptr)
  , fCurrentEvent(nullptr)
  , fSensor(nullptr)
{
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMvdSensorBuffer::~CbmMvdSensorBuffer()
{
  fBuffer->Delete();
  delete fBuffer;
}
// -------------------------------------------------------------------------

void CbmMvdSensorBuffer::Clear(Option_t* /*opt*/)
{
  if (fBuffer) { fBuffer->Clear(); }
  if (fCurrentEvent) { fCurrentEvent->Clear(); }
}

ClassImp(CbmMvdSensorBuffer)
