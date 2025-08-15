/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnGpuGraphConstructor.cxx
/// \brief The class contains data and kernels for running GNN tracking on CPU and GPU using XPU libraries

#include "GnnGpuGraphConstructor.h"

#include <stdio.h>
// printf ("iGThread: %d ...", iGThread);

using namespace cbm::algo::ca;

// Export Constants
XPU_EXPORT(strGnnGpuGraphConstructor);

// Kernel Definitions
//XPU_EXPORT(TestFunc);
//XPU_D void TestFunc::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().TestFunc(ctx); }

XPU_EXPORT(EmbedHits);
XPU_D void EmbedHits::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().EmbedHits(ctx); }

XPU_EXPORT(NearestNeighbours);
XPU_D void NearestNeighbours::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().NearestNeighbours(ctx); }

XPU_EXPORT(MakeTripletsOT);
XPU_D void MakeTripletsOT::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().MakeTripletsOT(ctx); }

XPU_EXPORT(FitTripletsOT);
XPU_D void FitTripletsOT::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().FitTripletsOT(ctx); }

XPU_D void GnnGpuGraphConstructor::EmbedHits(EmbedHits::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fIterationData[0].fNHits) return;

  const auto& hitl = fvHits[iGThread];

  std::array<float, 3> input{hitl.X(), hitl.Y(), hitl.Z() + 44.0f};
  std::array<float, 6> result;
  EmbedSingleHit(input, result);

  fEmbedCoord[iGThread] = result;
}

XPU_D void GnnGpuGraphConstructor::NearestNeighbours(NearestNeighbours::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fIterationData[0].fNHits) return;

  const float margin = 2.0f;  // FastPrim
  auto& neighbours   = fDoublets[iGThread];
  int neighCount     = 0;
  float maxDist      = 0.0f;
  float maxDistIndex = 0;

  const auto& hitl = fvHits[iGThread];
  const float y_l  = hitl.Y();
  const float z_l  = hitl.Z() + 44.0f;
  const int iStaL  = hitl.Station();
  if (iStaL > 10) {
    fNNeighbours[iGThread] = neighCount;
    return;
  }
  const int iStaM = iStaL + 1;

  // Find closest hits (upto kNNOrder) which satisfy slope condition
  const ca::HitIndex_t iHitStart = fIndexFirstHitStation[iStaM];      // start index
  const ca::HitIndex_t iHitEnd   = fIndexFirstHitStation[iStaM + 1];  // end index
  for (std::size_t ihitm = iHitStart; ihitm < iHitEnd; ihitm++) {
    const auto& hitm = fvHits[ihitm];
    // margin
    const float y_m   = hitm.Y();
    const float z_m   = hitm.Z() + 44.0f;
    const float slope = (y_m - y_l) / (z_m - z_l);
    if (xpu::abs(y_l - slope * z_l) > margin) continue;

    // embedding distance
    const float dist = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[ihitm]);

    if (neighCount < kNNOrder) {
      neighbours[neighCount++] = ihitm;
      if (dist > maxDist) {
        maxDist      = dist;
        maxDistIndex = neighCount - 1;
      }
    }
    else if (dist < maxDist) {  // replace hit max distance
      neighbours[maxDistIndex] = ihitm;
      maxDist                  = 0.0f;
      for (int i = 0; i < kNNOrder; i++) {
        const float dist_re = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[neighbours[i]]);
        if (dist_re > maxDist) {
          maxDist      = dist_re;
          maxDistIndex = i;
        }
      }
    }
  }  // hits on iStaM

  fNNeighbours[iGThread] = neighCount;
}

XPU_D void GnnGpuGraphConstructor::MakeTripletsOT(MakeTripletsOT::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fIterationData[0].fNHits) return;

  int tripletCount = 0;

  const float YZCut = 0.1;  // (radians) def - 0.1 from distributions
  const float XZCut = 0.1;  // def - 0.1 from distributions

  auto& tripletsLHit       = fTriplets[iGThread];
  const auto& doubletsLHit = fDoublets[iGThread];
  const int nLHitDoublets  = fNNeighbours[iGThread];
  const auto& hitl         = fvHits[iGThread];
  const float x_l          = hitl.X();
  const float y_l          = hitl.Y();
  const float z_l          = hitl.Z();
  const int iStaL          = hitl.Station();
  if (iStaL > 9) {
    fNTriplets[iGThread] = tripletCount;
    return;
  }
  const int iStaM = iStaL + 1;

  const ca::HitIndex_t iHitStartM = fIndexFirstHitStation[iStaM];      // start index middle station
  const ca::HitIndex_t iHitEndM   = fIndexFirstHitStation[iStaM + 1];  // end index
  for (int iDoubletL = 0; iDoubletL < nLHitDoublets; iDoubletL++) {
    const unsigned int iHitM = doubletsLHit[iDoubletL];
    for (std::size_t iM = iHitStartM; iM < iHitEndM; iM++) {  // hits on next station
      if (iHitM == iM) {
        const auto& hitm = fvHits[iHitM];
        const float x_m  = hitm.X();
        const float y_m  = hitm.Y();
        const float z_m  = hitm.Z();

        const int nMHitDoublets = fNNeighbours[iHitM];
        for (int iDoubletM = 0; iDoubletM < nMHitDoublets; iDoubletM++) {
          const unsigned int iHitR = fDoublets[iHitM][iDoubletM];
          const auto& hitr         = fvHits[iHitR];
          const float x_r          = hitr.X();
          const float y_r          = hitr.Y();
          const float z_r          = hitr.Z();

          // Cuts on triplet angles. Limits come from distributions
          // YZ
          const float angle1YZ    = xpu::atan2(y_m - y_l, z_m - z_l);
          const float angle2YZ    = xpu::atan2(y_r - y_m, z_r - z_m);
          const float angleDiffYZ = angle1YZ - angle2YZ;
          if (angleDiffYZ < -YZCut || angleDiffYZ > YZCut) continue;

          // XZ
          const float angle1XZ    = xpu::atan2(x_m - x_l, z_m - z_l);
          const float angle2XZ    = xpu::atan2(x_r - x_m, z_r - z_m);
          const float angleDiffXZ = angle1XZ - angle2XZ;
          if (angleDiffXZ < -XZCut || angleDiffXZ > XZCut) continue;

          tripletsLHit[tripletCount++] = std::array<unsigned int, 2>{iHitM, iHitR};
        }
        break;  // only one match possible
      }
    }
  }
  fNTriplets[iGThread] = tripletCount;
}

XPU_D void GnnGpuGraphConstructor::FitTripletsOT(FitTripletsOT::context& ctx) const
{
  const int iGThread       = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  const int NMaxTripletHit = kNNOrder * kNNOrder;
  if (iGThread >= fIterationData[0].fNHits * NMaxTripletHit) return;

  const unsigned int iHitL         = xpu::floor(iGThread / NMaxTripletHit);
  const unsigned int nTripletsHitL = fNTriplets[iHitL];
  const unsigned int iTriplet      = iGThread % NMaxTripletHit;
  if (iTriplet >= nTripletsHitL) return;  // empty triplet entry

  const std::array<unsigned int, 3> triplet = {iHitL, fTriplets[iHitL][iTriplet][0], fTriplets[iHitL][iTriplet][1]};

  const int nStations = 12;
  const float threshold_chi2 = 19.5;  // def - 19.5
  const float threshold_qp   = 5.0f;  // def - 5.0f

  int start_hit = 0;  // for interation in wData.RecoHitIndices()

  kf::FieldValue<float> fldB0, fldB1, fldB2;
  kf::FieldRegion<float> fld;

  kf::FieldValue<float> fldB01, fldB11, fldB21;
  kf::FieldRegion<float> fld1;

  int nTracks_SIMD    = float::size();

  kf::TrackKalmanFilter<float> fit;  // fit parameters coresponding to the current track
  TrackParamV& tr = fit.Tr();
  fit.SetParticleMass(fDefaultMass);
  fit.SetDoFitVelocity(true);

  Track* t[float::size()]{nullptr};

  const ca::Station<float>* sta = fParameters.GetStations().begin();

  // Spatial-time position of a hit vs. station and track in the portion

  float x[constants::size::MaxNstations];                       // Hit position along the x-axis [cm]
  float y[constants::size::MaxNstations];                       // Hit position along the y-axis [cm]
  kf::MeasurementXy<float> mxy[constants::size::MaxNstations];  // Covariance matrix for x,y

  float z[constants::size::MaxNstations];  // Hit position along the z-axis (precised) [cm]

  float time[constants::size::MaxNstations];  // Hit time [ns]
  float dt2[constants::size::MaxNstations];   // Hit time uncertainty [ns] squared

  float x_first;
  float y_first;
  kf::MeasurementXy<float> mxy_first;

  float time_first;
  float wtime_first;
  float dt2_first;

  float x_last;
  float y_last;
  kf::MeasurementXy<float> mxy_last;

  float time_last;
  float wtime_last;
  float dt2_last;

  float By[constants::size::MaxNstations];
  fmask w[constants::size::MaxNstations];
  fmask w_time[constants::size::MaxNstations];  // !!!

  float y_temp;
  float x_temp;
  float fldZ0;
  float fldZ1;
  float fldZ2;
  float z_start;
  float z_end;

  kf::FieldValue<float> fB[constants::size::MaxNstations], fB_temp _fvecalignment;

  float ZSta[constants::size::MaxNstations];
  for (int ista = 0; ista < nStations; ista++) {
    ZSta[ista] = sta[ista].fZ;
    mxy[ista].SetCov(1., 0., 1.);
  }

  int primaryCount = 0;
  int allCount     = 0;

  unsigned int N_vTracks = tripletCandidates.size();  // number of tracks processed per one SSE register

  for (unsigned int itrack = 0; itrack < N_vTracks; itrack += float::size()) {

    // OT added
    float isPrimary = 0;  // set using fitPV info

    if (N_vTracks - itrack < static_cast<unsigned short>(float::size())) nTracks_SIMD = N_vTracks - itrack;

    for (int i = 0; i < nTracks_SIMD; i++) {
      t[i] = &tripletCandidates[itrack + i];
    }

    // fill the rest of the SIMD vectors with something reasonable
    for (uint i = nTracks_SIMD; i < float::size(); i++) {
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
        const ca::Station<float>& st = fParameters.GetStation(ista);
        By[ista][iVec]              = st.fieldSlice.GetFieldValue(0., 0.).GetBy()[0];
      }
    }

    fit.GuessTrack(z_end, x, y, z, time, By, w, w_time, nStations);

    if (ca::TrackingMode::kGlobal == fTrackingMode || ca::TrackingMode::kMcbm == fTrackingMode) {
      tr.Qp() = float(1. / 1.1);
    }

    // tr.Qp() = iif(isFieldPresent, tr.Qp(), float(1. / 0.25));

    for (int iter = 0; iter < 2; iter++) {  // 1.5 iterations

      fit.SetQp0(tr.Qp());

      // fit backward

      int ista = nStations - 1;

      time_last = iif(w_time[ista], time_last, float::Zero());
      dt2_last  = iif(w_time[ista], dt2_last, float(1.e6));

      tr.ResetErrors(mxy_last.Dx2(), mxy_last.Dy2(), 0.1, 0.1, 1.0, dt2_last, 1.e-2);
      tr.C10()  = mxy_last.Dxy();
      tr.X()    = mxy_last.X();
      tr.Y()    = mxy_last.Y();
      tr.Time() = time_last;
      tr.Vi()   = constants::phys::SpeedOfLightInv;
      tr.InitVelocityRange(0.5);
      tr.Ndf()     = float(-5.) + float(2.);
      tr.NdfTime() = float(-2.) + wtime_last;

      fldZ1 = z[ista];

      fldB1 = sta[ista].fieldSlice.GetFieldValue(tr.X(), tr.Y());

      fldB1.SetSimdEntries(fB[ista], w[ista]);

      fldZ2   = z[ista - 2];
      float dz = fldZ2 - fldZ1;
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

        kf::MeasurementXy<float> vtxInfo = wData.TargetMeasurement();
        vtxInfo.SetDx2(1.e-8);
        vtxInfo.SetDxy(0.);
        vtxInfo.SetDy2(1.e-8);

        if (ca::TrackingMode::kGlobal == fTrackingMode) {
          kf::FieldRegion<float> fldFull(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
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

      tr.Ndf()     = float(-5. + 2.);
      tr.NdfTime() = float(-2.) + wtime_first;

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
      float chi2 = fit.Tr().GetChiSq();
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
}


XPU_D float GnnGpuGraphConstructor::hitDistanceSq(std::array<float, 6>& a, std::array<float, 6>& b) const
{
  float result  = 0;
  const int dim = 6;
  for (std::size_t i = 0; i < dim; i++) {
    result += xpu::pow(a[i] - b[i], 2);
  }
  return result;
}

XPU_D void GnnGpuGraphConstructor::EmbedSingleHit(std::array<float, 3>& input, std::array<float, 6>& result) const
{
  std::array<float, 16> result1;
  affine<16, 3>(fEmbedParameters[0].embedWeights_0, input, fEmbedParameters[0].embedBias_0, result1);
  applyTanH(result1);

  std::array<float, 16> result2;
  affine<16, 16>(fEmbedParameters[0].embedWeights_1, result1, fEmbedParameters[0].embedBias_1, result2);
  applyTanH(result2);

  affine<6, 16>(fEmbedParameters[0].embedWeights_2, result2, fEmbedParameters[0].embedBias_2, result);
  applyTanH(result);
}

template<int Rows, int Cols>
XPU_D void GnnGpuGraphConstructor::affine(const std::array<std::array<float, Cols>, Rows>& weight,
                                          const std::array<float, Cols>& input, const std::array<float, Rows>& bias,
                                          std::array<float, Rows>& result) const
{
  for (int i = 0; i < Rows; i++) {
    float tmp        = 0.0f;
    const auto& wrow = weight[i];
    for (int k = 0; k < Cols; k++) {
      tmp += wrow[k] * input[k];
    }
    result[i] = tmp + bias[i];
  }
}

template<std::size_t N>
XPU_D void GnnGpuGraphConstructor::applyTanH(std::array<float, N>& vec) const
{
  for (auto& v : vec) {
    if (v > 20.0f) {
      v = 1.0f;
    }
    else {
      float twoexp = xpu::exp(2.0f * v);
      v            = (twoexp - 1.0f) / (twoexp + 1.0f);
    }
  }
}


///-------------------------- TESTING --------------------------
// XPU_D void GnnGpuGraphConstructor::affine_3to16(std::array<std::array<float, 3>, 16>& weight,
//                                                 std::array<float, 3>& input, std::array<float, 16>& bias,
//                                                 std::array<float, 16>& result) const
// {
//   const int rows = 16;
//   const int cols = 3;
//   for (int i = 0; i < rows; i++) {
//     float tmp                     = 0.0f;
//     std::array<float, cols>& temp = weight[i];
//     for (int k = 0; k < cols; k++) {
//       tmp += temp[k] * input[k];
//     }
//     result[i] = tmp + bias[i];
//   }
// }
