/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#include "Cluster2D.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTrdDigi.h"

#include <cmath>

using std::vector;

namespace cbm::algo::trd
{
  //____________________________________________________________________
  Cluster2D::Cluster2D(const Cluster2D& ref)
    : fRecDigis(ref.fRecDigis)
    , fDigis(ref.fDigis)
    , fDigiIndices(ref.fDigiIndices)
    , fAddress(ref.fAddress)
    , fNCols(ref.fNCols)
    , fNRows(ref.fNRows)
    , fStartCh(ref.fStartCh)
    , fStartTime(ref.fStartTime)
  {
  }

  //____________________________________________________________________
  Cluster2D::Cluster2D(int32_t address, int32_t idx, const CbmTrdDigi* digi, uint16_t chT, uint16_t chR, int32_t row,
                       int32_t time)
    : fAddress(address)
    , fNCols(0)
    , fStartCh(0xffff)
    , fStartTime(time)
  {
    SetNRows(row);
    SetStart(false);
    SetStop(false);
    AddDigi(idx, digi, chT, chR);
  }

  //____________________________________________________________________
  bool Cluster2D::AddDigi(int32_t idx, const CbmTrdDigi* digi, uint16_t chT, uint16_t chR, int32_t dt)
  {
    /** Extend basic functionality of Cluster2D::AddDigi() for the case of 2D.
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
      AddDigiIdxPair(idx, digi);
      return true;
    }

    uint16_t chMin = (chT != 0 ? chT : chR), chMax = (chR != 0 ? chR : chT);

    // assume triangular pads only
    if (!fNCols) {  // first digi
      fStartCh = chMin;
      AddDigiIdxPair(idx, digi);
    }
    else if (chMin > GetEndCh()) {  // digi @ end
      //if (HasStop()) return false;
      AddDigiIdxPair(idx, digi);
    }
    else if (chMax < fStartCh) {  // digi @ beginning
      //if (HasStart()) return false;
      fStartCh                           = chMin;
      vector<int32_t> idx_vec            = GetDigiIndices();
      vector<const CbmTrdDigi*> digi_vec = GetDigis();
      ClearDigis();
      AddDigiIdxPair(idx, digi);
      AddDigiIdxPairs(idx_vec, digi_vec);
    }
    int nch(0);
    if (chT == 0)
      SetStart();
    else
      nch++;
    if (chR == 0)
      SetStop();
    else
      nch++;

    fNCols += nch;
    if (dt > 0) fStartTime -= dt;

    return true;
  }

  //____________________________________________________________________
  int32_t Cluster2D::IsChannelInRange(uint16_t chT, uint16_t chR) const
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
  bool Cluster2D::Merge(Cluster2D* second)
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
      fStartCh = second->fStartCh;
      fNCols += second->fNCols;
      fStartTime = std::min(fStartTime, second->fStartTime);

      vector<int32_t> idx_vec            = GetDigiIndices();
      vector<const CbmTrdDigi*> digi_vec = GetDigis();
      ClearDigis();
      AddDigiIdxPairs(second->GetDigiIndices(), second->GetDigis());
      AddDigiIdxPairs(idx_vec, digi_vec);
      if (second->HasStart()) SetStart();
      return true;
    }

    // look after current
    if (fStartCh + fNCols == second->fStartCh) {
      fNCols += second->fNCols;
      fStartTime = std::min(fStartTime, second->fStartTime);
      AddDigiIdxPairs(second->GetDigiIndices(), second->GetDigis());
      if (second->HasStop()) SetStop();
      return true;
    }
    return false;
  }

  bool Cluster2D::Finalize(size_t numCols)
  {
    /** Load RAW digis info in calibration aware strucuture CbmTrdDigiReco
 * Do basic sanity checks; also incomplete adjacent digi and if found merge them.
 */
    int last_col(-1);
    for (auto i = fDigis.begin(); i != fDigis.end(); i++) {
      if ((*i) == nullptr) continue;
      int colR(-1);
      const CbmTrdDigi* dgT = (*i);
      int colT              = dgT->GetAddressChannel() % numCols;

      // check column order for cluster  /// TO DO: we can probably drop this check
      if (last_col >= 0 && colT != last_col + 1) {
        L_(debug) << "TrdModuleRec2D::LoadDigis : digis in cluster not in increasing order !";
        return false;
      }
      last_col              = colT;
      const CbmTrdDigi* dgR = nullptr;

      auto j = std::next(i);
      if (j != fDigis.end()) {
        dgR  = (*j);
        colR = dgR->GetAddressChannel() % numCols;
      }
      if (colR == colT && dgR != nullptr) {
        fRecDigis.emplace_back(*dgT, *dgR);
        i++;
      }
      else {
        fRecDigis.emplace_back(*dgT);
      }
    }
    return true;
  }
}  // namespace cbm::algo::trd
