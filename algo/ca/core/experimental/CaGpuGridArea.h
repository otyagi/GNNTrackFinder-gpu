/* Copyright (C) 2012-2020 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak, Igor Kulakov [committer] */

/// \file CaGpuGridArea.h
///
/// \brief A temporary implementation for CaGridArea, created for compatibility with GPU code
///

#pragma once  // include this header only once per compilation unit

#include "CaGpuGrid.h"
#include "CaHit.h"

namespace cbm::algo::ca
{

  namespace
  {
    using namespace cbm::algo;  // to keep 'ca::' prefices in the code
  }

  /// \brief Class for accessing objects in the 2D area that are stored in ca::Grid
  ///
  class GpuGridArea {
   public:
    /// \brief Constructor
    /// \param grid - the grid to work with
    /// \param x - X coordinate of the center of the area
    /// \param y - Y coordinate of the center of the area
    /// \param dx - half-width of the area in X
    /// \param dy - half-width of the area in Y
    //TODO: try to provide first element for the current grid instead of first element of the original arrays
    XPU_D GpuGridArea(const ca::GpuGrid& grid, const unsigned int* BinEntryIndex, const unsigned int* Entries, float x,
                      float y, float dx, float dy)
      : fGrid(grid)
      , fBinEntryIndex(BinEntryIndex)
      , fEntries(Entries)
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
      fCurentEntry     = fBinEntryIndex[fAreaFirstBin];
      fEntriesXend     = fBinEntryIndex[fAreaFirstBin + fAreaNbinsX];
    }


    /// \brief look up the next grid entry in the requested area
    /// \return ind - the entry index in the grid.GetEntries() vector
    /// \return true if the entry is found, false if there are no more entries in the area
    XPU_D bool GetNextGridEntry(unsigned int& ind);

    /// \brief look up the next object id in the requested area
    /// \return objectId - the id of the object
    /// \return true if the object is found, false if there are no more pbjects in the area
    XPU_D bool GetNextObjectId(unsigned int& objectId);

    /// \brief debug mode: loop over the entire GetEntries() vector ignoring the search area
    //    void DoLoopOverEntireGrid();

   private:
    const ca::GpuGrid& fGrid;
    const unsigned int* fBinEntryIndex;
    const unsigned int* fEntries;

    int fAreaLastBinY{0};            // last Y bin of the area
    int fAreaNbinsX{0};              // number of area bins in X
    int fAreaFirstBin{0};            // first bin of the area (left-down corner of the area)
    int fAreaCurrentBinY{0};         // current Y bin (incremented while iterating)
    ca::HitIndex_t fCurentEntry{0};  // index of the current entry in fGrid.GetEntries()
    ca::HitIndex_t fEntriesXend{0};  // end of the hit indices in current y-row
    int fGridNbinsX{0};              // Number of grid bins in X (copy of fGrid::GetNofBinsX())
  };

  XPU_D inline bool GpuGridArea::GetNextGridEntry(unsigned int& ind)
  {
    bool xIndexOutOfRange = (fCurentEntry >= fEntriesXend);  // current entry is not in the area

    // jump to the next y row if fCurentEntry is outside of the X area
    while (xIndexOutOfRange) {
      if (fAreaCurrentBinY >= fAreaLastBinY) {
        return false;
      }
      fAreaCurrentBinY++;                                // get new y-line
      fAreaFirstBin += fGridNbinsX;                      // move the left-down corner of the area to the next y-line
      fCurentEntry     = fBinEntryIndex[fAreaFirstBin];  // get first hit in cell, if y-line is new
      fEntriesXend     = fBinEntryIndex[fAreaFirstBin + fAreaNbinsX];
      xIndexOutOfRange = (fCurentEntry >= fEntriesXend);
    }
    ind = fCurentEntry;  // return value
    fCurentEntry++;      // go to next
    return true;
  }

  XPU_D inline bool GpuGridArea::GetNextObjectId(unsigned int& objectId)
  {
    unsigned int entryIndex = 0;

    bool ok = GetNextGridEntry(entryIndex);
    if (ok) {
      objectId = fEntries[entryIndex];
    }
    return ok;
  }

}  // namespace cbm::algo::ca
