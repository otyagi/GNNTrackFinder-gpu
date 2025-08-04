/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak [committer], Valentina Akishina */

/// \file L1Grid.cxx
/// \brief Class for storing 2d objects in a grid


#include "CaGrid.h"

#include "CaHit.h"

#include <algorithm>
#include <cstring>


namespace cbm::algo::ca
{
  void Grid::BuildBins(fscal xMin, fscal xMax, fscal yMin, fscal yMax, fscal binWidthX, fscal binWidthY)
  {
    fMinX = std::min(xMin, xMax);
    fMinY = std::min(yMin, yMax);

    xMax = std::max(xMin, xMax);
    yMax = std::max(yMin, yMax);

    fBinWidthX = binWidthX;
    fBinWidthY = binWidthY;

    // some sanity checks
    if (fBinWidthX < 0.001) {
      fBinWidthX = 0.001;
    }
    if (fBinWidthY < 0.001) {
      fBinWidthY = 0.001;
    }

    fBinWidthXinv = 1. / fBinWidthX;
    fBinWidthYinv = 1. / fBinWidthY;

    fNx = static_cast<int>(std::ceil((xMax - fMinX) / fBinWidthX));
    fNy = static_cast<int>(std::ceil((yMax - fMinY) / fBinWidthY));

    // some sanity checks
    if (fNx < 1) fNx = 1;
    if (fNy < 1) fNy = 1;

    fN = fNx * fNy;

    fEntries.clear();
    fFirstBinEntryIndex.reset(fN + 1, 0);
    fNofBinEntries.reset(fN + 1, 0);
  }


  void Grid::StoreHits(const Vector<ca::Hit>& hits, ca::HitIndex_t hitStartIndex, ca::HitIndex_t nHits,
                       const Vector<unsigned char>& hitKeyFlags)
  {
    fFirstBinEntryIndex.reset(fN + 1, 0);
    fNofBinEntries.reset(fN + 1, 0);

    int nEntries = 0;
    for (ca::HitIndex_t ih = 0; ih < nHits; ih++) {
      const ca::Hit& hit = hits[hitStartIndex + ih];
      if (!(hitKeyFlags[hit.FrontKey()] || hitKeyFlags[hit.BackKey()])) {
        fNofBinEntries[GetBin(hit.X(), hit.Y())]++;
        nEntries++;
      }
    }

    fEntries.reset(nEntries);

    for (int bin = 0; bin < fN; bin++) {
      fFirstBinEntryIndex[bin + 1] = fFirstBinEntryIndex[bin] + fNofBinEntries[bin];
      fNofBinEntries[bin]          = 0;
    }
    fNofBinEntries[fN] = 0;

    fMaxRangeX = 0.;
    fMaxRangeY = 0.;
    fMaxRangeT = 0.;

    for (ca::HitIndex_t ih = 0; ih < nHits; ih++) {
      const ca::Hit& hit = hits[hitStartIndex + ih];
      if (!(hitKeyFlags[hit.FrontKey()] || hitKeyFlags[hit.BackKey()])) {
        int bin = GetBin(hit.X(), hit.Y());
        fEntries[fFirstBinEntryIndex[bin] + fNofBinEntries[bin]].Set(hit, hitStartIndex + ih);
        fNofBinEntries[bin]++;
        fMaxRangeX = std::max(fMaxRangeX, hit.RangeX());
        fMaxRangeY = std::max(fMaxRangeY, hit.RangeY());
        fMaxRangeT = std::max(fMaxRangeT, hit.RangeT());
      }
    }
  }

  void Grid::RemoveUsedHits(const Vector<ca::Hit>& hits, const Vector<unsigned char>& hitKeyFlags)
  {
    int nEntries = 0;
    fMaxRangeX   = 0.;
    fMaxRangeY   = 0.;
    fMaxRangeT   = 0.;

    for (int bin = 0; bin < fN; bin++) {
      ca::HitIndex_t firstEntryOld = fFirstBinEntryIndex[bin];
      fFirstBinEntryIndex[bin]     = nEntries;
      fNofBinEntries[bin]          = 0;
      for (ca::HitIndex_t i = firstEntryOld; i < fFirstBinEntryIndex[bin + 1]; i++) {
        auto entry         = fEntries[i];
        const ca::Hit& hit = hits[entry.GetObjectId()];

        if (!(hitKeyFlags[hit.FrontKey()] || hitKeyFlags[hit.BackKey()])) {
          fEntries[nEntries] = entry;
          nEntries++;
          fNofBinEntries[bin]++;
          fMaxRangeX = std::max(fMaxRangeX, entry.RangeX());
          fMaxRangeY = std::max(fMaxRangeY, entry.RangeY());
          fMaxRangeT = std::max(fMaxRangeT, entry.RangeT());
        }
      }
    }
    fFirstBinEntryIndex[fN] = nEntries;
    fNofBinEntries[fN]      = 0;
    fEntries.shrink(nEntries);
  }
}  // namespace cbm::algo::ca
