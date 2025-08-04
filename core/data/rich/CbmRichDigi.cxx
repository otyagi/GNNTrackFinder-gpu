/* Copyright (C) 2015-2019 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev, Andrey Lebedev [committer], Volker Friese */

/*
 * CbmRichDigi.cxx
 *
 *  Created on: Dec 17, 2015
 *      Author: slebedev
 *  Modified on: Mar 25, 2019
 *              e.ovcharenko
 */

#include "CbmRichDigi.h"

CbmRichDigi::CbmRichDigi() : fAddress(0), fTime(0.0), fToT(0.)
{
  // TODO Auto-generated constructor stub
}

CbmRichDigi::CbmRichDigi(int32_t addr, double time, double tot) : fAddress(addr), fTime(time), fToT(tot) {}

CbmRichDigi::~CbmRichDigi()
{
  // TODO Auto-generated destructor stub
}

#ifndef NO_ROOT
ClassImp(CbmRichDigi)
#endif
