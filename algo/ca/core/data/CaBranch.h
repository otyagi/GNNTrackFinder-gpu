/* Copyright (C) 2007-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov [committer], Maksym Zyzak, Valentina Akishina */

/// @file CaBranch.h
/// @author Sergey Gorbunov

#pragma once  // include this header only once per compilation unit

#include "CaHit.h"
#include "CaVector.h"

namespace cbm::algo::ca
{

  ///  The class describes a combinatorial branch of the CA tracker
  ///
  class Branch {
   public:
    /// default constructor
    Branch() { fHits.reserve(25); }

    ///------------------------------
    /// Setters and getters

    void SetStation(int iStation) { fStation = iStation; }
    void SetChi2(fscal chi2) { fChi2 = chi2; }
    void SetId(int Id) { fId = Id; }
    void SetAlive(bool isAlive) { fIsAlive = isAlive; }

    void AddHit(ca::HitIndex_t hitIndex) { fHits.push_back(hitIndex); }
    void ResetHits() { fHits.clear(); }

    int NofHits() const { return fHits.size(); }
    int Station() const { return fStation; }
    fscal Chi2() const { return fChi2; }
    int Id() const { return fId; }
    bool IsAlive() const { return fIsAlive; }
    const Vector<ca::HitIndex_t>& Hits() const { return fHits; }

    Vector<ca::HitIndex_t>& RefHits() { return fHits; }

    ///------------------------------
    /// Methods

    bool IsBetterThan(const Branch& b) const
    {
      if (NofHits() != b.NofHits()) return (NofHits() > b.NofHits());
      if (Station() != b.Station()) return (Station() < b.Station());
      return (Chi2() <= b.Chi2());
    }

   private:
    ///------------------------------
    /// Data members

    int fStation{0};
    fscal fChi2{0.};
    int fId{0};
    bool fIsAlive{0};
    Vector<ca::HitIndex_t> fHits{"Branch::fHits"};
  };

}  // namespace cbm::algo::ca
