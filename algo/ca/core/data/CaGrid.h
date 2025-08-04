/* Copyright (C) 2010-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Sergey Gorbunov */

/// \file CaGrid.h
/// \brief A class to store hit information in a backet-sorted way on 2D grid
///
/// This code is based on the code of the ALICE HLT Project
///

#pragma once  // include this header only once per compilation unit

#include "CaGridEntry.h"
#include "CaHit.h"
#include "CaSimd.h"
#include "CaVector.h"

namespace cbm::algo::ca
{

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
  class Grid {
   public:
    /// \brief  Default constructor
    Grid() = default;

    /// \brief Destructor
    ~Grid() = default;

    /// \brief  Build the grid
    /// \param  xMin - minimal X value
    /// \param  xMax - maximal X value
    /// \param  yMin - minimal Y value
    /// \param  yMax - maximal Y value
    /// \param  binWidthX - bin width in X
    /// \param  binWidthY - bin width in Y
    void BuildBins(fscal xMin, fscal xMax, fscal yMin, fscal yMax, fscal binWidthX, fscal binWidthY);

    /// \brief  Store objects in the grid
    // TODO: write the method with a template input
    /// void StoreData(...);

    /// \brief  Store ca::Hits in the grid
    /// \param  hits - vector of hits to store
    /// \param  hitStartIndex - index of the first hit to store
    /// \param  nHits - number of hits to store starting from the hitStartIndex
    /// \param  hitKeyFlags - vector of flags to recognise used hits and skip them
    void StoreHits(const ca::Vector<ca::Hit>& hits, ca::HitIndex_t hitStartIndex, ca::HitIndex_t nHits,
                   const ca::Vector<unsigned char>& hitKeyFlags);

    /// \brief Remove grid entries that correspond to the used hits
    void RemoveUsedHits(const ca::Vector<ca::Hit>& hits, const ca::Vector<unsigned char>& hitKeyFlags);

    /// \brief Get bin index for (X,Y) point with boundary check
    /// \param X - point x coordinate
    /// \param Y - point y coordinate
    /// \return bin index in the range [0, fN-1]
    int GetBin(fscal X, fscal Y) const;

    /// \brief Get bin X index with boundary check
    /// \param X - x coordinate
    /// \return binX index
    int GetBinX(fscal X) const;

    /// \brief Get bin Y index with boundary check
    /// \param Y - y coordinate
    /// \return binY index
    int GetBinY(fscal Y) const;

    /// \brief Get bin bounds along X
    /// \param iBin - bin index
    /// \return pair of (Xmin, Xmax) bounds
    std::tuple<fscal, fscal> GetBinBoundsX(int iBin) const;

    /// \brief Get bin bounds along Y
    /// \param iBin - bin index
    /// \return pair of (Ymin, Ymax) bounds
    std::tuple<fscal, fscal> GetBinBoundsY(int iBin) const;

    /// Get number of bins
    int GetNofBins() const { return fN; }

    /// Get number of bins along X
    int GetNofBinsX() const { return fNx; }

    /// Get number of bins along Y
    int GetNofBinsY() const { return fNy; }

    /// Get index of the first bin entry in fHitsInBin array
    ca::HitIndex_t GetFirstBinEntryIndex(int bin) const { return fFirstBinEntryIndex[(bin < fN) ? bin : fN]; }

    /// Get number of entries in the bin
    //ca::HitIndex_t GetNofBinEntries(int bin) const { return fNofBinEntries[bin < fN ? bin : fN]; }

    /// Get entries
    const ca::Vector<ca::GridEntry>& GetEntries() const { return fEntries; }

    /// Get minimal X value
    fscal GetMinX() const { return fMinX; }

    /// Get minimal Y value
    fscal GetMinY() const { return fMinY; }

    /// Get bin width in X
    fscal GetBinWidthX() const { return fBinWidthX; }

    /// Get bin width in Y
    fscal GetBinWidthY() const { return fBinWidthY; }

    /// Get maximal entry range in X
    fscal GetMaxRangeX() const { return fMaxRangeX; }

    /// Get maximal entry range in Y
    fscal GetMaxRangeY() const { return fMaxRangeY; }

    /// Get maximal entry range in T
    fscal GetMaxRangeT() const { return fMaxRangeT; }

    /// Get bin width in X inversed
    fscal GetBinWidthXinv() const { return fBinWidthXinv; }

    /// Get bin width in Y inversed
    fscal GetBinWidthYinv() const { return fBinWidthYinv; }

    /// Get the link to the first bin entry index
    const ca::Vector<ca::HitIndex_t>& GetFirstBinEntryIndex() const { return fFirstBinEntryIndex; }

   private:
    /// --- Data members ---

    int fN{0};   ///< total N bins
    int fNx{0};  ///< N bins in X
    int fNy{0};  ///< N bins in Y

    fscal fMinX{0.};          ///< minimal X value
    fscal fMinY{0.};          ///< minimal Y value
    fscal fBinWidthX{0.};     ///< bin width in X
    fscal fBinWidthY{0.};     ///< bin width in Y
    fscal fBinWidthXinv{0.};  ///< inverse bin width in X
    fscal fBinWidthYinv{0.};  ///< inverse bin width in Y

    fscal fMaxRangeX{0.};  ///< maximal entry range in X
    fscal fMaxRangeY{0.};  ///< maximal entry range in Y
    fscal fMaxRangeT{0.};  ///< maximal entry range in T

    ca::Vector<ca::HitIndex_t> fFirstBinEntryIndex{"Grid::fFirstBinEntryIndex", 1,
                                                   0};  ///< index of the first entry in the bin

    ca::Vector<ca::HitIndex_t> fNofBinEntries{"Grid::fNofBinEntries", 1, 0};  ///< number of hits in the bin

    ca::Vector<ca::GridEntry> fEntries{
      "Ca::Grid::fEntries"};  ///< grid entries with references to the hit index in fWindowHits
  };

  /// --- Inline methods ---

  inline int Grid::GetBin(fscal X, fscal Y) const
  {
    //
    return GetBinY(Y) * fNx + GetBinX(X);
  }

  inline int Grid::GetBinX(fscal X) const
  {
    int binX = static_cast<int>((X - fMinX) * fBinWidthXinv);
    return std::max(0, std::min(fNx - 1, binX));
  }

  inline int Grid::GetBinY(fscal Y) const
  {
    int binY = static_cast<int>((Y - fMinY) * fBinWidthYinv);
    return std::max(0, std::min(fNy - 1, binY));
  }

  inline std::tuple<fscal, fscal> Grid::GetBinBoundsX(int iBin) const
  {
    fscal Xmin = fMinX + (iBin % fNx) * fBinWidthX;
    return std::make_tuple(Xmin, Xmin + fBinWidthX);
  }

  inline std::tuple<fscal, fscal> Grid::GetBinBoundsY(int iBin) const
  {
    fscal Ymin = fMinY + (iBin / fNx) * fBinWidthY;
    return std::make_tuple(Ymin, Ymin + fBinWidthY);
  }

}  // namespace cbm::algo::ca
