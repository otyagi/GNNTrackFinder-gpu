/* Copyright (C) 2010-2020 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina, Igor Kulakov [committer], Maksym Zyzak */

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaHit.h"
#include "CaSimd.h"

namespace cbm::algo::ca
{

  /// \brief A class to store hit information on the ca::Grid
  ///
  struct GridEntry {

    GridEntry() = default;

    GridEntry(const ca::Hit& hit, ca::HitIndex_t id) { Set(hit, id); };

    void Set(const ca::Hit& hit, ca::HitIndex_t id)
    {
      fObjectId = id;
      x         = hit.X();
      y         = hit.Y();
      z         = hit.Z();
      t         = hit.T();
      rangeX    = hit.RangeX();
      rangeY    = hit.RangeY();
      rangeT    = hit.RangeT();
    }

    ca::HitIndex_t GetObjectId() const { return fObjectId; }

    fscal Z() const { return z; }
    fscal T() const { return t; }
    fscal X() const { return x; }
    fscal Y() const { return y; }
    fscal RangeX() const { return rangeX; }
    fscal RangeY() const { return rangeY; }
    fscal RangeT() const { return rangeT; }

   private:
    constexpr static fscal kUndef = 0.;  // TODO: test with constants::Undef<fscal>

    ca::HitIndex_t fObjectId{0};  ///< hit id in L1Algo::fWindowHits array

    fscal x{kUndef};       // x coordinate of the hit
    fscal y{kUndef};       // y coordinate of the hit
    fscal z{kUndef};       // z coordinate of the hit
    fscal t{kUndef};       // t coordinate of the hit
    fscal rangeX{kUndef};  // x range of the hit
    fscal rangeY{kUndef};  // y range of the hit
    fscal rangeT{kUndef};  // t range of the hit
  };

}  // namespace cbm::algo::ca
