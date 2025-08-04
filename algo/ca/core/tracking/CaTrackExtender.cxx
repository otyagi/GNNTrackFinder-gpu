/* Copyright (C) 2010-2021 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Maksym Zyzak, Valentina Akishina */

#include "CaTrackExtender.h"

#include "AlgoFairloggerCompat.h"
#include "CaBranch.h"
#include "CaDefines.h"
#include "CaFramework.h"
#include "CaGridArea.h"
#include "CaInputData.h"
#include "CaTrack.h"
#include "CaUtils.h"
#include "CaVector.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"

#include <iostream>


namespace cbm::algo::ca
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  TrackExtender::TrackExtender(const ca::Parameters<fvec>& pars, const fscal mass)
    : fParameters(pars)
    , fSetup(fParameters.GetActiveSetup())
    , fDefaultMass(mass)
  {
  }


  // -------------------------------------------------------------------------------------------------------------------
  //
  TrackExtender::~TrackExtender() {}

  // -------------------------------------------------------------------------------------------------------------------
  //

  void TrackExtender::FitBranchFast(const ca::Branch& t, TrackParamV& Tout, const kf::FitDirection direction,
                                    const fvec qp0, const bool initParams)
  {
    CBMCA_DEBUG_ASSERT(t.NHits >= 3);

    kf::TrackKalmanFilter<fvec> fit;
    fit.SetParticleMass(fDefaultMass);
    fit.SetMask(fmask::One());
    fit.SetTrack(Tout);
    TrackParamV& T = fit.Tr();

    // get hits of current track
    const Vector<ca::HitIndex_t>& hits = t.Hits();  // array of indeses of hits of current track
    const int nHits                    = t.NofHits();

    const signed short int step = (direction == kf::FitDirection::kUpstream ? -1 : 1);  // increment for station index
    const int iFirstHit         = (direction == kf::FitDirection::kUpstream) ? nHits - 1 : 0;
    const int iLastHit          = (direction == kf::FitDirection::kUpstream) ? 0 : nHits - 1;

    const ca::Hit& hit0 = frWData->Hit(hits[iFirstHit]);
    const ca::Hit& hit1 = frWData->Hit(hits[iFirstHit + step]);
    const ca::Hit& hit2 = frWData->Hit(hits[iFirstHit + 2 * step]);

    int ista0 = hit0.Station();
    int ista1 = hit1.Station();
    int ista2 = hit2.Station();

    const ca::Station<fvec>& sta0 = fParameters.GetStation(ista0);
    const ca::Station<fvec>& sta1 = fParameters.GetStation(ista1);
    const ca::Station<fvec>& sta2 = fParameters.GetStation(ista2);

    fvec x0 = hit0.X();
    fvec y0 = hit0.Y();
    fvec z0 = hit0.Z();

    fvec x1 = hit1.X();
    fvec y1 = hit1.Y();
    fvec z1 = hit1.Z();

    fvec x2 = hit2.X();
    fvec y2 = hit2.Y();

    T.X() = x0;
    T.Y() = y0;
    if (initParams) {
      fvec dzi = fvec(1.) / (z1 - z0);
      T.Tx()   = (x1 - x0) * dzi;
      T.Ty()   = (y1 - y0) * dzi;
      T.Qp()   = qp0;
    }
    fit.SetQp0(qp0);

    T.Z()    = z0;
    T.Time() = hit0.T();
    T.Vi()   = 0.;

    T.ResetErrors(1., 1., .1, .1, 1., (sta0.timeInfo ? hit0.dT2() : 1.e6), 1.e6);
    T.Ndf()     = fvec(2.);
    T.NdfTime() = sta0.timeInfo ? fvec(-1.) : fvec(-2.);

    T.C00() = hit0.dX2();
    T.C10() = hit0.dXY();
    T.C11() = hit0.dY2();

    kf::FieldRegion<fvec> fld _fvecalignment;
    fvec fldZ0 = sta1.fZ;  // suppose field is smoth
    fvec fldZ1 = sta2.fZ;
    fvec fldZ2 = sta0.fZ;


    kf::FieldValue fldB0 = sta1.fieldSlice.GetFieldValue(x1, y1);
    kf::FieldValue fldB1 = sta2.fieldSlice.GetFieldValue(x2, y2);
    kf::FieldValue fldB2 = sta0.fieldSlice.GetFieldValue(x0, y0);

    fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

    for (int i = iFirstHit + step; step * i <= step * iLastHit; i += step) {
      const ca::Hit& hit           = frWData->Hit(hits[i]);
      int ista                     = hit.Station();
      const ca::Station<fvec>& sta = fParameters.GetStation(ista);

      fit.Extrapolate(hit.Z(), fld);
      ca::utils::FilterHit(fit, hit, fmask(sta.timeInfo));
      auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(T.X(), T.Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, direction);

      fldB0 = fldB1;
      fldB1 = fldB2;
      fldZ0 = fldZ1;
      fldZ1 = fldZ2;
      fldB2 = sta.fieldSlice.GetFieldValue(hit.X(), hit.Y());
      fldZ2 = sta.fZ;
      fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);
    }  // i

    Tout = T;
  }  // void ca::Framework::BranchFitterFast

  /// like BranchFitterFast but more precise
  void TrackExtender::FitBranch(const ca::Branch& t, TrackParamV& T, const kf::FitDirection direction, const fvec qp0,
                                const bool initParams)
  {
    FitBranchFast(t, T, direction, qp0, initParams);
    for (int i = 0; i < 1; i++) {
      FitBranchFast(t, T, !direction, T.Qp(), false);
      FitBranchFast(t, T, direction, T.Qp(), false);
    }
  }  // void ca::Framework::BranchFitter


  void TrackExtender::FindMoreHits(ca::Branch& t, TrackParamV& Tout, const kf::FitDirection direction, const fvec qp0)
  {
    Vector<ca::HitIndex_t> newHits{"ca::TrackExtender::newHits"};
    newHits.reserve(fParameters.GetNstationsActive());

    kf::TrackKalmanFilter<fvec> fit;
    fit.SetParticleMass(fDefaultMass);
    fit.SetMask(fmask::One());
    fit.SetTrack(Tout);
    fit.SetQp0(qp0);

    const signed short int step = (direction == kf::FitDirection::kUpstream) ? -1 : 1;  // increment for station index
    const int iFirstHit         = (direction == kf::FitDirection::kUpstream) ? 2 : t.NofHits() - 3;
    //  int ista = fInputData->GetHit(t.Hits[iFirstHit]).iSt + 2 * step; // current station. set to the end of track

    const ca::Hit& hit0 = frWData->Hit(t.Hits()[iFirstHit]);  // optimize
    const ca::Hit& hit1 = frWData->Hit(t.Hits()[iFirstHit + step]);
    const ca::Hit& hit2 = frWData->Hit(t.Hits()[iFirstHit + 2 * step]);

    const int ista0 = hit0.Station();
    const int ista1 = hit1.Station();
    const int ista2 = hit2.Station();

    const ca::Station<fvec>& sta0 = fParameters.GetStation(ista0);
    const ca::Station<fvec>& sta1 = fParameters.GetStation(ista1);
    const ca::Station<fvec>& sta2 = fParameters.GetStation(ista2);

    fvec x0 = hit0.X();
    fvec y0 = hit0.Y();

    fvec x1 = hit1.X();
    fvec y1 = hit1.Y();

    fvec x2 = hit2.X();
    fvec y2 = hit2.Y();

    kf::FieldRegion<fvec> fld _fvecalignment;
    fvec fldZ0 = sta1.fZ;
    fvec fldZ1 = sta2.fZ;
    fvec fldZ2 = sta0.fZ;

    kf::FieldValue fldB0 = sta1.fieldSlice.GetFieldValue(x1, y1);
    kf::FieldValue fldB1 = sta2.fieldSlice.GetFieldValue(x2, y2);
    kf::FieldValue fldB2 = sta0.fieldSlice.GetFieldValue(x0, y0);

    fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

    int ista = ista2 + 2 * step;  // skip one station. if there would be hit it has to be found on previous stap

    if (ista2 == frWData->CurrentIteration()->GetFirstStationIndex()) ista = ista2 + step;

    const fscal pickGather = frWData->CurrentIteration()->GetPickGather();
    const fvec pickGather2 = pickGather * pickGather;
    const fvec maxDZ       = frWData->CurrentIteration()->GetMaxDZ();
    for (; (ista < fParameters.GetNstationsActive()) && (ista >= 0); ista += step) {  // CHECKME why ista2?

      const ca::Station<fvec>& sta = fParameters.GetStation(ista);

      fit.Extrapolate(sta.fZ, fld);

      fscal r2_best = 1e8;  // best distance to hit
      int iHit_best = -1;   // index of the best hit

      TrackParamV& tr = fit.Tr();

      const auto& grid = frWData->Grid(ista);
      ca::GridArea area(grid, tr.X()[0], tr.Y()[0],
                        (sqrt(pickGather * tr.C00()) + grid.GetMaxRangeX() + maxDZ * kf::utils::fabs(tr.Tx()))[0],
                        (sqrt(pickGather * tr.C11()) + grid.GetMaxRangeY() + maxDZ * kf::utils::fabs(tr.Ty()))[0]);

      if (fParameters.DevIsIgnoreHitSearchAreas()) {
        area.DoLoopOverEntireGrid();
      }

      ca::HitIndex_t ih = 0;

      while (area.GetNextObjectId(ih)) {  // loop over the hits in the area

        if (frWData->IsHitSuppressed(ih)) {
          continue;
        }
        const ca::Hit& hit = frWData->Hit(ih);

        if (sta.timeInfo && tr.NdfTime()[0] > -2.) {
          fscal dt = hit.T() - tr.Time()[0];
          if (fabs(dt) > sqrt(25. * tr.C55()[0]) + hit.RangeT()) continue;
        }

        //if (GetFUsed((*fStripFlag)[hit.f] | (*fStripFlag)[hit.b])) continue;  // if used

        if (frWData->IsHitKeyUsed(hit.FrontKey()) || frWData->IsHitKeyUsed(hit.BackKey())) {
          continue;
        }

        auto [y, C11] = fit.ExtrapolateLineYdY2(hit.Z());

        //   fscal dym_est = ( fPickGather * sqrt(fabs(C11[0]+sta.XYInfo.C11[0])) );
        //   fscal y_minus_new = y[0] - dym_est;
        // if (yh < y_minus_new) continue;  // CHECKME take into account overlaping?

        auto [x, C00] = fit.ExtrapolateLineXdX2(hit.Z());

        fscal d_x = hit.X() - x[0];
        fscal d_y = hit.Y() - y[0];
        fscal d2  = d_x * d_x + d_y * d_y;
        if (d2 > r2_best) continue;
        fscal dxm_est = sqrt(pickGather2 * C00)[0] + grid.GetMaxRangeX();
        if (fabs(d_x) > dxm_est) continue;

        fscal dym_est = sqrt(pickGather2 * C11)[0] + grid.GetMaxRangeY();
        if (fabs(d_y) > dym_est) continue;

        r2_best   = d2;
        iHit_best = ih;
      }

      if (iHit_best < 0) break;


      const ca::Hit& hit = frWData->Hit(iHit_best);
      newHits.push_back(iHit_best);

      fit.Extrapolate(hit.Z(), fld);
      ca::utils::FilterHit(fit, hit, fmask(sta.timeInfo));
      auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, direction);

      fldB0 = fldB1;
      fldB1 = fldB2;
      fldZ0 = fldZ1;
      fldZ1 = fldZ2;
      fldB2 = sta.fieldSlice.GetFieldValue(hit.X(), hit.Y());
      fldZ2 = sta.fZ;
      fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);
    }

    // save hits
    const unsigned int NOldHits = t.NofHits();
    const unsigned int NNewHits = newHits.size();
    t.RefHits().enlarge(NNewHits + NOldHits);

    if (direction == kf::FitDirection::kUpstream) {
      for (int i = NOldHits - 1; i >= 0; i--) {
        t.RefHits()[NNewHits + i] = t.RefHits()[i];
      }
      for (unsigned int i = 0, ii = NNewHits - 1; i < NNewHits; i++, ii--) {
        t.RefHits()[i] = newHits[ii];
      }
    }
    else {  // downstream
      for (unsigned int i = 0; i < newHits.size(); i++) {
        t.RefHits()[NOldHits + i] = newHits[i];
      }
    }

    Tout = fit.Tr();

  }  // void ca::Framework::FindMoreHits

  /// Try to extrapolate and find additional hits on other stations
  fscal TrackExtender::ExtendBranch(ca::Branch& t, WindowData& wData)
  {
    frWData = &wData;
    //   const unsigned int minNHits = 3;

    TrackParamV T;

    // downstream

    FitBranch(t, T, kf::FitDirection::kDownstream, 0.0);
    FindMoreHits(t, T, kf::FitDirection::kDownstream, T.Qp());

    // upstream

    FitBranchFast(t, T, kf::FitDirection::kUpstream, T.Qp(), false);
    FindMoreHits(t, T, kf::FitDirection::kUpstream, T.Qp());

    return T.GetChiSq()[0];
  }
}  // namespace cbm::algo::ca
