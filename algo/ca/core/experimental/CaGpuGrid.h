/* Copyright (C) 2010-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov, Sergey Gorbunov, Grigory Kozlov [committer] */

/// \file CaGpuGrid.h
/// \brief A class to store hit information in a backet-sorted way on 2D grid
///
/// This is a temporary solution to store the grid on GPU
/// The code has been simplified and modified relative to the CaGrid class for use on GPU
/// The original code is based on the code of the ALICE HLT Project
///

#pragma once  // include this header only once per compilation unit

#include "CaGrid.h"

#include <xpu/device.h>

namespace cbm::algo::ca
{

  namespace
  {
    using namespace cbm::algo;
  }

  /// \class Grid
  /// \brief Class for storing 2d objects in a grid

  /// It creates 2-dimensional backet-sorted grid of pointers to 2-dimensional objects.
  ///
  /// The class provides an access to the objects in selected 2D bin without touching the rest of the data.
  /// To loop over the objects in arbitrary XY-area one can use the ca::GridArea class.
  ///
  /// The class is used by CaTracker to speed-up the hit search operations
  /// The grid axis are named X and Y
  ///
  class GpuGrid {
   public:
    /// \brief  Default constructor
    GpuGrid() = default;

    /// \brief Destructor
    ~GpuGrid() = default;

    /// \brief Constructor
    GpuGrid(const Grid& grid, int bin_start, int entries_start)
      : fN(grid.GetNofBins())
      , fNx(grid.GetNofBinsX())
      , fNy(grid.GetNofBinsY())
      , fMinX(grid.GetMinX())
      , fMinY(grid.GetMinY())
      , fBinWidthX(grid.GetBinWidthX())
      , fBinWidthY(grid.GetBinWidthY())
      , fBinWidthXinv(grid.GetBinWidthXinv())
      , fBinWidthYinv(grid.GetBinWidthYinv())
      , fMaxRangeX(grid.GetMaxRangeX())
      , fMaxRangeY(grid.GetMaxRangeY())
      , fMaxRangeT(grid.GetMaxRangeT())
      , fBinEntryIndex_start(bin_start)
      , fBinEntryIndex_amount(grid.GetFirstBinEntryIndex().size())
      , fEntries_start(entries_start)
      , fEntries_amount(grid.GetEntries().size())
    {
    }

    /// \brief Get bin index for (X,Y) point with boundary check
    /// \param X - point x coordinate
    /// \param Y - point y coordinate
    /// \return bin index in the range [0, fN-1]
    XPU_D int GetBin(float X, float Y) const;

    /// \brief Get bin X index with boundary check
    /// \param X - x coordinate
    /// \return binX index
    XPU_D int GetBinX(float X) const;

    /// \brief Get bin Y index with boundary check
    /// \param Y - y coordinate
    /// \return binY index
    XPU_D int GetBinY(float Y) const;

    /// Get number of bins
    XPU_D int GetNofBins() const { return fN; }

    /// Get number of bins along X
    XPU_D int GetNofBinsX() const { return fNx; }

    /// Get number of bins along Y
    XPU_D int GetNofBinsY() const { return fNy; }

    /// Get minimal X value
    XPU_D float GetMinX() const { return fMinX; }

    /// Get minimal Y value
    XPU_D float GetMinY() const { return fMinY; }

    /// Get bin width in X
    XPU_D float GetBinWidthX() const { return fBinWidthX; }

    /// Get bin width in Y
    XPU_D float GetBinWidthY() const { return fBinWidthY; }

    /// Get maximal entry range in X
    XPU_D float GetMaxRangeX() const { return fMaxRangeX; }

    /// Get maximal entry range in Y
    XPU_D float GetMaxRangeY() const { return fMaxRangeY; }

    /// Get maximal entry range in T
    XPU_D float GetMaxRangeT() const { return fMaxRangeT; }

    /// Get bin width in X inversed
    XPU_D float GetBinWidthXinv() const { return fBinWidthXinv; }

    /// Get bin width in Y inversed
    XPU_D float GetBinWidthYinv() const { return fBinWidthYinv; }

    /// Get the index of the first bin entry in the index array
    XPU_D int GetBinEntryIndexStart() const { return fBinEntryIndex_start; }

    /// Get the number of entries in the index array
    XPU_D int GetBinEntryIndexAmount() const { return fBinEntryIndex_amount; }

    /// Get the index of the first entry in the entry array
    XPU_D int GetEntriesStart() const { return fEntries_start; }

    /// Get the number of entries in the entry array
    XPU_D int GetEntriesAmount() const { return fEntries_amount; }

   private:
    /// --- Data members ---

    int fN{0};   ///< total N bins
    int fNx{0};  ///< N bins in X
    int fNy{0};  ///< N bins in Y

    float fMinX{0.};          ///< minimal X value
    float fMinY{0.};          ///< minimal Y value
    float fBinWidthX{0.};     ///< bin width in X
    float fBinWidthY{0.};     ///< bin width in Y
    float fBinWidthXinv{0.};  ///< inverse bin width in X
    float fBinWidthYinv{0.};  ///< inverse bin width in Y

    float fMaxRangeX{0.};  ///< maximal entry range in X
    float fMaxRangeY{0.};  ///< maximal entry range in Y
    float fMaxRangeT{0.};  ///< maximal entry range in T

    int fBinEntryIndex_start{0};   ///< index of the first bin entry in the index array
    int fBinEntryIndex_amount{0};  ///< number of entries in the index array

    int fEntries_start{0};   ///< index of the first entry in the entry array
    int fEntries_amount{0};  ///< number of entries in the entry array
  };

  /// --- Inline methods ---

  XPU_D inline int GpuGrid::GetBin(float X, float Y) const
  {
    //
    return GetBinY(Y) * fNx + GetBinX(X);
  }

  XPU_D inline int GpuGrid::GetBinX(float X) const
  {
    int binX = static_cast<int>((X - fMinX) * fBinWidthXinv);
    return xpu::max(0, xpu::min(fNx - 1, binX));
  }

  XPU_D inline int GpuGrid::GetBinY(float Y) const
  {
    int binY = static_cast<int>((Y - fMinY) * fBinWidthYinv);
    return xpu::max(0, xpu::min(fNy - 1, binY));
  }

}  // namespace cbm::algo::ca
