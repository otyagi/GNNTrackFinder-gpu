/* Copyright (C) 2019-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina, Sergey Gorbunov[committer] */

/// \file CaTriplet.h
/// \author Sergey Gorbunov
/// \author Valentina Akishina
/// \date 2021-05-18

#pragma once  // include this header only once per compilation unit

#include "CaHit.h"
#include "CaSimd.h"

#include <string>

namespace cbm::algo::ca
{

  /// \brief Triplet class represents a short 3-hits track segment called a "triplet".
  ///
  class Triplet {
   public:
    /// Default constructor
    Triplet() = default;

    /// Constructor
    XPU_D Triplet(ca::HitIndex_t iHitL, ca::HitIndex_t iHitM, ca::HitIndex_t iHitR, unsigned int iStaL,
                  unsigned int iStaM, unsigned int iStaR, unsigned char Level, unsigned int firstNeighbour,
                  char nNeighbours, fscal Chi2, fscal Qp, fscal Cqp, fscal tx, fscal Ctx, fscal ty, fscal Cty,
                  bool isMomentumFitted)
      : fChi2(Chi2)
      , fQp(Qp)
      , fCqp(Cqp)
      , fTx(tx)
      , fCtx(Ctx)
      , fTy(ty)
      , fCty(Cty)
      , fFirstNeighbour(firstNeighbour)
      , fHitL(iHitL)
      , fHitM(iHitM)
      , fHitR(iHitR)
      , fNneighbours(nNeighbours)
      , fLevel(Level)
      , fSta((iStaL << 4) + ((iStaM - iStaL - 1) << 2) + (iStaR - iStaL - 2))
      , fIsMomentumFitted(isMomentumFitted)
    {
    }

    /// Setters and getters

    void SetLevel(unsigned char Level) { fLevel = Level; }
    unsigned char GetLevel() const { return fLevel; }

    ca::HitIndex_t GetLHit() const { return fHitL; }
    ca::HitIndex_t GetMHit() const { return fHitM; }
    ca::HitIndex_t GetRHit() const { return fHitR; }

    void SetNNeighbours(int n) { fNneighbours = n; }
    int GetNNeighbours() const { return fNneighbours; }

    void SetFNeighbour(unsigned int n) { fFirstNeighbour = n; }
    unsigned int GetFNeighbour() const { return fFirstNeighbour; }

    fscal GetQp() const { return fQp; }
    fscal GetChi2() const { return fChi2; }
    fscal GetTime() const { return -111.; }

    int GetLSta() const { return fSta >> 4; }
    int GetMSta() const { return ((fSta % 16) >> 2) + GetLSta() + 1; }
    int GetRSta() const { return (fSta % 4) + GetLSta() + 2; }

    fscal GetCqp() const { return fCqp; }
    fscal GetTx() const { return fTx; }
    fscal GetCtx() const { return fCtx; }
    fscal GetTy() const { return fTy; }
    fscal GetCty() const { return fCty; }

    bool IsMomentumFitted() const { return fIsMomentumFitted; }
    void SetIsMomentumFitted(bool val) { fIsMomentumFitted = val; }

    /// String representation of class contents
    /// \param indentLevel      number of indent characters in the output
    std::string ToString(int indentLevel = 0) const;

   private:
    ///-----------------------------------------------------------------------------------------------
    /// Data members

    fscal fChi2{0.};  ///< chi^2
    fscal fQp{0.};    ///< q/p
    fscal fCqp{0.};   ///< RMS^2 of q/p
    fscal fTx{0.};    ///< tx at the left hit
    fscal fCtx{0.};   ///< RMS^2 of tx
    fscal fTy{0.};    ///< ty at the left hit
    fscal fCty{0.};   ///< RMS^2 of ty

    unsigned int fFirstNeighbour{0};  ///< ID of the first neighbouring triplet
    ca::HitIndex_t fHitL{0};          ///< left hit index (16b) in vHits array
    ca::HitIndex_t fHitM{0};          ///< middle hit index (16b)
    ca::HitIndex_t fHitR{0};          ///< right hit index (16b)
    int fNneighbours{0};              ///< n of neighbouring triplets

    /// Triplet level - its possible position on the longest track candidate it belongs to.
    /// level 0 = rightmost triplet of a track candidate
    /// level k = k-ths triplet along the track counting upstream, from right to left.
    unsigned char fLevel{0};

    unsigned short fSta{0};     ///< packed station numbers: staL (12b), staM-1-staL (2b), staR-2-staL (2b)
    bool fIsMomentumFitted{0};  ///< if the triplet momentum is fitted
  };

}  // namespace cbm::algo::ca
