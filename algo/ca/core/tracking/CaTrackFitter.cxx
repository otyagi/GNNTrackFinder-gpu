/* Copyright (C) 2010-2020 Frankfurt Institute for Advanced Studies, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov, Igor Kulakov [committer], Valentina Akishina, Maksym Zyzak */

#include "CaTrackFitter.h"

#include "CaFramework.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"

#include <iostream>
#include <vector>


namespace cbm::algo::ca
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  TrackFitter::TrackFitter(const ca::Parameters<fvec>& pars, const fscal mass, const ca::TrackingMode& mode)
    : fParameters(pars)
    , fSetup(fParameters.GetActiveSetup())
    , fDefaultMass(mass)
    , fTrackingMode(mode)
  {
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  TrackFitter::~TrackFitter() {}

  // -------------------------------------------------------------------------------------------------------------------
  //
  void TrackFitter::FitCaTracks(const ca::InputData& input, WindowData& wData)
  {
    //  LOG(info) << " Start CA Track Fitter ";
    int start_hit = 0;  // for interation in wData.RecoHitIndices()

    //  static kf::FieldValue fldB0, fldB1, fldB2 _fvecalignment;
    //  static kf::FieldRegion fld _fvecalignment;
    kf::FieldValue<fvec> fldB0, fldB1, fldB2 _fvecalignment;
    kf::FieldRegion<fvec> fld _fvecalignment;


    kf::FieldValue<fvec> fldB01, fldB11, fldB21 _fvecalignment;
    kf::FieldRegion<fvec> fld1 _fvecalignment;

    const int nStations = fParameters.GetNstationsActive();
    int nTracks_SIMD    = fvec::size();

    kf::TrackKalmanFilter<fvec> fit;  // fit parameters coresponding to the current track
    TrackParamV& tr = fit.Tr();
    fit.SetParticleMass(fDefaultMass);
    fit.SetDoFitVelocity(true);

    Track* t[fvec::size()]{nullptr};

    const ca::Station<fvec>* sta = fParameters.GetStations().begin();

    // Spatial-time position of a hit vs. station and track in the portion

    fvec x[constants::size::MaxNstations];                       // Hit position along the x-axis [cm]
    fvec y[constants::size::MaxNstations];                       // Hit position along the y-axis [cm]
    kf::MeasurementXy<fvec> mxy[constants::size::MaxNstations];  // Covariance matrix for x,y

    fvec z[constants::size::MaxNstations];  // Hit position along the z-axis (precised) [cm]

    fvec time[constants::size::MaxNstations];  // Hit time [ns]
    fvec dt2[constants::size::MaxNstations];   // Hit time uncertainty [ns] squared

    fvec x_first;
    fvec y_first;
    kf::MeasurementXy<fvec> mxy_first;

    fvec time_first;
    fvec wtime_first;
    fvec dt2_first;

    fvec x_last;
    fvec y_last;
    kf::MeasurementXy<fvec> mxy_last;

    fvec time_last;
    fvec wtime_last;
    fvec dt2_last;

    fvec By[constants::size::MaxNstations];
    fmask w[constants::size::MaxNstations];
    fmask w_time[constants::size::MaxNstations];  // !!!

    fvec y_temp;
    fvec x_temp;
    fvec fldZ0;
    fvec fldZ1;
    fvec fldZ2;
    fvec z_start;
    fvec z_end;

    kf::FieldValue<fvec> fB[constants::size::MaxNstations], fB_temp _fvecalignment;


    fvec ZSta[constants::size::MaxNstations];
    for (int ista = 0; ista < nStations; ista++) {
      ZSta[ista] = sta[ista].fZ;
      mxy[ista].SetCov(1., 0., 1.);
    }

    unsigned short N_vTracks = wData.RecoTracks().size();  // number of tracks processed per one SSE register

    for (unsigned short itrack = 0; itrack < N_vTracks; itrack += fvec::size()) {

      if (N_vTracks - itrack < static_cast<unsigned short>(fvec::size())) nTracks_SIMD = N_vTracks - itrack;

      for (int i = 0; i < nTracks_SIMD; i++) {
        t[i] = &wData.RecoTrack(itrack + i);
      }

      // fill the rest of the SIMD vectors with something reasonable
      for (uint i = nTracks_SIMD; i < fvec::size(); i++) {
        t[i] = &wData.RecoTrack(itrack);
      }

      // get hits of current track
      for (int ista = 0; ista < nStations; ista++) {
        w[ista]      = fmask::Zero();
        w_time[ista] = fmask::Zero();
        z[ista]      = ZSta[ista];
      }

      //fmask isFieldPresent = fmask::Zero();

      for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {

        int nHitsTrack = t[iVec]->fNofHits;
        int iSta[constants::size::MaxNstations];

        for (int ih = 0; ih < nHitsTrack; ih++) {

          const ca::Hit& hit           = input.GetHit(wData.RecoHitIndex(start_hit++));
          const int ista               = hit.Station();
          auto [detSystemId, iStLocal] = fParameters.GetActiveSetup().GetIndexMap().GlobalToLocal<EDetectorID>(ista);

          //if (sta[ista].fieldStatus) { isFieldPresent[iVec] = true; }

          iSta[ih]      = ista;
          w[ista][iVec] = true;
          if (sta[ista].timeInfo) {
            w_time[ista][iVec] = true;
          }
          // subtract misalignment tolerances to get the original hit errors
          float dX2Orig = hit.dX2() - fParameters.GetMisalignmentXsq(detSystemId);
          float dY2Orig = hit.dY2() - fParameters.GetMisalignmentYsq(detSystemId);
          float dXYOrig = hit.dXY();
          if (dX2Orig < 0. || dY2Orig < 0. || fabs(dXYOrig / sqrt(dX2Orig * dY2Orig)) > 1.) {
            dX2Orig = hit.dX2();
            dY2Orig = hit.dY2();
          }
          float dT2Orig = hit.dT2() - fParameters.GetMisalignmentTsq(detSystemId);
          if (dT2Orig < 0.) {
            dT2Orig = hit.dT2();
          }

          x[ista][iVec]    = hit.X();  //x_temp[iVec];
          y[ista][iVec]    = hit.Y();  //y_temp[iVec];
          time[ista][iVec] = hit.T();
          dt2[ista][iVec]  = dT2Orig;
          if (!sta[ista].timeInfo) {
            dt2[ista][iVec] = 1.e4;
          }
          z[ista][iVec]          = hit.Z();
          fB_temp                = sta[ista].fieldSlice.GetFieldValue(x[ista], y[ista]);
          mxy[ista].X()[iVec]    = hit.X();
          mxy[ista].Y()[iVec]    = hit.Y();
          mxy[ista].Dx2()[iVec]  = dX2Orig;
          mxy[ista].Dy2()[iVec]  = dY2Orig;
          mxy[ista].Dxy()[iVec]  = dXYOrig;
          mxy[ista].NdfX()[iVec] = 1.;
          mxy[ista].NdfY()[iVec] = 1.;

          fB[ista].SetSimdEntry(fB_temp.GetBx()[iVec], fB_temp.GetBy()[iVec], fB_temp.GetBz()[iVec], iVec);

          if (ih == 0) {
            z_start[iVec]          = z[ista][iVec];
            x_first[iVec]          = x[ista][iVec];
            y_first[iVec]          = y[ista][iVec];
            time_first[iVec]       = time[ista][iVec];
            wtime_first[iVec]      = sta[ista].timeInfo ? 1. : 0.;
            dt2_first[iVec]        = dt2[ista][iVec];
            mxy_first.X()[iVec]    = mxy[ista].X()[iVec];
            mxy_first.Y()[iVec]    = mxy[ista].Y()[iVec];
            mxy_first.Dx2()[iVec]  = mxy[ista].Dx2()[iVec];
            mxy_first.Dy2()[iVec]  = mxy[ista].Dy2()[iVec];
            mxy_first.Dxy()[iVec]  = mxy[ista].Dxy()[iVec];
            mxy_first.NdfX()[iVec] = mxy[ista].NdfX()[iVec];
            mxy_first.NdfY()[iVec] = mxy[ista].NdfY()[iVec];
          }
          else if (ih == nHitsTrack - 1) {
            z_end[iVec]           = z[ista][iVec];
            x_last[iVec]          = x[ista][iVec];
            y_last[iVec]          = y[ista][iVec];
            mxy_last.X()[iVec]    = mxy[ista].X()[iVec];
            mxy_last.Y()[iVec]    = mxy[ista].Y()[iVec];
            mxy_last.Dx2()[iVec]  = mxy[ista].Dx2()[iVec];
            mxy_last.Dy2()[iVec]  = mxy[ista].Dy2()[iVec];
            mxy_last.Dxy()[iVec]  = mxy[ista].Dxy()[iVec];
            mxy_last.NdfX()[iVec] = mxy[ista].NdfX()[iVec];
            mxy_last.NdfY()[iVec] = mxy[ista].NdfY()[iVec];
            time_last[iVec]       = time[ista][iVec];
            dt2_last[iVec]        = dt2[ista][iVec];
            wtime_last[iVec]      = sta[ista].timeInfo ? 1. : 0.;
          }
        }

        for (int ih = nHitsTrack - 1; ih >= 0; ih--) {
          const int ista              = iSta[ih];
          const ca::Station<fvec>& st = fParameters.GetStation(ista);
          By[ista][iVec]              = st.fieldSlice.GetFieldValue(0., 0.).GetBy()[0];
        }
      }

      fit.GuessTrack(z_end, x, y, z, time, By, w, w_time, nStations);

      if (ca::TrackingMode::kGlobal == fTrackingMode || ca::TrackingMode::kMcbm == fTrackingMode) {
        tr.Qp() = fvec(1. / 1.1);
      }

      // tr.Qp() = iif(isFieldPresent, tr.Qp(), fvec(1. / 0.25));

      for (int iter = 0; iter < 2; iter++) {  // 1.5 iterations

        fit.SetQp0(tr.Qp());

        // fit backward

        int ista = nStations - 1;

        time_last = iif(w_time[ista], time_last, fvec::Zero());
        dt2_last  = iif(w_time[ista], dt2_last, fvec(1.e6));

        tr.ResetErrors(mxy_last.Dx2(), mxy_last.Dy2(), 0.1, 0.1, 1.0, dt2_last, 1.e-2);
        tr.C10()  = mxy_last.Dxy();
        tr.X()    = mxy_last.X();
        tr.Y()    = mxy_last.Y();
        tr.Time() = time_last;
        tr.Vi()   = constants::phys::SpeedOfLightInv;
        tr.InitVelocityRange(0.5);
        tr.Ndf()     = fvec(-5.) + fvec(2.);
        tr.NdfTime() = fvec(-2.) + wtime_last;

        fldZ1 = z[ista];

        fldB1 = sta[ista].fieldSlice.GetFieldValue(tr.X(), tr.Y());

        fldB1.SetSimdEntries(fB[ista], w[ista]);

        fldZ2   = z[ista - 2];
        fvec dz = fldZ2 - fldZ1;
        fldB2   = sta[ista].fieldSlice.GetFieldValue(tr.X() + tr.Tx() * dz, tr.Y() + tr.Ty() * dz);
        fldB2.SetSimdEntries(fB[ista - 2], w[ista - 2]);
        fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

        for (--ista; ista >= 0; ista--) {

          fldZ0 = z[ista];
          dz    = (fldZ1 - fldZ0);
          fldB0 = sta[ista].fieldSlice.GetFieldValue(tr.X() - tr.Tx() * dz, tr.Y() - tr.Ty() * dz);
          fldB0.SetSimdEntries(fB[ista], w[ista]);
          fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

          fmask initialised = (z[ista] < z_end) & (z_start <= z[ista]);

          fld1 = fld;

          fit.SetMask(initialised);
          fit.Extrapolate(z[ista], fld1);
          auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
          fit.MultipleScattering(radThick);
          fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);

          fit.SetMask(initialised && w[ista]);
          fit.FilterXY(mxy[ista]);
          fit.FilterTime(time[ista], dt2[ista], fmask(sta[ista].timeInfo));


          fldB2 = fldB1;
          fldZ2 = fldZ1;
          fldB1 = fldB0;
          fldZ1 = fldZ0;
        }

        // extrapolate to the PV region

        kf::TrackKalmanFilter fitpv = fit;
        {
          fitpv.SetMask(fmask::One());

          kf::MeasurementXy<fvec> vtxInfo = wData.TargetMeasurement();
          vtxInfo.SetDx2(1.e-8);
          vtxInfo.SetDxy(0.);
          vtxInfo.SetDy2(1.e-8);

          if (ca::TrackingMode::kGlobal == fTrackingMode) {
            kf::FieldRegion<fvec> fldFull(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
            fitpv.SetMaxExtrapolationStep(1.);
            for (int vtxIter = 0; vtxIter < 2; vtxIter++) {
              fitpv.SetQp0(fitpv.Tr().Qp());
              fitpv.Tr()      = fit.Tr();
              fitpv.Tr().Qp() = fitpv.Qp0();
              fitpv.Extrapolate(fParameters.GetTargetPositionZ(), fldFull);
              fitpv.FilterXY(vtxInfo);
            }
          }
          else {
            fitpv.SetQp0(fitpv.Tr().Qp());
            fitpv.Extrapolate(fParameters.GetTargetPositionZ(), fld);
          }
        }

        //fit.SetQp0(tr.Qp());
        //fit.SetMask(fmask::One());
        //fit.MeasureVelocityWithQp();
        //fit.FilterVi(TrackParamV::kClightNsInv);

        TrackParamV Tf = fit.Tr();
        if (ca::TrackingMode::kGlobal == fTrackingMode) {
          Tf.Qp() = fitpv.Tr().Qp();
        }

        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          t[iVec]->fParFirst.Set(Tf, iVec);
        }

        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          t[iVec]->fParPV.Set(fitpv.Tr(), iVec);
        }

        if (iter == 1) {
          break;
        }  // only 1.5 iterations

        // fit forward

        ista = 0;

        tr.ResetErrors(mxy_first.Dx2(), mxy_first.Dy2(), 0.1, 0.1, 1., dt2_first, 1.e-2);
        tr.C10() = mxy_first.Dxy();

        tr.X()    = mxy_first.X();
        tr.Y()    = mxy_first.Y();
        tr.Time() = time_first;
        tr.Vi()   = constants::phys::SpeedOfLightInv;
        tr.InitVelocityRange(0.5);

        tr.Ndf()     = fvec(-5. + 2.);
        tr.NdfTime() = fvec(-2.) + wtime_first;

        fit.SetQp0(tr.Qp());

        fldZ1 = z[ista];
        fldB1 = sta[ista].fieldSlice.GetFieldValue(tr.X(), tr.Y());
        fldB1.SetSimdEntries(fB[ista], w[ista]);

        fldZ2 = z[ista + 2];
        dz    = fldZ2 - fldZ1;
        fldB2 = sta[ista].fieldSlice.GetFieldValue(tr.X() + tr.Tx() * dz, tr.Y() + tr.Ty() * dz);
        fldB2.SetSimdEntries(fB[ista + 2], w[ista + 2]);
        fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

        for (++ista; ista < nStations; ista++) {
          fldZ0 = z[ista];
          dz    = (fldZ1 - fldZ0);
          fldB0 = sta[ista].fieldSlice.GetFieldValue(tr.X() - tr.Tx() * dz, tr.Y() - tr.Ty() * dz);
          fldB0.SetSimdEntries(fB[ista], w[ista]);
          fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

          fmask initialised = (z[ista] <= z_end) & (z_start < z[ista]);

          fit.SetMask(initialised);
          fit.Extrapolate(z[ista], fld);
          auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
          fit.MultipleScattering(radThick);
          fit.EnergyLossCorrection(radThick, kf::FitDirection::kDownstream);
          fit.SetMask(initialised && w[ista]);
          fit.FilterXY(mxy[ista]);
          fit.FilterTime(time[ista], dt2[ista], fmask(sta[ista].timeInfo));

          fldB2 = fldB1;
          fldZ2 = fldZ1;
          fldB1 = fldB0;
          fldZ1 = fldZ0;
        }

        //fit.SetQp0(tr.Qp());
        //fit.SetMask(fmask::One());
        //fit.MeasureVelocityWithQp();
        //fit.FilterVi(TrackParamV::kClightNsInv);

        TrackParamV Tl = fit.Tr();
        if (ca::TrackingMode::kGlobal == fTrackingMode) {
          Tl.Qp() = fitpv.Tr().Qp();
        }

        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          t[iVec]->fParLast.Set(Tl, iVec);
        }

      }  // iter
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void TrackFitter::FitGNNTriplets(const ca::InputData& input, WindowData& wData, Vector<Track>& tripletCandidates,
                                   Vector<HitIndex_t>& tripletHits, Vector<int>& selectedTripletIndexes,
                                   Vector<float>& selectedTripletScores,
                                   std::vector<std::vector<float>>& selectedTripletParams, const int GNNiteration)
  {
    //  LOG(info) << " Start CA Track Fitter ";
    int start_hit = 0;  // for interation in wData.RecoHitIndices()

    //  static kf::FieldValue fldB0, fldB1, fldB2 _fvecalignment;
    //  static kf::FieldRegion fld _fvecalignment;
    kf::FieldValue<fvec> fldB0, fldB1, fldB2 _fvecalignment;
    kf::FieldRegion<fvec> fld _fvecalignment;


    kf::FieldValue<fvec> fldB01, fldB11, fldB21 _fvecalignment;
    kf::FieldRegion<fvec> fld1 _fvecalignment;

    const int nStations = fParameters.GetNstationsActive();
    int nTracks_SIMD    = fvec::size();

    kf::TrackKalmanFilter<fvec> fit;  // fit parameters coresponding to the current track
    TrackParamV& tr = fit.Tr();
    fit.SetParticleMass(fDefaultMass);
    fit.SetDoFitVelocity(true);

    Track* t[fvec::size()]{nullptr};

    const ca::Station<fvec>* sta = fParameters.GetStations().begin();

    // Spatial-time position of a hit vs. station and track in the portion

    fvec x[constants::size::MaxNstations];                       // Hit position along the x-axis [cm]
    fvec y[constants::size::MaxNstations];                       // Hit position along the y-axis [cm]
    kf::MeasurementXy<fvec> mxy[constants::size::MaxNstations];  // Covariance matrix for x,y

    fvec z[constants::size::MaxNstations];  // Hit position along the z-axis (precised) [cm]

    fvec time[constants::size::MaxNstations];  // Hit time [ns]
    fvec dt2[constants::size::MaxNstations];   // Hit time uncertainty [ns] squared

    fvec x_first;
    fvec y_first;
    kf::MeasurementXy<fvec> mxy_first;

    fvec time_first;
    fvec wtime_first;
    fvec dt2_first;

    fvec x_last;
    fvec y_last;
    kf::MeasurementXy<fvec> mxy_last;

    fvec time_last;
    fvec wtime_last;
    fvec dt2_last;

    fvec By[constants::size::MaxNstations];
    fmask w[constants::size::MaxNstations];
    fmask w_time[constants::size::MaxNstations];  // !!!

    fvec y_temp;
    fvec x_temp;
    fvec fldZ0;
    fvec fldZ1;
    fvec fldZ2;
    fvec z_start;
    fvec z_end;

    kf::FieldValue<fvec> fB[constants::size::MaxNstations], fB_temp _fvecalignment;


    fvec ZSta[constants::size::MaxNstations];
    for (int ista = 0; ista < nStations; ista++) {
      ZSta[ista] = sta[ista].fZ;
      mxy[ista].SetCov(1., 0., 1.);
    }

    int primaryCount = 0;
    int allCount     = 0;

    unsigned int N_vTracks = tripletCandidates.size();  // number of tracks processed per one SSE register

    for (unsigned int itrack = 0; itrack < N_vTracks; itrack += fvec::size()) {

      // OT added
      fvec isPrimary = fvec::Zero();  // set using fitPV info

      if (N_vTracks - itrack < static_cast<unsigned short>(fvec::size())) nTracks_SIMD = N_vTracks - itrack;

      for (int i = 0; i < nTracks_SIMD; i++) {
        t[i] = &tripletCandidates[itrack + i];
      }

      // fill the rest of the SIMD vectors with something reasonable
      for (uint i = nTracks_SIMD; i < fvec::size(); i++) {
        t[i] = &tripletCandidates[itrack];
      }

      // get hits of current track
      for (int ista = 0; ista < nStations; ista++) {
        w[ista]      = fmask::Zero();
        w_time[ista] = fmask::Zero();
        z[ista]      = ZSta[ista];
      }

      //fmask isFieldPresent = fmask::Zero();

      for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {

        int nHitsTrack = t[iVec]->fNofHits;
        int iSta[constants::size::MaxNstations];

        for (int ih = 0; ih < nHitsTrack; ih++) {

          const ca::Hit& hit           = input.GetHit(tripletHits[start_hit++]);
          const int ista               = hit.Station();
          auto [detSystemId, iStLocal] = fParameters.GetActiveSetup().GetIndexMap().GlobalToLocal<EDetectorID>(ista);

          //if (sta[ista].fieldStatus) { isFieldPresent[iVec] = true; }

          iSta[ih]      = ista;
          w[ista][iVec] = true;
          if (sta[ista].timeInfo) {
            w_time[ista][iVec] = true;
          }
          // subtract misalignment tolerances to get the original hit errors
          float dX2Orig = hit.dX2() - fParameters.GetMisalignmentXsq(detSystemId);
          float dY2Orig = hit.dY2() - fParameters.GetMisalignmentYsq(detSystemId);
          float dXYOrig = hit.dXY();
          if (dX2Orig < 0. || dY2Orig < 0. || fabs(dXYOrig / sqrt(dX2Orig * dY2Orig)) > 1.) {
            dX2Orig = hit.dX2();
            dY2Orig = hit.dY2();
          }
          float dT2Orig = hit.dT2() - fParameters.GetMisalignmentTsq(detSystemId);
          if (dT2Orig < 0.) {
            dT2Orig = hit.dT2();
          }

          x[ista][iVec]    = hit.X();  //x_temp[iVec];
          y[ista][iVec]    = hit.Y();  //y_temp[iVec];
          time[ista][iVec] = hit.T();
          dt2[ista][iVec]  = dT2Orig;
          if (!sta[ista].timeInfo) {
            dt2[ista][iVec] = 1.e4;
          }
          z[ista][iVec]          = hit.Z();
          fB_temp                = sta[ista].fieldSlice.GetFieldValue(x[ista], y[ista]);
          mxy[ista].X()[iVec]    = hit.X();
          mxy[ista].Y()[iVec]    = hit.Y();
          mxy[ista].Dx2()[iVec]  = dX2Orig;
          mxy[ista].Dy2()[iVec]  = dY2Orig;
          mxy[ista].Dxy()[iVec]  = dXYOrig;
          mxy[ista].NdfX()[iVec] = 1.;
          mxy[ista].NdfY()[iVec] = 1.;

          fB[ista].SetSimdEntry(fB_temp.GetBx()[iVec], fB_temp.GetBy()[iVec], fB_temp.GetBz()[iVec], iVec);

          if (ih == 0) {
            z_start[iVec]          = z[ista][iVec];
            x_first[iVec]          = x[ista][iVec];
            y_first[iVec]          = y[ista][iVec];
            time_first[iVec]       = time[ista][iVec];
            wtime_first[iVec]      = sta[ista].timeInfo ? 1. : 0.;
            dt2_first[iVec]        = dt2[ista][iVec];
            mxy_first.X()[iVec]    = mxy[ista].X()[iVec];
            mxy_first.Y()[iVec]    = mxy[ista].Y()[iVec];
            mxy_first.Dx2()[iVec]  = mxy[ista].Dx2()[iVec];
            mxy_first.Dy2()[iVec]  = mxy[ista].Dy2()[iVec];
            mxy_first.Dxy()[iVec]  = mxy[ista].Dxy()[iVec];
            mxy_first.NdfX()[iVec] = mxy[ista].NdfX()[iVec];
            mxy_first.NdfY()[iVec] = mxy[ista].NdfY()[iVec];
          }
          else if (ih == nHitsTrack - 1) {
            z_end[iVec]           = z[ista][iVec];
            x_last[iVec]          = x[ista][iVec];
            y_last[iVec]          = y[ista][iVec];
            mxy_last.X()[iVec]    = mxy[ista].X()[iVec];
            mxy_last.Y()[iVec]    = mxy[ista].Y()[iVec];
            mxy_last.Dx2()[iVec]  = mxy[ista].Dx2()[iVec];
            mxy_last.Dy2()[iVec]  = mxy[ista].Dy2()[iVec];
            mxy_last.Dxy()[iVec]  = mxy[ista].Dxy()[iVec];
            mxy_last.NdfX()[iVec] = mxy[ista].NdfX()[iVec];
            mxy_last.NdfY()[iVec] = mxy[ista].NdfY()[iVec];
            time_last[iVec]       = time[ista][iVec];
            dt2_last[iVec]        = dt2[ista][iVec];
            wtime_last[iVec]      = sta[ista].timeInfo ? 1. : 0.;
          }
        }

        for (int ih = nHitsTrack - 1; ih >= 0; ih--) {
          const int ista              = iSta[ih];
          const ca::Station<fvec>& st = fParameters.GetStation(ista);
          By[ista][iVec]              = st.fieldSlice.GetFieldValue(0., 0.).GetBy()[0];
        }
      }

      fit.GuessTrack(z_end, x, y, z, time, By, w, w_time, nStations);

      if (ca::TrackingMode::kGlobal == fTrackingMode || ca::TrackingMode::kMcbm == fTrackingMode) {
        tr.Qp() = fvec(1. / 1.1);
      }

      // tr.Qp() = iif(isFieldPresent, tr.Qp(), fvec(1. / 0.25));

      for (int iter = 0; iter < 2; iter++) {  // 1.5 iterations

        fit.SetQp0(tr.Qp());

        // fit backward

        int ista = nStations - 1;

        time_last = iif(w_time[ista], time_last, fvec::Zero());
        dt2_last  = iif(w_time[ista], dt2_last, fvec(1.e6));

        tr.ResetErrors(mxy_last.Dx2(), mxy_last.Dy2(), 0.1, 0.1, 1.0, dt2_last, 1.e-2);
        tr.C10()  = mxy_last.Dxy();
        tr.X()    = mxy_last.X();
        tr.Y()    = mxy_last.Y();
        tr.Time() = time_last;
        tr.Vi()   = constants::phys::SpeedOfLightInv;
        tr.InitVelocityRange(0.5);
        tr.Ndf()     = fvec(-5.) + fvec(2.);
        tr.NdfTime() = fvec(-2.) + wtime_last;

        fldZ1 = z[ista];

        fldB1 = sta[ista].fieldSlice.GetFieldValue(tr.X(), tr.Y());

        fldB1.SetSimdEntries(fB[ista], w[ista]);

        fldZ2   = z[ista - 2];
        fvec dz = fldZ2 - fldZ1;
        fldB2   = sta[ista].fieldSlice.GetFieldValue(tr.X() + tr.Tx() * dz, tr.Y() + tr.Ty() * dz);
        fldB2.SetSimdEntries(fB[ista - 2], w[ista - 2]);
        fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

        for (--ista; ista >= 0; ista--) {

          fldZ0 = z[ista];
          dz    = (fldZ1 - fldZ0);
          fldB0 = sta[ista].fieldSlice.GetFieldValue(tr.X() - tr.Tx() * dz, tr.Y() - tr.Ty() * dz);
          fldB0.SetSimdEntries(fB[ista], w[ista]);
          fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

          fmask initialised = (z[ista] < z_end) & (z_start <= z[ista]);

          fld1 = fld;

          fit.SetMask(initialised);
          fit.Extrapolate(z[ista], fld1);
          auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
          fit.MultipleScattering(radThick);
          fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);

          fit.SetMask(initialised && w[ista]);
          fit.FilterXY(mxy[ista]);
          fit.FilterTime(time[ista], dt2[ista], fmask(sta[ista].timeInfo));


          fldB2 = fldB1;
          fldZ2 = fldZ1;
          fldB1 = fldB0;
          fldZ1 = fldZ0;
        }

        // extrapolate to the PV region

        kf::TrackKalmanFilter fitpv = fit;
        {
          fitpv.SetMask(fmask::One());

          kf::MeasurementXy<fvec> vtxInfo = wData.TargetMeasurement();
          vtxInfo.SetDx2(1.e-8);
          vtxInfo.SetDxy(0.);
          vtxInfo.SetDy2(1.e-8);

          if (ca::TrackingMode::kGlobal == fTrackingMode) {
            kf::FieldRegion<fvec> fldFull(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
            fitpv.SetMaxExtrapolationStep(1.);
            for (int vtxIter = 0; vtxIter < 2; vtxIter++) {
              fitpv.SetQp0(fitpv.Tr().Qp());
              fitpv.Tr()      = fit.Tr();
              fitpv.Tr().Qp() = fitpv.Qp0();
              fitpv.Extrapolate(fParameters.GetTargetPositionZ(), fldFull);
              fitpv.FilterXY(vtxInfo);
            }
          }
          else {
            fitpv.SetQp0(fitpv.Tr().Qp());
            fitpv.Extrapolate(fParameters.GetTargetPositionZ(), fld);
          }
        }

        // OT : Use fitPV to determine if primary track
        if (iter == 1 and GNNiteration == 1) {  // use iter 1 of KF fit for better fit, for all primary iteration
          const auto pv_x = fitpv.Tr().X();     // in cm
          const auto pv_y = fitpv.Tr().Y();
          const auto pv_z = fitpv.Tr().Z();
          for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
            allCount++;
            if (std::isnan(pv_x[iVec]) || std::isnan(pv_y[iVec]) || std::isnan(pv_z[iVec])
                || (std::abs(pv_z[iVec] + 44.0f) > 0.1)) {
              isPrimary[iVec] = 1.0;  // just leave alone. Some of these are apparently useful
              continue;
            }
            const float dist = std::sqrt(pv_x[iVec] * pv_x[iVec] + pv_y[iVec] * pv_y[iVec]);
            if (dist > 1.0f) continue;  // 1 cm radius
            isPrimary[iVec] = 1.0;
            primaryCount++;
            // LOG(info) << "z=" << pv_z[iVec] << ", x=" << pv_x[iVec] << ", y=" << pv_y[iVec];
          }
        }

        //fit.SetQp0(tr.Qp());
        //fit.SetMask(fmask::One());
        //fit.MeasureVelocityWithQp();
        //fit.FilterVi(TrackParamV::kClightNsInv);

        TrackParamV Tf = fit.Tr();
        if (ca::TrackingMode::kGlobal == fTrackingMode) {
          Tf.Qp() = fitpv.Tr().Qp();
        }

        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          t[iVec]->fParFirst.Set(Tf, iVec);
        }

        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          t[iVec]->fParPV.Set(fitpv.Tr(), iVec);
        }

        if (iter == 1) {
          break;
        }  // only 1.5 iterations

        // fit forward

        ista = 0;

        tr.ResetErrors(mxy_first.Dx2(), mxy_first.Dy2(), 0.1, 0.1, 1., dt2_first, 1.e-2);
        tr.C10() = mxy_first.Dxy();

        tr.X()    = mxy_first.X();
        tr.Y()    = mxy_first.Y();
        tr.Time() = time_first;
        tr.Vi()   = constants::phys::SpeedOfLightInv;
        tr.InitVelocityRange(0.5);

        tr.Ndf()     = fvec(-5. + 2.);
        tr.NdfTime() = fvec(-2.) + wtime_first;

        fit.SetQp0(tr.Qp());

        fldZ1 = z[ista];
        fldB1 = sta[ista].fieldSlice.GetFieldValue(tr.X(), tr.Y());
        fldB1.SetSimdEntries(fB[ista], w[ista]);

        fldZ2 = z[ista + 2];
        dz    = fldZ2 - fldZ1;
        fldB2 = sta[ista].fieldSlice.GetFieldValue(tr.X() + tr.Tx() * dz, tr.Y() + tr.Ty() * dz);
        fldB2.SetSimdEntries(fB[ista + 2], w[ista + 2]);
        fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

        for (++ista; ista < nStations; ista++) {
          fldZ0 = z[ista];
          dz    = (fldZ1 - fldZ0);
          fldB0 = sta[ista].fieldSlice.GetFieldValue(tr.X() - tr.Tx() * dz, tr.Y() - tr.Ty() * dz);
          fldB0.SetSimdEntries(fB[ista], w[ista]);
          fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

          fmask initialised = (z[ista] <= z_end) & (z_start < z[ista]);

          fit.SetMask(initialised);
          fit.Extrapolate(z[ista], fld);
          auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
          fit.MultipleScattering(radThick);
          fit.EnergyLossCorrection(radThick, kf::FitDirection::kDownstream);
          fit.SetMask(initialised && w[ista]);
          fit.FilterXY(mxy[ista]);
          fit.FilterTime(time[ista], dt2[ista], fmask(sta[ista].timeInfo));

          fldB2 = fldB1;
          fldZ2 = fldZ1;
          fldB1 = fldB0;
          fldZ1 = fldZ0;
        }

        //fit.SetQp0(tr.Qp());
        //fit.SetMask(fmask::One());
        //fit.MeasureVelocityWithQp();
        //fit.FilterVi(TrackParamV::kClightNsInv);

        TrackParamV Tl = fit.Tr();
        if (ca::TrackingMode::kGlobal == fTrackingMode) {
          Tl.Qp() = fitpv.Tr().Qp();
        }

        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          t[iVec]->fParLast.Set(Tl, iVec);
        }

      }  // iter

      /// if track chi2 per dof is larger than threshold. Also kill negative and non-finite values
      /// if track p low than threshold_qp, then kill the track
      /// then remove triplet from list
      {
        // default/initial
        float threshold_chi2 = 50.0f;  // def - 50. (19.5 from analysis of sec pions)
        float threshold_qp   = 10.0f;  // def -

        // for Primary iteration
        if (GNNiteration == 0) {  // Fast primary
          threshold_chi2 = 19.5;  // def - 19.5
          threshold_qp   = 5.0f;  // def - 5.0f
        }
        else if (GNNiteration == 1) {  // All primary
          threshold_chi2 = 5.0f;       // def - 5
          threshold_qp   = 10.0f;      // def - 10
        }
        else if (GNNiteration == 3) {  // Secondary
          threshold_chi2 = 50.0f;      // def - 10.0f
          threshold_qp   = 10.0f;      // def - 10.0f
        }

        fvec chi2 = fit.Tr().GetChiSq();
        for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
          bool killTrack = !std::isfinite(chi2[iVec]) || (chi2[iVec] < 0) || (chi2[iVec] > threshold_chi2);

          // momentum cut to reduce ghosts
          if (abs(fit.Tr().Qp()[iVec]) > threshold_qp) {
            killTrack = true;
          }

          /// check isPrimary
          if (GNNiteration == 1) {
            if (isPrimary[iVec] != 1.0) killTrack = true;  // not primary track
          }

          if (!killTrack) {
            // std::cout << "chi2 of track length " << trackCandidates[itrack + iVec].fNofHits << " is " << chi2[iVec]
            //           << '\n';
            selectedTripletIndexes.push_back(itrack + iVec);
            selectedTripletScores.push_back(chi2[iVec]);

            const float qp  = fit.Tr().Qp()[iVec];
            const float Cqp = fit.Tr().C44()[iVec] + 0.001;  // 0.001 magic number added. (see triplet constructor)
            const float Tx  = fit.Tr().Tx()[iVec];
            const float C22 = fit.Tr().C22()[iVec];
            const float Ty  = fit.Tr().Ty()[iVec];
            const float C33 = fit.Tr().C33()[iVec];
            std::vector<float> tripletParams{chi2[iVec], qp, Cqp, Tx, C22, Ty, C33};
            selectedTripletParams.push_back(tripletParams);
          }
        }
      }
    }  // itrack/triplet
  }  // FitGNNTriplets
}  // namespace cbm::algo::ca
