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

XPU_EXPORT(FitTripletsOT1);
XPU_D void FitTripletsOT1::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().FitTripletsOT1(ctx); }

XPU_EXPORT(FitTripletsOT2);
XPU_D void FitTripletsOT2::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().FitTripletsOT2(ctx); }

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

XPU_D void GnnGpuGraphConstructor::FitTripletsOT1(FitTripletsOT1::context& ctx) const
{
  const int iGThread       = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  const int NMaxTripletHit = kNNOrder * kNNOrder;
  if (iGThread >= fIterationData[0].fNHits * NMaxTripletHit) return;

  const unsigned int iHitL         = xpu::floor(iGThread / NMaxTripletHit);
  const unsigned int nTripletsHitL = fNTriplets[iHitL];
  const unsigned int iTriplet      = iGThread % NMaxTripletHit;
  if (iTriplet >= nTripletsHitL) return;  // empty triplet entry

  const std::array<unsigned int, 3> triplet = {iHitL, fTriplets[iHitL][iTriplet][0], fTriplets[iHitL][iTriplet][1]};

  const int nStations        = 12;
  const float threshold_chi2 = 19.5;  // def - 19.5
  const float threshold_qp   = 5.0f;  // def - 5.0f

  // ----- start fixing here
  int start_hit = 0;  // for interation in wData.RecoHitIndices()

  ca::GpuFieldValue fldB0, fldB1, fldB2;
  ca::GpuFieldRegion fld;

  ca::GpuFieldValue fldB01, fldB11, fldB21;
  ca::GpuFieldRegion fld1;

  int nTracks_SIMD = 1;

  kf::TrackKalmanFilter<float> fit;  // fit parameters coresponding to the current track
  kf::TrackParamBase<float>& tr = fit.Tr();
  fit.SetParticleMass(fParams_const[fIteration].particleMass);
  fit.SetDoFitVelocity(true);

  // Track* t{nullptr}; //

  const ca::GpuStation* sta[nStations];
  for (int is = 0; is < nStations; ++is) {
    sta[is] = &fStations_const[is];
  };

  // Spatial-time position of a hit vs. station and track in the portion
  float x[constants::size::MaxNstations];                       // Hit position along the x-axis [cm]
  float y[constants::size::MaxNstations];                       // Hit position along the y-axis [cm]
  kf::MeasurementXy<float> mxy[constants::size::MaxNstations];  // Covariance matrix for x,y
  float z[constants::size::MaxNstations];                       // Hit position along the z-axis (precised) [cm]
  float time[constants::size::MaxNstations];                    // Hit time [ns]
  float dt2[constants::size::MaxNstations];                     // Hit time uncertainty [ns] squared

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
  bool w[constants::size::MaxNstations];
  bool w_time[constants::size::MaxNstations];  // !!!

  float y_temp;
  float x_temp;
  float fldZ0;
  float fldZ1;
  float fldZ2;
  float z_start;
  float z_end;

  ca::GpuFieldValue fB[constants::size::MaxNstations], fB_temp;

  float ZSta[constants::size::MaxNstations];
  for (int ista = 0; ista < nStations; ista++) {
    ZSta[ista] = sta[ista]->fZ;
    mxy[ista].SetCov(1., 0., 1.);
  }

  int primaryCount = 0;
  int allCount     = 0;

  unsigned int N_vTracks = 1;

  // OT added
  float isPrimary = 0;  // set using fitPV info

  // t[i] = &tripletCandidates[itrack + i];

  // get hits of current track
  for (int ista = 0; ista < nStations; ista++) {
    w[ista]      = false;
    w_time[ista] = false;
    z[ista]      = ZSta[ista];
  }

  //fmask isFieldPresent = fmask::Zero();

  int nHitsTrack = 3;  // triplet
  int iSta[constants::size::MaxNstations];

  for (int ih = 0; ih < nHitsTrack; ih++) {

    const ca::Hit& hit           = fvHits[triplet[ih]];
    const int ista               = hit.Station();
    auto [detSystemId, iStLocal] = fParameters.GetActiveSetup().GetIndexMap().GlobalToLocal<EDetectorID>(ista);

    //if (sta[ista].fieldStatus) { isFieldPresent= true; }

    iSta[ih] = ista;
    w[ista]  = true;
    if (sta[ista]->timeInfo) {
      w_time[ista] = true;
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

    x[ista]    = hit.X();  //x_temp;
    y[ista]    = hit.Y();  //y_temp;
    time[ista] = hit.T();
    dt2[ista]  = dT2Orig;
    if (!sta[ista]->timeInfo) {
      dt2[ista] = 1.e4;
    }
    z[ista]          = hit.Z();
    fB_temp          = sta[ista]->fieldSlice.GetFieldValue(x[ista], y[ista]);
    mxy[ista].X()    = hit.X();
    mxy[ista].Y()    = hit.Y();
    mxy[ista].Dx2()  = dX2Orig;
    mxy[ista].Dy2()  = dY2Orig;
    mxy[ista].Dxy()  = dXYOrig;
    mxy[ista].NdfX() = 1.;
    mxy[ista].NdfY() = 1.;

    fB[ista].x = fB_temp.x;
    fB[ista].y = fB_temp.y;
    fB[ista].z = fB_temp.z;

    if (ih == 0) {
      z_start          = z[ista];
      x_first          = x[ista];
      y_first          = y[ista];
      time_first       = time[ista];
      wtime_first      = sta[ista]->timeInfo ? 1. : 0.;
      dt2_first        = dt2[ista];
      mxy_first.X()    = mxy[ista].X();
      mxy_first.Y()    = mxy[ista].Y();
      mxy_first.Dx2()  = mxy[ista].Dx2();
      mxy_first.Dy2()  = mxy[ista].Dy2();
      mxy_first.Dxy()  = mxy[ista].Dxy();
      mxy_first.NdfX() = mxy[ista].NdfX();
      mxy_first.NdfY() = mxy[ista].NdfY();
    }
    else if (ih == nHitsTrack - 1) {
      z_end           = z[ista];
      x_last          = x[ista];
      y_last          = y[ista];
      mxy_last.X()    = mxy[ista].X();
      mxy_last.Y()    = mxy[ista].Y();
      mxy_last.Dx2()  = mxy[ista].Dx2();
      mxy_last.Dy2()  = mxy[ista].Dy2();
      mxy_last.Dxy()  = mxy[ista].Dxy();
      mxy_last.NdfX() = mxy[ista].NdfX();
      mxy_last.NdfY() = mxy[ista].NdfY();
      time_last       = time[ista];
      dt2_last        = dt2[ista];
      wtime_last      = sta[ista]->timeInfo ? 1. : 0.;
    }
  }

  for (int ih = nHitsTrack - 1; ih >= 0; ih--) {
    const int ista = iSta[ih];
    By[ista]       = sta[ista]->fieldSlice.GetFieldValue(0., 0.).y;
  }

  fit.GuessTrack(z_end, x, y, z, time, By, w, w_time, nStations);

  tr.Qp() = float(1. / 1.1);

  for (int iter = 0; iter < 2; iter++) {  // 1.5 iterations

    fit.SetQp0(tr.Qp());

    // fit backward

    int ista = nStations - 1;

    time_last = w_time[ista] ? time_last : 0;
    time_last = w_time[ista] ? dt2_last : 0;

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

    fldB1 = sta[ista]->fieldSlice.GetFieldValue(tr.X(), tr.Y());

    fldB1.Combine(fB[ista], w[ista]);

    fldZ2    = z[ista - 2];
    float dz = fldZ2 - fldZ1;
    fldB2    = sta[ista]->fieldSlice.GetFieldValue(tr.X() + tr.Tx() * dz, tr.Y() + tr.Ty() * dz);
    fldB2.Combine(fB[ista - 2], w[ista - 2]);
    fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

    for (--ista; ista >= 0; ista--) {

      fldZ0 = z[ista];
      dz    = (fldZ1 - fldZ0);
      fldB0 = sta[ista]->fieldSlice.GetFieldValue(tr.X() - tr.Tx() * dz, tr.Y() - tr.Ty() * dz);
      fldB0.Combine(fB[ista], w[ista]);

      fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

      bool initialised = (z[ista] < z_end) & (z_start <= z[ista]);

      fld1 = fld;

      fit.SetMask(initialised);
      fit.Extrapolate(z[ista], fld1);
      int bin       = fMaterialMap[ista].GetBin(tr.X(), tr.Y());
      auto radThick = fMaterialMapTables[bin];
      // auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);

      fit.SetMask(initialised && w[ista]);
      fit.FilterXY(mxy[ista]);
      fit.FilterTime(time[ista], dt2[ista], sta[ista]->timeInfo);


      fldB2 = fldB1;
      fldZ2 = fldZ1;
      fldB1 = fldB0;
      fldZ1 = fldZ0;
    }

    // extrapolate to the PV region

    kf::TrackKalmanFilter fitpv = fit;
    {
      fitpv.SetMask(true);

      kf::MeasurementXy<float> vtxInfo = fParams_const[fIteration].targetMeasurement;
      vtxInfo.SetDx2(1.e-8);
      vtxInfo.SetDxy(0.);
      vtxInfo.SetDy2(1.e-8);

      kf::FieldRegion<float> fldFull(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
      fitpv.SetMaxExtrapolationStep(1.);
      for (int vtxIter = 0; vtxIter < 2; vtxIter++) {
        fitpv.SetQp0(fitpv.Tr().Qp());
        fitpv.Tr()      = fit.Tr();
        fitpv.Tr().Qp() = fitpv.Qp0();
        fitpv.Extrapolate(fParams_const[fIteration].GetTargetPositionZ(), fldFull);
        fitpv.FilterXY(vtxInfo);
      }
    }

    // OT : Use fitPV to determine if primary track
    if (iter == 1 and fIteration == 1) {  // use iter 1 of KF fit for better fit, for all primary iteration
      const auto pv_x = fitpv.Tr().X();   // in cm
      const auto pv_y = fitpv.Tr().Y();
      const auto pv_z = fitpv.Tr().Z();
      for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
        allCount++;
        if (std::isnan(pv_x) || std::isnan(pv_y) || std::isnan(pv_z) || (std::abs(pv_z + 44.0f) > 0.1)) {
          isPrimary = 1.0;  // just leave alone. Some of these are apparently useful
          continue;
        }
        const float dist = std::sqrt(pv_x * pv_x + pv_y * pv_y);
        if (dist > 1.0f) continue;  // 1 cm radius
        isPrimary = 1.0;
        primaryCount++;
        // LOG(info) << "z=" << pv_z<< ", x=" << pv_x<< ", y=" << pv_y;
      }
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
    fldB1 = sta[ista]->fieldSlice.GetFieldValue(tr.X(), tr.Y());
    fldB1.Combine(fB[ista], w[ista]);


    fldZ2 = z[ista + 2];
    dz    = fldZ2 - fldZ1;
    fldB2 = sta[ista]->fieldSlice.GetFieldValue(tr.X() + tr.Tx() * dz, tr.Y() + tr.Ty() * dz);
    fldB2.Combine(fB[ista + 2], w[ista + 2]);

    fld.Set(fldB2, fldZ2, fldB1, fldZ1, fldB0, fldZ0);

    for (++ista; ista < nStations; ista++) {
      fldZ0 = z[ista];
      dz    = (fldZ1 - fldZ0);
      fldB0 = sta[ista]->fieldSlice.GetFieldValue(tr.X() - tr.Tx() * dz, tr.Y() - tr.Ty() * dz);
      fldB0.Combine(fB[ista], w[ista]);
      fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

      bool initialised = (z[ista] <= z_end) & (z_start < z[ista]);

      fit.SetMask(initialised);
      fit.Extrapolate(z[ista], fld);
      int bin       = fMaterialMap[ista].GetBin(tr.X(), tr.Y());
      auto radThick = fMaterialMapTables[bin];
      // auto radThick = fSetup.GetMaterial(ista).GetThicknessX0(tr.X(), tr.Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, kf::FitDirection::kDownstream);
      fit.SetMask(initialised && w[ista]);
      fit.FilterXY(mxy[ista]);
      fit.FilterTime(time[ista], dt2[ista], sta[ista]->timeInfo);

      fldB2 = fldB1;
      fldZ2 = fldZ1;
      fldB1 = fldB0;
      fldZ1 = fldZ0;
    }
  }  // iter 1.5

  /// if track chi2 per dof is larger than threshold. Also kill negative and non-finite values
  /// if track p low than threshold_qp, then kill the track
  /// then remove triplet from list
  float chi2     = fit.Tr().GetChiSq();
  bool killTrack = !xpu::isfinite(chi2) || (chi2 < 0) || (chi2 > threshold_chi2);

  // momentum cut to reduce ghosts
  if (abs(fit.Tr().Qp()) > threshold_qp) {
    killTrack = true;
  }

  /// check isPrimary
  if (fIteration == 1) {
    if (isPrimary != 1.0) killTrack = true;  // not primary track
  }

  if (!killTrack) {
    const float qp  = fit.Tr().Qp();
    const float Cqp = fit.Tr().C44() + 0.001;  // 0.001 magic number added. (see triplet constructor)
    const float Tx  = fit.Tr().Tx();
    const float C22 = fit.Tr().C22();
    const float Ty  = fit.Tr().Ty();
    const float C33 = fit.Tr().C33();
    std::array<float, 7> tripletParams{chi2, qp, Cqp, Tx, C22, Ty, C33};

    // selectedTripletIndexes[iGThread] = true;
    // selectedTripletParams[iGThread]  = tripletParams;
  }
}


XPU_D void GnnGpuGraphConstructor::FitTripletsOT2(FitTripletsOT2::context& ctx) const
{
  const int iGThread       = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  const int NMaxTripletHit = kNNOrder * kNNOrder;
  if (iGThread >= fIterationData[0].fNHits * NMaxTripletHit) return;

  const unsigned int iHitL         = xpu::floor(iGThread / NMaxTripletHit);
  const unsigned int nTripletsHitL = fNTriplets[iHitL];
  const unsigned int iTriplet      = iGThread % NMaxTripletHit;
  if (iTriplet >= nTripletsHitL) return;  // empty triplet entry

  const std::array<unsigned int, 3> triplet = {iHitL, fTriplets[iHitL][iTriplet][0], fTriplets[iHitL][iTriplet][1]};

  const int nStations        = 12;
  const float threshold_chi2 = 19.5;  // def - 19.5
  const float threshold_qp   = 5.0f;  // def - 5.0f

  int ista[3] = {-1, -1, -1};
  ista[0] = fvHits[iHitL].Station();
  ista[1] = ista[0] + 1;
  ista[2] = ista[1] + 1;

  //  ca::GpuStation sta[3];
  const ca::GpuStation* sta[3];
  for (int is = 0; is < 3; ++is) {
    sta[is] = &fStations_const[ista[is]];
  };

  bool isMomentumFitted = ((sta[0]->fieldStatus != 0) || (sta[1]->fieldStatus != 0) || (sta[2]->fieldStatus != 0));

  float ndfTrackModel = 4;                    // straight line
  ndfTrackModel += isMomentumFitted ? 1 : 0;  // track with momentum

  const float maxQp = fParams[fIterationData[0].fIteration].maxQp;

  float x[3], y[3], z[3], t[3];
  float dt2[3];
  ca::MeasurementXy<float> mxy[3];

  for (int ih = 0; ih < 3; ++ih) {
    const ca::Hit& hit = fvHits[triplet[ih]];
    mxy[ih]            = ca::MeasurementXy<float>(hit.X(), hit.Y(), hit.dX2(), hit.dY2(), hit.dXY(), 1.f, 1.f);
    x[ih]   = hit.X();
    y[ih]   = hit.Y();
    z[ih]   = hit.Z();
    t[ih]   = hit.T();
    dt2[ih] = hit.dT2();
  };

  // find the field along the track
  ca::GpuFieldValue B[3];
  ca::GpuFieldRegion fld;
  ca::GpuFieldRegion fldTarget;

  float tx[3] = {(x[1] - x[0]) / (z[1] - z[0]), (x[2] - x[0]) / (z[2] - z[0]), (x[2] - x[1]) / (z[2] - z[1])};
  float ty[3] = {(y[1] - y[0]) / (z[1] - z[0]), (y[2] - y[0]) / (z[2] - z[0]), (y[2] - y[1]) / (z[2] - z[1])};
  for (int ih = 0; ih < 3; ++ih) {
    float dz = (sta[ih]->fZ - z[ih]);
    B[ih]    = sta[ih]->fieldSlice.GetFieldValue(x[ih] + tx[ih] * dz, y[ih] + ty[ih] * dz);
  };

  fld.Set(B[0], sta[0]->fZ, B[1], sta[1]->fZ, B[2], sta[2]->fZ);
  fldTarget.Set(fParams[fIterationData[0].fIteration].targB, fParams[fIterationData[0].fIteration].GetTargetPositionZ(),
                B[0], sta[0]->fZ, B[1], sta[1]->fZ);

  kf::TrackParamBase<float>
    fit_params;  // = fvTrackParamsTriplets[iGThread];	//TODO: set all to 0 and recheck - done, no difference

  fit_params.ResetCovMatrix();

  // initial parameters
  {
    float dz01      = 1.f / (z[1] - z[0]);
    fit_params.Tx() = (x[1] - x[0]) * dz01;
    fit_params.Ty() = (y[1] - y[0]) * dz01;
    fit_params.Qp() = 0.f;
    fit_params.Vi() = 0.f;
  }

  bool not_fitted = false;
  float qp0       = 0;
  // repeat several times in order to improve the precision
  for (int iiter = 0; iiter < 2 /*nIterations*/; ++iiter) {
    // fit forward
    {
      qp0 = fit_params.GetQp();
      if (qp0 > maxQp) qp0 = maxQp;
      if (qp0 < -maxQp) qp0 = -maxQp;

      fit_params.Qp()   = 0.f;
      fit_params.Vi()   = 0.f;
      fit_params.X()    = x[0];
      fit_params.Y()    = y[0];
      fit_params.Z()    = z[0];
      fit_params.Time() = t[0];
      fit_params.ResetErrors(1.f, 1.f, 1.f, 1.f, 100., (sta[0]->timeInfo ? dt2[0] : 1.e6f), 1.e2f);
      fit_params.C(0, 0) = mxy[0].Dx2();
      fit_params.C(1, 0) = mxy[0].Dxy();
      fit_params.C(1, 1) = mxy[0].Dy2();

      fit_params.Ndf()     = -ndfTrackModel + 2;
      fit_params.NdfTime() = sta[0]->timeInfo ? 0 : -1;

      //  add the target constraint
      // FilterWithTargetAtLine(&fit_params, &fldTarget); // removed by OT

      for (int ih = 1; ih < 3; ++ih) {
        ExtrapolateStep(&fit_params, z[ih], qp0, &fld);

        //	Extrapolate(&fit_params, z[ih], 0.f, &fld);	//TODO: add qp0 instead of 0.f

        int bin       = fMaterialMap[ista[ih]].GetBin(fit_params.GetX(), fit_params.GetY());
        auto radThick = fMaterialMapTables[bin];

        MultipleScattering(&fit_params, radThick, qp0);  //TODO: why it was commented in original gpu code?

        float Tx    = fit_params.Tx();
        float Ty    = fit_params.Ty();
        float txtx  = Tx * Tx;
        float tyty  = Ty * Ty;
        float txtx1 = txtx + 1.f;
        float h     = txtx + tyty;
        float tt    = xpu::sqrt(txtx1 + tyty);
        float h2    = h * h;
        float qp0t  = qp0 * tt;
        float c1 = 0.0136f, c2 = c1 * 0.038f, c3 = c2 * 0.5f, c4 = -c3 * 0.5f, c5 = c3 * 0.333333f, c6 = -c3 * 0.25f;

        float s0 = (c1 + c2 * xpu::log(radThick) + c3 * h + h2 * (c4 + c5 * h + c6 * h2)) * qp0t;
        float a  = ((tt
                    + fParams[fIterationData[0].fIteration].particleMass
                        * fParams[fIterationData[0].fIteration].particleMass * qp0 * qp0t)
                   * radThick * s0 * s0);
        //        fit_params.C(2, 2) += 0.001;//txtx1 * a;	//TODO: check if it is needed. Switching on leads to the strong ghosts increase

        fit_params.C(3, 2) += Tx * Ty * a;
        fit_params.C(3, 3) += (1. + tyty) * a;

        EnergyLossCorrection(&fit_params, radThick, -1.f);
        FilterXY(&fit_params, mxy[ih]);
        FilterTime(&fit_params, t[ih], dt2[ih], sta[ih]->timeInfo);
      }
    }

    if (iiter == 1) break;

    // fit backward
    {
      qp0 = fit_params.GetQp();
      if (qp0 > maxQp) qp0 = maxQp;
      if (qp0 < -maxQp) qp0 = -maxQp;

      fit_params.X()    = x[2];
      fit_params.Y()    = y[2];
      fit_params.Z()    = z[2];
      fit_params.Time() = t[2];
      fit_params.Qp()   = 0.f;
      fit_params.Vi()   = 0.f;

      fit_params.ResetErrors(1.f, 1.f, 1.f, 1.f, 100., (sta[2]->timeInfo ? dt2[2] : 1.e6f), 1.e2f);

      fit_params.Ndf()     = -ndfTrackModel + 2;
      fit_params.NdfTime() = sta[2]->timeInfo ? 0 : -1;

      for (int ih = 1; ih >= 0; --ih) {
        ExtrapolateStep(&fit_params, z[ih], qp0, &fld);
        auto radThick = fMaterialMapTables[fMaterialMap[ista[ih]].GetBin(fit_params.GetX(), fit_params.GetY())];
        MultipleScattering(&fit_params, radThick, qp0);
        EnergyLossCorrection(&fit_params, radThick, 1.f);
        FilterXY(&fit_params, mxy[ih]);
        FilterTime(&fit_params, t[ih], dt2[ih], sta[ih]->timeInfo);
      }
    }
  }  // for iiter

  float chi2 = fit_params.GetChiSq() + fit_params.GetChiSqTime();

  if (chi2 > fParams[fIterationData[0].fIteration].tripletFinalChi2Cut || !xpu::isfinite(chi2) || chi2 < 0.f
      || not_fitted) {
    chi2 = -1.;
  }

  const float qp  = fit_params.GetQp();
  const float Cqp = fit_params.C(4, 4) + 0.001;
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
