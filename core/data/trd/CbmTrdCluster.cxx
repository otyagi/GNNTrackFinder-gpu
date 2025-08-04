/* Copyright (C) 2010-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

/**
 * \file CbmTrdCluster.cxx
 * \author Florian Uhlig <f.uhlig@gsi.de>
 * \brief Data Container for TRD clusters.
 */
#include "CbmTrdCluster.h"

#include <Logger.h>  // for LOG, Logger

#include <iostream>  // for operator<<, basic_ostream, stringstream

#include <cmath>

using std::endl;
using std::string;
using std::stringstream;
using std::vector;
//____________________________________________________________________
CbmTrdCluster::CbmTrdCluster() : CbmCluster() {}

//____________________________________________________________________
CbmTrdCluster::CbmTrdCluster(const CbmTrdCluster& ref)
  : CbmCluster(ref.GetDigis(), ref.GetAddress())
  , fNCols(ref.fNCols)
  , fNRows(ref.fNRows)
  , fStartCh(ref.fStartCh)
  , fStartTime(ref.fStartTime)
{
}

//____________________________________________________________________
CbmTrdCluster::CbmTrdCluster(const std::vector<int32_t>& indices, int32_t address)
  : CbmCluster(indices, address)
{
}

//____________________________________________________________________
CbmTrdCluster::CbmTrdCluster(int32_t address, int32_t idx, uint16_t chT, uint16_t chR, int32_t row, int32_t time)
  : CbmCluster()
{
  ReInit(address, row, time);
  AddDigi(idx, chT, chR);
}

//____________________________________________________________________
CbmTrdCluster::~CbmTrdCluster() {}

CbmTrdCluster& CbmTrdCluster::operator=(const CbmTrdCluster& ref)
{
  if (this != &ref) {
    CbmCluster::operator=(ref);
    fNCols              = ref.fNCols;
    fNRows              = ref.fNRows;
    fStartCh            = ref.fStartCh;
    fStartTime          = ref.fStartTime;
  }
  return *this;
}

//____________________________________________________________________
bool CbmTrdCluster::AddChannel(bool r)
{
  if (!r) {
    if (!fStartCh) return false;
    fStartCh--;
  }
  fNCols++;
  return true;
}

//____________________________________________________________________
bool CbmTrdCluster::AddDigi(int32_t idx, uint16_t chT, uint16_t chR, int32_t dt)
{
  /** Extend basic functionality of CbmCluster::AddDigi() for the case of 2D.
 * If chT < 0 use the basic functionality [default].
 * 
 * For the 2D the parameters are intergpreted as follows
 * chT : tilted paired channel [default 0x0fffffff]
 * chR : rectangular paired channel
 * dt  : offset in clks of the prompt signal
 *
 * if chT and chR positive the (chT, chR) are interpreted as the 2 channels
 * of the digi specific to the 2D version. The following specific cases 
 * can be distinguished : 
 *  - ch == 0 : no data, cluster signal sequence terminator
 *  - ch == -ch : no data, channel masked in HW 
 */

  if (chT == 0xffff) {  // basic functionality for rectangular pads
    CbmCluster::AddDigi(idx);
    return true;
  }

  uint16_t chMin = (chT != 0 ? chT : chR), chMax = (chR != 0 ? chR : chT);

  // assume triangular pads only
  if (!fNCols) {  // first digi
    fStartCh = chMin;
    CbmCluster::AddDigi(idx);
  }
  else if (chMin > GetEndCh()) {  // digi @ end
    //if (HasStop()) return false;
    CbmCluster::AddDigi(idx);
  }
  else if (chMax < fStartCh) {  // digi @ beginning
    //if (HasStart()) return false;
    fStartCh            = chMin;
    vector<int32_t> vec = GetDigis();
    ClearDigis();
    CbmCluster::AddDigi(idx);
    AddDigis(vec);
  }
  int nch(0);
  if (chT == 0) SetStart();
  else
    nch++;
  if (chR == 0) SetStop();
  else
    nch++;

  fNCols += nch;
  if (dt > 0) fStartTime -= dt;

  return true;
}

//____________________________________________________________________
void CbmTrdCluster::Clear(Option_t*)
{
  CbmCluster::ClearDigis();
  fNCols     = 0;
  fNRows     = 0x1f;
  fStartCh   = 0xffff;
  fStartTime = 0xffffffff;
}

//____________________________________________________________________
void CbmTrdCluster::ReInit(int32_t address, int32_t row, int32_t time)
{
  SetAddress(address);
  fNCols   = 0;
  fStartCh = 0xffff;
  // check truncation
  if (row >= 0x1f) LOG(warn) << GetName() << "::ReInit: pad-row truncated to 5bits.";
  SetNRows(row);
  SetStart(false);
  SetStop(false);
  if (std::abs(time) >= 0x7fffffff) LOG(warn) << GetName() << "::ReInit: buffer time truncated to 4bytes.";
  fStartTime = time;
}

//____________________________________________________________________
int32_t CbmTrdCluster::IsChannelInRange(uint16_t chT, uint16_t chR) const
{
  if (!fNCols) return -2;
  //   if(IsTerminatedLeft() && fAddressCh[0]>ch) return -1;
  //   if(IsTerminatedRight() && fAddressCh[clSize-1]<ch) return 1;

  uint16_t chMin = (chT != 0 ? chT : chR), chMax = (chR != 0 ? chR : chT);
  if (fStartCh > chMax + 1) return -1;
  if (fStartCh + fNCols < chMin) return 1;
  return 0;
}

//____________________________________________________________________
bool CbmTrdCluster::Merge(CbmTrdCluster* second)
{
  if (GetRow() != second->GetRow()) return false;
  // time difference condition
  if (fNCols == 1 || second->fNCols == 1) {
    if (abs(int32_t(second->fStartTime - fStartTime)) > 50) return false;
  }
  else if (abs(int32_t(second->fStartTime - fStartTime)) > 20)
    return false;
  // look before current
  if (second->fStartCh + second->fNCols == fStartCh) {
    // std::cout<<"Merge before with "<<second->ToString();
    fStartCh = second->fStartCh;
    fNCols += second->fNCols;
    fStartTime = std::min(fStartTime, second->fStartTime);

    vector<int32_t> vec = GetDigis();
    ClearDigis();
    AddDigis(second->GetDigis());
    AddDigis(vec);
    if (second->HasStart()) SetStart();
    return true;
  }

  // look after current
  if (fStartCh + fNCols == second->fStartCh) {
    // std::cout<<"Merge after  with "<<second->ToString();
    fNCols += second->fNCols;
    fStartTime = std::min(fStartTime, second->fStartTime);
    AddDigis(second->GetDigis());
    if (second->HasStop()) SetStop();
    return true;
  }
  return false;
}

//____________________________________________________________________
string CbmTrdCluster::ToString() const
{
  stringstream ss;
  ss << CbmCluster::ToString();
  ss << "CbmTrdCluster: mod=" << GetAddress() << " row=" << (int32_t) GetRow() << " "
     << (HasFaspDigis() ? "Fasp_" : "Spadic_") << "Chs=";
  ss << (HasStart() ? "|" : "/");
  for (int32_t i(0); i < fNCols; i++)
    ss << fStartCh + i << " ";
  ss << (HasStop() ? "|" : "/");
  ss << endl;
  return ss.str();
}

ClassImp(CbmTrdCluster)
