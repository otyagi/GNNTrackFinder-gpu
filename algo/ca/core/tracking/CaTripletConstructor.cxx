/* Copyright (C) 2010-2021 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Maksym Zyzak, Valentina Akishina */

#include "CaTripletConstructor.h"

#include "CaDefines.h"
#include "CaFramework.h"
#include "CaGridArea.h"

#include <algorithm>
#include <iostream>
// #include "CaToolsDebugger.h"
#include "AlgoFairloggerCompat.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"

// using cbm::ca::tools::Debugger;

namespace cbm::algo::ca
{
  TripletConstructor::TripletConstructor(const ca::Parameters<fvec>& pars, WindowData& wData, const fscal mass,
                                         const ca::TrackingMode& mode)
    : fParameters(pars)
    , fSetup(fParameters.GetActiveSetup())
    , frWData(wData)
    , fDefaultMass(mass)
    , fTrackingMode(mode)
  {
    // FIXME: SZh 24.08.2022
    //        This approach is suitable only for a case, when all the stations inside a magnetic field come right before
    //        all the stations outside of the field!
    fNfieldStations = std::lower_bound(fParameters.GetStations().cbegin(),
                                       fParameters.GetStations().cbegin() + fParameters.GetNstationsActive(),
                                       0,  // we are looking for the first zero element
                                       [](const ca::Station<fvec>& s, int edge) { return bool(s.fieldStatus) > edge; })
                      - fParameters.GetStations().cbegin();

    fIsTargetField = !frWData.TargB().IsZero();
  }


  bool TripletConstructor::InitStations(int istal, int istam, int istar)
  {
    fIstaL = istal;
    fIstaM = istam;
    fIstaR = istar;

    if (fIstaM >= fParameters.GetNstationsActive()) {
      return false;
    }
    fStaL = &fParameters.GetStation(fIstaL);
    fStaM = &fParameters.GetStation(fIstaM);
    fStaR = &fParameters.GetStation(fIstaR);

    {  // two stations for approximating the field between the target and the left hit
      const int sta1 = (fNfieldStations <= 1) ? 1 : std::clamp(fIstaL, 1, fNfieldStations - 1);
      const int sta0 = sta1 / 2;  // station between fIstaL and the target

      assert(0 <= sta0 && sta0 < sta1 && sta1 < fParameters.GetNstationsActive());
      fFld0Sta[0] = &fParameters.GetStation(sta0);
      fFld0Sta[1] = &fParameters.GetStation(sta1);
    }

    {  // three stations for approximating the field between the left and the right hit

      int sta0 = fIstaL;
      int sta1 = fIstaM;
      int sta2 = fIstaM + 1;
      if (sta2 >= fParameters.GetNstationsActive()) {
        sta2 = fIstaM;
        sta1 = fIstaM - 1;
        sta0 = (sta1 <= 0) ? 0 : std::clamp(sta0, 0, sta1 - 1);
      }
      if (fParameters.GetNstationsActive() >= 3) {
        assert(0 <= sta0 && sta0 < sta1 && sta1 < sta2 && sta2 < fParameters.GetNstationsActive());
      }

      fFld1Sta[0] = &fParameters.GetStation(sta0);
      fFld1Sta[1] = &fParameters.GetStation(sta1);
      fFld1Sta[2] = &fParameters.GetStation(sta2);
    }
    return true;
  }


  void TripletConstructor::CreateTripletsForHit(Vector<ca::Triplet>& tripletsOut, int istal, int istam, int istar,
                                                ca::HitIndex_t iHitL)
  {
    if (!InitStations(istal, istam, istar)) {
      tripletsOut.clear();
      return;
    }

    kf::TrackKalmanFilter<fvec> fit(fmask::One(), true);
    fit.SetParticleMass(frWData.CurrentIteration()->GetElectronFlag() ? constants::phys::ElectronMass : fDefaultMass);
    fit.SetQp0(frWData.CurrentIteration()->GetMaxQp());

    fIhitL           = iHitL;
    const auto& hitl = frWData.Hit(fIhitL);

    // fit a straight line through the target and the left hit.
    TrackParamV& T = fit.Tr();
    {
      /// Get the field approximation. Add the target to parameters estimation.
      /// Propagaete to the middle station of a triplet.
      //kf::FieldValue<fvec> lB, mB, cB, rB _fvecalignment;           currently not used
      //kf::FieldValue<fvec> l_B_global, centB_global _fvecalignment; currently not used

      // get the magnetic field along the track.
      // (suppose track is straight line with origin in the target)
      {
        const fvec dzli = 1.f / (hitl.Z() - fParameters.GetTargetPositionZ());

        T.X()    = hitl.X();
        T.Y()    = hitl.Y();
        T.Z()    = hitl.Z();
        T.Tx()   = (hitl.X() - fParameters.GetTargetPositionX()) * dzli;
        T.Ty()   = (hitl.Y() - fParameters.GetTargetPositionY()) * dzli;
        T.Qp()   = fvec(0.);
        T.Time() = hitl.T();
        T.Vi()   = constants::phys::SpeedOfLightInv;

        const fvec maxSlopePV = frWData.CurrentIteration()->GetMaxSlopePV();
        const fvec maxQp      = frWData.CurrentIteration()->GetMaxQp();
        const fvec txErr2     = maxSlopePV * maxSlopePV / fvec(9.);
        const fvec qpErr2     = maxQp * maxQp / fvec(9.);

        T.ResetErrors(1., 1., txErr2, txErr2, qpErr2, (fStaL->timeInfo ? hitl.dT2() : 1.e6), 1.e10);
        T.InitVelocityRange(1. / frWData.CurrentIteration()->GetMaxQp());

        T.C00()     = hitl.dX2();
        T.C10()     = hitl.dXY();
        T.C11()     = hitl.dY2();
        T.Ndf()     = (frWData.CurrentIteration()->GetPrimaryFlag()) ? fvec(2.) : fvec(0.);
        T.NdfTime() = (fStaL->timeInfo ? 0 : -1);
      }

      // NDF = number of track parameters (6: x, y, tx, ty, qp, time)
      //       - number of measured parameters (3: x, y, time) on station or (2: x, y) on target

      // field made by  the left hit, the target and the station istac in-between.
      // is used for extrapolation to the target and to the middle hit
      kf::FieldRegion<fvec> fld0;
      {
        kf::FieldValue<fvec> B0 = fFld0Sta[0]->fieldSlice.GetFieldValueForLine(T);
        kf::FieldValue<fvec> B1 = fFld0Sta[1]->fieldSlice.GetFieldValueForLine(T);
        fld0.Set(frWData.TargB(), fParameters.GetTargetPositionZ(), B0, fFld0Sta[0]->fZ, B1, fFld0Sta[1]->fZ);
      }

      {  // field, made by the left hit, the middle station and the right station
        // Will be used for extrapolation to the right hit
        kf::FieldValue<fvec> B0 = fFld1Sta[0]->fieldSlice.GetFieldValueForLine(T);
        kf::FieldValue<fvec> B1 = fFld1Sta[1]->fieldSlice.GetFieldValueForLine(T);
        kf::FieldValue<fvec> B2 = fFld1Sta[2]->fieldSlice.GetFieldValueForLine(T);
        fFldL.Set(B0, fFld1Sta[0]->fZ, B1, fFld1Sta[1]->fZ, B2, fFld1Sta[2]->fZ);
      }

      // add the target constraint
      fit.FilterWithTargetAtLine(fParameters.GetTargetPositionZ(), frWData.TargetMeasurement(), fld0);
      fit.MultipleScattering(fSetup.GetMaterial(fIstaL).GetThicknessX0(T.GetX(), T.GetY()));

      // extrapolate to the middle hit
      fit.ExtrapolateLine(fStaM->fZ, fFldL);
    }

    /// Find the doublets. Reformat data into portions of doublets.
    auto FindDoubletHits = [&]() {
      const bool matchMc = fParameters.DevIsMatchDoubletsViaMc();
      const int iMC      = matchMc ? ca::Framework::GetMcTrackIdForWindowHit(fIhitL) : -1;
      fDoubletData.second.clear();
      if (iMC < 0 && matchMc) {
        return;
      }
      CollectHits(fDoubletData.second, fit, fIstaM, frWData.CurrentIteration()->GetDoubletChi2Cut(), iMC,
                  fParameters.GetMaxDoubletsPerSinglet());
    };

    FindDoubletHits();
    FindDoublets(fit);

    //D.Smith 28.8.24: Moving this upward (before doublet finding) changes QA output slightly
    if (fIstaR >= fParameters.GetNstationsActive()) {
      tripletsOut.clear();
      return;
    }

    FindTripletHits();
    FindTriplets();
    SelectTriplets(tripletsOut);
  }


  void TripletConstructor::FindDoublets(kf::TrackKalmanFilter<fvec>& fit)
  {
    // ---- Add the middle hits to parameters estimation ----
    Vector<TrackParamV>& tracks   = fDoubletData.first;
    Vector<ca::HitIndex_t>& hitsM = fDoubletData.second;

    tracks.clear();
    tracks.reserve(hitsM.size());

    const bool isMomentumFitted = (fIsTargetField || (fStaL->fieldStatus != 0) || (fStaM->fieldStatus != 0));

    const TrackParamV Tr = fit.Tr();  // copy contents of fit

    auto it2 = hitsM.begin();
    for (auto it = hitsM.begin(); it != hitsM.end(); it++) {

      const ca::HitIndex_t indM = *it;
      const ca::Hit& hitm       = frWData.Hit(indM);

      if (frWData.IsHitSuppressed(indM)) {
        continue;
      }

      TrackParamV& T2 = fit.Tr();
      T2              = Tr;
      fit.SetQp0(fvec(0.f));

      fvec z_2 = hitm.Z();
      kf::MeasurementXy<fvec> m_2(hitm.X(), hitm.Y(), hitm.dX2(), hitm.dY2(), hitm.dXY(), fvec::One(), fvec::One());
      fvec t_2   = hitm.T();
      fvec dt2_2 = hitm.dT2();

      // add the middle hit
      fit.ExtrapolateLineNoField(z_2);
      fit.FilterXY(m_2);
      fit.FilterTime(t_2, dt2_2, fmask(fStaM->timeInfo));
      fit.SetQp0(isMomentumFitted ? fit.Tr().GetQp() : frWData.CurrentIteration()->GetMaxQp());
      fit.MultipleScattering(fSetup.GetMaterial(fIstaM).GetThicknessX0(T2.GetX(), T2.Y()));
      fit.SetQp0(fit.Tr().Qp());

      // check if there are other hits close to the doublet on the same station
      if (ca::kMcbm != fTrackingMode) {
        // TODO: SG: adjust cuts, put them to parameters

        const fscal tx = T2.Tx()[0];
        const fscal ty = T2.Ty()[0];
        const fscal tt = T2.Vi()[0] * sqrt(1. + tx * tx + ty * ty);  // dt/dl * dl/dz

        for (auto itClone = it + 1; itClone != hitsM.end(); itClone++) {

          const int indClone      = *itClone;
          const ca::Hit& hitClone = frWData.Hit(indClone);

          const fscal dz = hitClone.Z() - T2.Z()[0];

          if ((fStaM->timeInfo) && (T2.NdfTime()[0] >= 0)) {
            const fscal dt = T2.Time()[0] + tt * dz - hitClone.T();
            if (!(fabs(dt) <= 3.5 * sqrt(T2.C55()[0]) + hitClone.RangeT())) {
              continue;
            }
          }

          const fscal dx = T2.GetX()[0] + tx * dz - hitClone.X();
          if (!(fabs(dx) <= 3.5 * sqrt(T2.C00()[0]) + hitClone.RangeX())) {
            continue;
          }

          const fscal dy = T2.Y()[0] + ty * dz - hitClone.Y();
          if (!(fabs(dy) <= 3.5 * sqrt(T2.C11()[0]) + hitClone.RangeY())) {
            continue;
          }

          if (fParameters.DevIsSuppressOverlapHitsViaMc()) {
            const int iMC = ca::Framework::GetMcTrackIdForWindowHit(fIhitL);
            if ((iMC != ca::Framework::GetMcTrackIdForWindowHit(indM))
                || (iMC != ca::Framework::GetMcTrackIdForWindowHit(indClone))) {
              continue;
            }
          }

          frWData.SuppressHit(indClone);
        }
      }

      tracks.push_back(T2);
      *it2 = indM;
      it2++;
    }  // it
    hitsM.shrink(std::distance(hitsM.begin(), it2));
  }


  void TripletConstructor::FindTripletHits()
  {
    //auto& [tracks_2, hitsM_2] = doublets;   TO DO: Reactivate when MacOS compiler bug is fixed.
    Vector<TrackParamV>& tracks_2   = fDoubletData.first;
    Vector<ca::HitIndex_t>& hitsM_2 = fDoubletData.second;

    Vector<ca::HitIndex_t>& hitsM_3 = std::get<1>(fTripletData);
    Vector<ca::HitIndex_t>& hitsR_3 = std::get<2>(fTripletData);

    /// Add the middle hits to parameters estimation. Propagate to right station.
    /// Find the triplets(right hit). Reformat data in the portion of triplets.
    kf::TrackKalmanFilter<fvec> fit(fmask::One(), true);
    fit.SetParticleMass(frWData.CurrentIteration()->GetElectronFlag() ? constants::phys::ElectronMass : fDefaultMass);
    fit.SetQp0(fvec(0.));

    {
      const int maxTriplets = hitsM_2.size() * fParameters.GetMaxTripletPerDoublets();
      hitsM_3.clear();
      hitsR_3.clear();
      hitsM_3.reserve(maxTriplets);
      hitsR_3.reserve(maxTriplets);
    }
    // ---- Add the middle hits to parameters estimation. Propagate to right station. ----

    const double maxSlope       = frWData.CurrentIteration()->GetMaxSlope();
    const double tripletChi2Cut = frWData.CurrentIteration()->GetTripletChi2Cut();
    for (unsigned int i2 = 0; i2 < hitsM_2.size(); i2++) {

      fit.SetTrack(tracks_2[i2]);
      TrackParamV& T2 = fit.Tr();

      // extrapolate to the right hit station
      fit.Extrapolate(fStaR->fZ, fFldL);

      if constexpr (fDebugDublets) {
        ca::HitIndex_t iwhit[2] = {fIhitL, hitsM_2[i2]};
        ca::HitIndex_t ihit[2]  = {frWData.Hit(iwhit[0]).Id(), frWData.Hit(iwhit[1]).Id()};

        const int ih0     = ihit[0];
        const int ih1     = ihit[1];
        const ca::Hit& h0 = frWData.Hit(iwhit[0]);
        const ca::Hit& h1 = frWData.Hit(iwhit[1]);

        LOG(info) << "\n======  Extrapolated Doublet : "
                  << "  iter " << frWData.CurrentIteration()->GetName() << " hits: {" << fIstaL << "/" << ih0 << " "
                  << fIstaM << "/" << ih1 << " " << fIstaR << "/?} xyz: {" << h0.X() << " " << h0.Y() << " " << h0.Z()
                  << "}, {" << h1.X() << " " << h1.Y() << " " << h1.Z() << "} chi2 " << T2.GetChiSq()[0] << " ndf "
                  << T2.Ndf()[0] << " chi2time " << T2.ChiSqTime()[0] << " ndfTime " << T2.NdfTime()[0];
        LOG(info) << "  extr. track: " << T2.ToString(0);
      }

      // ---- Find the triplets(right hit). Reformat data in the portion of triplets. ----
      int iMC = -1;

      auto rejectDoublet = [&]() -> bool {
        if (ca::TrackingMode::kSts == fTrackingMode && (T2.C44()[0] < 0)) {
          return true;
        }
        if (T2.C00()[0] < 0 || T2.C11()[0] < 0 || T2.C22()[0] < 0 || T2.C33()[0] < 0 || T2.C55()[0] < 0) {
          return true;
        }
        if (fabs(T2.Tx()[0]) > maxSlope) {
          return true;
        }
        if (fabs(T2.Ty()[0]) > maxSlope) {
          return true;
        }
        if (fParameters.DevIsMatchTripletsViaMc()) {
          int indM = hitsM_2[i2];
          iMC      = ca::Framework::GetMcTrackIdForWindowHit(fIhitL);
          if (iMC < 0 || iMC != ca::Framework::GetMcTrackIdForWindowHit(indM)) {
            return true;
          }
        }
        return false;
      };
      const bool isDoubletGood = !rejectDoublet();

      if constexpr (fDebugDublets) {
        if (isDoubletGood) {
          LOG(info) << "  extrapolated doublet accepted";
        }
        else {
          LOG(info) << "  extrapolated doublet rejected";
          LOG(info) << "======== end of extrapolated doublet ==== \n";
        }
      }

      if (!isDoubletGood) {
        continue;
      }

      Vector<ca::HitIndex_t> collectedHits;
      CollectHits(collectedHits, fit, fIstaR, tripletChi2Cut, iMC, fParameters.GetMaxTripletPerDoublets());

      if (collectedHits.size() >= fParameters.GetMaxTripletPerDoublets()) {
        // FU, 28.08.2024, Comment the following log lines since it spams the output
        // of our tests and finally results in crashes on run4
        //      LOG(debug) << "Ca: GetMaxTripletPerDoublets==" << fParameters.GetMaxTripletPerDoublets()
        //                 << " reached in findTripletsStep0()";
        // reject already created triplets for this doublet
        collectedHits.clear();
      }
      if constexpr (fDebugDublets) {
        LOG(info) << " collected " << collectedHits.size() << " hits on the right station ";
      }
      for (ca::HitIndex_t& irh : collectedHits) {
        if constexpr (fDebugDublets) {
          const ca::Hit& h    = frWData.Hit(irh);
          ca::HitIndex_t ihit = h.Id();
          LOG(info) << "  hit " << ihit << " " << h.ToString();
        }
        if (frWData.IsHitSuppressed(irh)) {
          if constexpr (fDebugDublets) {
            LOG(info) << "  the hit is suppressed";
          }
          continue;
        }
        // pack the triplet
        hitsM_3.push_back(hitsM_2[i2]);
        hitsR_3.push_back(irh);
      }  // search area
      if constexpr (fDebugDublets) {
        LOG(info) << "======== end of extrapolated doublet ==== \n";
      }
    }  // i2
  }

  void TripletConstructor::FindTriplets()
  {
    constexpr int nIterations = 2;

    Vector<TrackParamV>& tracks   = std::get<0>(fTripletData);
    Vector<ca::HitIndex_t>& hitsM = std::get<1>(fTripletData);
    Vector<ca::HitIndex_t>& hitsR = std::get<2>(fTripletData);
    assert(hitsM.size() == hitsR.size());

    tracks.clear();
    tracks.reserve(hitsM.size());

    /// Refit Triplets
    if constexpr (fDebugTriplets) {
      //cbm::ca::tools::Debugger::Instance().AddNtuple(
      //  "triplets", "ev:iter:i0:x0:y0:z0:i1:x1:y1:z1:i2:x2:y2:z2:mc:sta:p:vx:vy:vz:chi2:ndf:chi2time:ndfTime");
    }

    kf::TrackKalmanFilter<fvec> fit;
    fit.SetMask(fmask::One());
    fit.SetParticleMass(frWData.CurrentIteration()->GetElectronFlag() ? constants::phys::ElectronMass : fDefaultMass);

    // prepare data
    const int NHits       = 3;  // triplets
    const int ista[NHits] = {fIstaL, fIstaM, fIstaR};

    const ca::Station<fvec> sta[NHits] = {fParameters.GetStation(ista[0]), fParameters.GetStation(ista[1]),
                                          fParameters.GetStation(ista[2])};

    const bool isMomentumFitted = ((fStaL->fieldStatus != 0) || (fStaM->fieldStatus != 0) || (fStaR->fieldStatus != 0));
    const fvec ndfTrackModel    = 4 + (isMomentumFitted ? 1 : 0);  // straight line or track with momentum

    for (size_t i3 = 0; i3 < hitsM.size(); ++i3) {

      // prepare data
      const ca::HitIndex_t iwhit[NHits] = {fIhitL, hitsM[i3], hitsR[i3]};

      const ca::HitIndex_t ihit[NHits] = {frWData.Hit(iwhit[0]).Id(), frWData.Hit(iwhit[1]).Id(),
                                          frWData.Hit(iwhit[2]).Id()};

      if (fParameters.DevIsMatchTripletsViaMc()) {
        int mc1 = ca::Framework::GetMcTrackIdForCaHit(ihit[0]);
        int mc2 = ca::Framework::GetMcTrackIdForCaHit(ihit[1]);
        int mc3 = ca::Framework::GetMcTrackIdForCaHit(ihit[2]);
        if ((mc1 != mc2) || (mc1 != mc3)) {
          // D.S.: Added to preserve the ordering when switching from index-based
          // access to push_back(). Discuss with SZ.
          tracks.push_back(TrackParamV());
          continue;
        }
      }

      fscal x[NHits], y[NHits], z[NHits], t[NHits];
      fscal dt2[NHits];
      kf::MeasurementXy<fvec> mxy[NHits];

      for (int ih = 0; ih < NHits; ++ih) {
        const ca::Hit& hit = frWData.Hit(iwhit[ih]);
        mxy[ih] = kf::MeasurementXy<fvec>(hit.X(), hit.Y(), hit.dX2(), hit.dY2(), hit.dXY(), fvec::One(), fvec::One());

        x[ih]   = hit.X();
        y[ih]   = hit.Y();
        z[ih]   = hit.Z();
        t[ih]   = hit.T();
        dt2[ih] = hit.dT2();
      };

      // find the field along the track

      kf::FieldValue<fvec> B[3] _fvecalignment;
      kf::FieldRegion<fvec> fld _fvecalignment;
      kf::FieldRegion<fvec> fldTarget _fvecalignment;

      fvec tx[3] = {(x[1] - x[0]) / (z[1] - z[0]), (x[2] - x[0]) / (z[2] - z[0]), (x[2] - x[1]) / (z[2] - z[1])};
      fvec ty[3] = {(y[1] - y[0]) / (z[1] - z[0]), (y[2] - y[0]) / (z[2] - z[0]), (y[2] - y[1]) / (z[2] - z[1])};

      for (int ih = 0; ih < NHits; ++ih) {
        fvec dz = (sta[ih].fZ - z[ih]);
        B[ih]   = sta[ih].fieldSlice.GetFieldValue(x[ih] + tx[ih] * dz, y[ih] + ty[ih] * dz);
      };

      fld.Set(B[0], sta[0].fZ, B[1], sta[1].fZ, B[2], sta[2].fZ);
      fldTarget.Set(frWData.TargB(), fParameters.GetTargetPositionZ(), B[0], sta[0].fZ, B[1], sta[1].fZ);

      TrackParamV& T = fit.Tr();

      // initial parameters
      {
        fvec dz01 = 1. / (z[1] - z[0]);
        T.Tx()    = (x[1] - x[0]) * dz01;
        T.Ty()    = (y[1] - y[0]) * dz01;
        T.Qp()    = 0.;
        T.Vi()    = 0.;
      }

      // repeat several times in order to improve the precision
      for (int iiter = 0; iiter < nIterations; ++iiter) {

        auto fitTrack = [&](int startIdx, int endIdx, int step, kf::FitDirection direction) {
          const fvec maxQp = frWData.CurrentIteration()->GetMaxQp();
          fit.SetQp0(T.Qp());
          fit.Qp0()(fit.Qp0() > maxQp)  = maxQp;
          fit.Qp0()(fit.Qp0() < -maxQp) = -maxQp;

          int ih0  = startIdx;
          T.X()    = x[ih0];
          T.Y()    = y[ih0];
          T.Z()    = z[ih0];
          T.Time() = t[ih0];
          T.Qp()   = 0.;
          T.Vi()   = 0.;

          T.ResetErrors(1., 1., 1., 1., 100., (sta[ih0].timeInfo ? dt2[ih0] : 1.e6), 1.e2);
          T.C00() = mxy[ih0].Dx2();
          T.C10() = mxy[ih0].Dxy();
          T.C11() = mxy[ih0].Dy2();

          T.Ndf()     = -ndfTrackModel + 2;
          T.NdfTime() = sta[ih0].timeInfo ? 0 : -1;

          if (startIdx == 0) {  //only for the forward fit
            fit.FilterWithTargetAtLine(fParameters.GetTargetPositionZ(), frWData.TargetMeasurement(), fldTarget);
          }

          for (int ih = startIdx + step; ih != endIdx; ih += step) {
            fit.Extrapolate(z[ih], fld);
            auto radThick = fSetup.GetMaterial(ista[ih]).GetThicknessX0(T.X(), T.Y());
            fit.MultipleScattering(radThick);
            fit.EnergyLossCorrection(radThick, direction);
            fit.FilterXY(mxy[ih]);
            fit.FilterTime(t[ih], dt2[ih], fmask(sta[ih].timeInfo));
          }
        };

        // Fit downstream
        fitTrack(0, NHits, 1, kf::FitDirection::kDownstream);

        if (iiter == nIterations - 1) break;

        // Fit upstream
        fitTrack(NHits - 1, -1, -1, kf::FitDirection::kUpstream);
      }  // for iiter

      tracks.push_back(T);

      if constexpr (fDebugTriplets) {
        int ih0 = ihit[0];
        int ih1 = ihit[1];
        int ih2 = ihit[2];
        int mc1 = ca::Framework::GetMcTrackIdForCaHit(ih0);
        int mc2 = ca::Framework::GetMcTrackIdForCaHit(ih1);
        int mc3 = ca::Framework::GetMcTrackIdForCaHit(ih2);

        if (1 || (mc1 >= 0) && (mc1 == mc2) && (mc1 == mc3)) {
          const ca::Hit& h0 = frWData.Hit(iwhit[0]);
          const ca::Hit& h1 = frWData.Hit(iwhit[1]);
          const ca::Hit& h2 = frWData.Hit(iwhit[2]);
          //const CbmL1MCTrack& mctr = CbmL1::Instance()->GetMcTracks()[mc1];
          LOG(info) << "== fitted triplet: "
                    << " iter " << frWData.CurrentIteration()->GetName() << " hits: {" << fIstaL << "/" << ih0 << " "
                    << fIstaM << "/" << ih1 << " " << fIstaR << "/" << ih2 << "} xyz: {" << h0.X() << " " << h0.Y()
                    << " " << h0.Z() << "}, {" << h1.X() << " " << h1.Y() << " " << h1.Z() << "}, {" << h2.X() << ", "
                    << h2.Y() << ", " << h2.Z() << "} chi2 " << T.GetChiSq()[0] << " ndf " << T.Ndf()[0] << " chi2time "
                    << T.ChiSqTime()[0] << " ndfTime " << T.NdfTime()[0];
          /*
          cbm::ca::tools::Debugger::Instance().FillNtuple(
            "triplets", mctr.iEvent, frAlgo.fCurrentIterationIndex, ih0, h0.X(), h0.Y(), h0.Z(), ih1, h1.X(), h1.Y(),
            h1.Z(), ih2, h2.X(), h2.Y(), h2.Z(), mc1, fIstaL, mctr.p, mctr.x, mctr.y, mctr.z, (fscal) T.GetChiSq()[0],
            (fscal) T.Ndf()[0], (fscal) T.ChiSqTime()[0], (fscal) T.NdfTime()[0]);
          */
        }
      }
    }  //i3
  }    // FindTriplets


  void TripletConstructor::SelectTriplets(Vector<ca::Triplet>& tripletsOut)
  {
    /// Selects good triplets and saves them into tripletsOut.
    /// Finds neighbouring triplets at the next station.

    Vector<TrackParamV>& tracks   = std::get<0>(fTripletData);
    Vector<ca::HitIndex_t>& hitsM = std::get<1>(fTripletData);
    Vector<ca::HitIndex_t>& hitsR = std::get<2>(fTripletData);

    bool isMomentumFitted = ((fStaL->fieldStatus != 0) || (fStaM->fieldStatus != 0) || (fStaR->fieldStatus != 0));

    tripletsOut.clear();
    tripletsOut.reserve(hitsM.size());

    for (size_t i3 = 0; i3 < hitsM.size(); ++i3) {

      TrackParamV& T3 = tracks[i3];

      // TODO: SG: normalize chi2, separate cuts on time and space

      const fscal chi2 = T3.GetChiSq()[0] + T3.GetChiSqTime()[0];

      const ca::HitIndex_t ihitl = fIhitL;
      const ca::HitIndex_t ihitm = hitsM[i3];
      const ca::HitIndex_t ihitr = hitsR[i3];

      CBMCA_DEBUG_ASSERT(ihitl < frWData.HitStartIndexOnStation(fIstaL + 1));
      CBMCA_DEBUG_ASSERT(ihitm < frWData.HitStartIndexOnStation(fIstaM + 1));
      CBMCA_DEBUG_ASSERT(ihitr < frWData.HitStartIndexOnStation(fIstaR + 1));

      if (!frWData.CurrentIteration()->GetTrackFromTripletsFlag()) {
        if (chi2 > frWData.CurrentIteration()->GetTripletFinalChi2Cut()) {
          continue;
        }
      }
      // assert(std::isfinite(chi2) && chi2 > 0);

      if (!std::isfinite(chi2) || chi2 < 0) {
        continue;
      }
      //if (!T3.IsEntryConsistent(true, 0)) { continue; }

      fscal qp  = T3.Qp()[0];
      fscal Cqp = T3.C44()[0];

      // TODO: SG: a magic correction that comes from the legacy code
      // removing it leads to a higher ghost ratio
      Cqp += 0.001;

      tripletsOut.emplace_back(ihitl, ihitm, ihitr, fIstaL, fIstaM, fIstaR, 0, 0, 0, chi2, qp, Cqp, T3.Tx()[0],
                               T3.C22()[0], T3.Ty()[0], T3.C33()[0], isMomentumFitted);
    }
  }


  void TripletConstructor::CollectHits(Vector<ca::HitIndex_t>& collectedHits, kf::TrackKalmanFilter<fvec>& fit,
                                       const int iSta, const double chi2Cut, const int iMC, const int maxNhits)
  {
    /// Collect hits on a station
    collectedHits.clear();
    collectedHits.reserve(maxNhits);

    const ca::Station<fvec>& sta = fParameters.GetStation(iSta);

    TrackParamV& T = fit.Tr();
    //LOG(info) << T.chi2[0] ;
    T.ChiSq() = 0.;

    // if make it bigger the found hits will be rejected later because of the chi2 cut.
    const fvec Pick_m22    = (fvec(chi2Cut) - T.GetChiSq());
    const fscal timeError2 = T.C55()[0];
    const fscal time       = T.Time()[0];

    const auto& grid = frWData.Grid(iSta);
    const fvec maxDZ = frWData.CurrentIteration()->GetMaxDZ();
    ca::GridArea area(grid, T.X()[0], T.Y()[0],
                      (sqrt(Pick_m22 * T.C00()) + grid.GetMaxRangeX() + maxDZ * kf::utils::fabs(T.Tx()))[0],
                      (sqrt(Pick_m22 * T.C11()) + grid.GetMaxRangeY() + maxDZ * kf::utils::fabs(T.Ty()))[0]);
    if constexpr (fDebugCollectHits) {
      LOG(info) << "search area: " << T.X()[0] << " " << T.Y()[0] << " "
                << (sqrt(Pick_m22 * T.C00()) + grid.GetMaxRangeX() + maxDZ * kf::utils::fabs(T.Tx()))[0] << " "
                << (sqrt(Pick_m22 * T.C11()) + grid.GetMaxRangeY() + maxDZ * kf::utils::fabs(T.Ty()))[0];
    }
    if (fParameters.DevIsIgnoreHitSearchAreas()) {
      area.DoLoopOverEntireGrid();
    }

    // loop over station hits (index incremented in condition)
    for (ca::HitIndex_t ih = 0; area.GetNextObjectId(ih) && ((int) collectedHits.size() < maxNhits);) {

      if (frWData.IsHitSuppressed(ih)) {
        continue;
      }

      const ca::Hit& hit = frWData.Hit(ih);
      if constexpr (fDebugCollectHits) {
        LOG(info) << "hit in the search area: " << hit.ToString();
        LOG(info) << "   check the hit.. ";
      }

      if (iMC >= 0) {  // match via MC
        if (iMC != ca::Framework::GetMcTrackIdForWindowHit(ih)) {
          if constexpr (fDebugCollectHits) {
            LOG(info) << "   hit mc does not match";
          }
          continue;
        }
      }

      // check time-boundaries

      //TODO: move hardcoded cuts to parameters
      if ((sta.timeInfo) && (T.NdfTime()[0] >= 0)) {
        if (fabs(time - hit.T()) > 1.4 * (3.5 * sqrt(timeError2) + hit.RangeT())) {
          if constexpr (fDebugCollectHits) {
            LOG(info) << "   hit time does not match";
          }
          continue;
        }
        // if (fabs(time - hit.T()) > 30) continue;
      }

      // - check whether hit belong to the window ( track position +\- errors ) -
      const fscal z = hit.Z();

      // check y-boundaries
      const auto [y, C11] = fit.ExtrapolateLineYdY2(z);
      const fscal dy_est  = sqrt(Pick_m22[0] * C11[0]) + hit.RangeY();
      const fscal dY      = hit.Y() - y[0];

      if (fabs(dY) > dy_est) {
        if constexpr (fDebugCollectHits) {
          LOG(info) << "   hit Y does not match";
        }
        continue;
      }

      // check x-boundaries
      const auto [x, C00] = fit.ExtrapolateLineXdX2(z);
      const fscal dx_est  = sqrt(Pick_m22[0] * C00[0]) + hit.RangeX();
      const fscal dX      = hit.X() - x[0];

      if (fabs(dX) > dx_est) {
        if constexpr (fDebugCollectHits) {
          LOG(info) << "   hit X does not match";
        }
        continue;
      }

      // check chi2
      kf::MeasurementXy<fvec> mxy(hit.X(), hit.Y(), hit.dX2(), hit.dY2(), hit.dXY(), fvec::One(), fvec::One());

      const fvec C10            = fit.ExtrapolateLineDxy(z);
      const auto [chi2x, chi2u] = kf::TrackKalmanFilter<fvec>::GetChi2XChi2U(mxy, x, y, C00, C10, C11);

      // TODO: adjust the cut, cut on chi2x & chi2u separately
      if (!frWData.CurrentIteration()->GetTrackFromTripletsFlag()) {
        if (chi2x[0] > chi2Cut) {
          if constexpr (fDebugCollectHits) {
            LOG(info) << "   hit chi2X is too large";
          }
          continue;
        }
        if (chi2u[0] > chi2Cut) {
          if constexpr (fDebugCollectHits) {
            LOG(info) << "   hit chi2U is too large";
          }
          continue;
        }
      }
      if constexpr (fDebugCollectHits) {
        LOG(info) << "   hit passed all the checks";
      }
      collectedHits.push_back(ih);

    }  // loop over the hits in the area
  }
}  // namespace cbm::algo::ca
