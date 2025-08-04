/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#include "CbmKfUtil.h"

#include "FairTrackParam.h"
#include "KfDefs.h"

namespace cbm::kf
{
  cbm::algo::kf::TrackParamD ConvertTrackParam(const FairTrackParam& par)
  {
    cbm::algo::kf::TrackParamD t;
    t.X()    = par.GetX();
    t.Y()    = par.GetY();
    t.Tx()   = par.GetTx();
    t.Ty()   = par.GetTy();
    t.Qp()   = par.GetQp();
    t.Z()    = par.GetZ();
    t.Time() = 0.;
    t.Vi()   = cbm::algo::kf::defs::SpeedOfLightInv<>;

    t.CovMatrix().fill(0.);

    t.ChiSq()     = 0.;
    t.Ndf()       = 0.;
    t.ChiSqTime() = 0.;
    t.NdfTime()   = 0.;

    for (Int_t i = 0; i < 5; i++) {
      for (Int_t j = 0; j <= i; j++) {
        t.C(i, j) = par.GetCovariance(i, j);
      }
    }

    t.C55() = 1.;
    t.C66() = 1.;

    return t;
  }

  FairTrackParam ConvertTrackParam(const cbm::algo::kf::TrackParamD& t)
  {
    FairTrackParam par;
    par.SetX(t.GetX());
    par.SetY(t.GetY());
    par.SetZ(t.GetZ());
    par.SetTx(t.GetTx());
    par.SetTy(t.GetTy());
    par.SetQp(t.GetQp());
    for (Int_t i = 0; i < 5; i++) {
      for (Int_t j = 0; j <= i; j++) {
        par.SetCovariance(i, j, t.C(i, j));
      }
    }
    return par;
  }


}  // namespace cbm::kf
