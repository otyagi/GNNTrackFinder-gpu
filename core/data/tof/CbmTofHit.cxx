/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: E. Cordier, Florian Uhlig, Andrey Lebedev, Denis Bertini [committer], Pierre-Alain Loizeau */

/**
 * \file CbmTofHit.cxx
 * \author E. Cordier
**/

#include "CbmTofHit.h"

#include <TString.h>   // for Form
#include <TVector3.h>  // for TVector3

#include <sstream>  // for operator<<, basic_ostream, stringstream

using std::endl;
using std::string;
using std::stringstream;

CbmTofHit::CbmTofHit() : CbmPixelHit(), fFlag(1), fChannel(0)
{
  SetType(kTOFHIT);
  SetTime(0.);
}

CbmTofHit::CbmTofHit(int32_t address, TVector3 pos, TVector3 dpos, int32_t index, double time, double dtime,
                     int32_t flag, int32_t channel)
  : CbmPixelHit(address, pos, dpos, 0., index, time, dtime)
  , fFlag(flag)
  , fChannel(channel)
{
  SetType(kTOFHIT);
}

CbmTofHit::CbmTofHit(int32_t address, TVector3 pos, TVector3 dpos, int32_t index, double time, int32_t flag,
                     int32_t channel)
  : CbmPixelHit(address, pos, dpos, 0., index)
  , fFlag(flag)
  , fChannel(channel)
{
  SetType(kTOFHIT);
  SetTime(time);
}

CbmTofHit::CbmTofHit(int32_t address, TVector3 pos, TVector3 dpos, int32_t index, double time, int32_t flag)
  : CbmPixelHit(address, pos, dpos, 0., index)
  , fFlag(flag)
  , fChannel(0)
{
  SetType(kTOFHIT);
  SetTime(time);
}

CbmTofHit::CbmTofHit(int32_t address, TVector3 pos, TVector3 dpos, int32_t index, double time)
  : CbmPixelHit(address, pos, dpos, 0., index)
  , fFlag(1)
  , fChannel(0)
{
  SetType(kTOFHIT);
  SetTime(time);
}

CbmTofHit::~CbmTofHit() {}

string CbmTofHit::ToString() const
{
  stringstream ss;
  ss << "CbmTofHit: address=" << GetAddress() << " pos=(" << GetX() << "," << GetY() << "," << GetZ() << ") err=("
     << GetDx() << "," << GetDy() << "," << GetDz() << ") dxy=" << GetDxy()  //<< " refId=" << GetRefId()
     << Form(" time=%8.2f", GetTime()) << " flag=" << GetFlag();
  // << " channel=" << GetCh(); // << endl;
  return ss.str();
}

ClassImp(CbmTofHit)
