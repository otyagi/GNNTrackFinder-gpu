/* Copyright (C) 2012-2020 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak, Igor Kulakov [committer] */

/// \file CaGrid.h

#pragma once  // include this header only once per compilation unit

#include "CaGrid.h"
#include "CaHit.h"
#include "CaSimd.h"

namespace cbm::algo::ca
{
  /// \brief Class for accessing objects in the 2D area that are stored in ca::Grid
  ///
  class GridArea {
   public:
    /// \brief Constructor
    /// \param grid - the grid to work with
    /// \param x - X coordinate of the center of the area
    /// \param y - Y coordinate of the center of the area
    /// \param dx - half-width of the area in X
    /// \param dy - half-width of the area in Y
    GridArea(const ca::Grid& grid, fscal x, fscal y, fscal dx, fscal dy);


    /// \brief look up the next grid entry in the requested area
    /// \return ind - the entry index in the grid.GetEntries() vector
    /// \return true if the entry is found, false if there are no more entries in the area
    bool GetNextGridEntry(ca::HitIndex_t& ind);

    /// \brief look up the next object id in the requested area
    /// \return objectId - the id of the object
    /// \return true if the object is found, false if there are no more pbjects in the area
    bool GetNextObjectId(ca::HitIndex_t& objectId);

    /// \brief debug mode: loop over the entire GetEntries() vector ignoring the search area
    void DoLoopOverEntireGrid();

   private:
    const ca::Grid& fGrid;

    int fAreaLastBinY{0};            // last Y bin of the area
    int fAreaNbinsX{0};              // number of area bins in X
    int fAreaFirstBin{0};            // first bin of the area (left-down corner of the area)
    int fAreaCurrentBinY{0};         // current Y bin (incremented while iterating)
    ca::HitIndex_t fCurentEntry{0};  // index of the current entry in fGrid.GetEntries()
    ca::HitIndex_t fEntriesXend{0};  // end of the hit indices in current y-row
    int fGridNbinsX{0};              // Number of grid bins in X (copy of fGrid::GetNofBinsX())
  };

  inline GridArea::GridArea(const ca::Grid& grid, fscal x, fscal y, fscal dx, fscal dy)
    : fGrid(grid)
    , fGridNbinsX(fGrid.GetNofBinsX())
  {
    int binXmin = fGrid.GetBinX(x - dx);
    int binXmax = fGrid.GetBinX(x + dx);
    int binYmin = fGrid.GetBinY(y - dy);
    int binYmax = fGrid.GetBinY(y + dy);

    fAreaLastBinY = binYmax;
    fAreaNbinsX   = (binXmax - binXmin + 1);  // bin index span in x direction

    fAreaFirstBin = (binYmin * fGridNbinsX + binXmin);

    fAreaCurrentBinY = binYmin;
    fCurentEntry     = fGrid.GetFirstBinEntryIndex(fAreaFirstBin);
    fEntriesXend     = fGrid.GetFirstBinEntryIndex(fAreaFirstBin + fAreaNbinsX);
  }

  inline bool GridArea::GetNextGridEntry(ca::HitIndex_t& ind)
  {
    bool xIndexOutOfRange = (fCurentEntry >= fEntriesXend);  // current entry is not in the area

    // jump to the next y row if fCurentEntry is outside of the X area
    while (kfutils::IsUnlikely(xIndexOutOfRange)) {
      if (kfutils::IsUnlikely(fAreaCurrentBinY >= fAreaLastBinY)) {
        return false;
      }
      fAreaCurrentBinY++;            // get new y-line
      fAreaFirstBin += fGridNbinsX;  // move the left-down corner of the area to the next y-line
      fCurentEntry     = fGrid.GetFirstBinEntryIndex(fAreaFirstBin);  // get first hit in cell, if y-line is new
      fEntriesXend     = fGrid.GetFirstBinEntryIndex(fAreaFirstBin + fAreaNbinsX);
      xIndexOutOfRange = (fCurentEntry >= fEntriesXend);
    }
    ind = fCurentEntry;  // return value
    fCurentEntry++;      // go to next
    return true;
  }

  inline bool GridArea::GetNextObjectId(ca::HitIndex_t& objectId)
  {
    ca::HitIndex_t entryIndex = 0;

    bool ok = GetNextGridEntry(entryIndex);
    if (ok) {
      objectId = fGrid.GetEntries()[entryIndex].GetObjectId();
    }
    return ok;
  }

  inline void GridArea::DoLoopOverEntireGrid()
  {
    fCurentEntry     = 0;
    fEntriesXend     = fGrid.GetEntries().size();
    fAreaLastBinY    = 0;
    fAreaNbinsX      = 0;
    fAreaFirstBin    = 0;
    fAreaCurrentBinY = 0;
  }

}  // namespace cbm::algo::ca
