/* Copyright (C) 2013 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

#include "CbmLitRadLengthGenerator.h"

#include "FairPrimaryGenerator.h"

CbmLitRadLengthGenerator::CbmLitRadLengthGenerator() : FairGenerator(), fX(), fY() {}

Bool_t CbmLitRadLengthGenerator::Init() { return kTRUE; }

Bool_t CbmLitRadLengthGenerator::ReadEvent(FairPrimaryGenerator* primGen)
{
  static UInt_t eventNo = 0;

  if (eventNo < fX.size() && eventNo < fY.size()) {
    primGen->AddTrack(0, 0, 0, 1, fX[eventNo], fY[eventNo], 0);
  }
  else {
  }

  eventNo++;

  return kTRUE;
}

ClassImp(CbmLitRadLengthGenerator)
