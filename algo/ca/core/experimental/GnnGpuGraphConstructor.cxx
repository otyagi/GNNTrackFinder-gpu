/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnGpuGraphConstructor.cxx
/// \brief The class contains data and kernels for running GNN tracking on CPU and GPU using XPU libraries

#include "GnnGpuGraphConstructor.h"

#include <stdio.h>  // for debugging
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

XPU_EXPORT(ConstructCandidates);
XPU_D void ConstructCandidates::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().ConstructCandidates(ctx);
}

XPU_EXPORT(ExclusiveScan);
XPU_D void ExclusiveScan::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().ExclusiveScan(ctx); }

XPU_EXPORT(AddBlockSums);
XPU_D void AddBlockSums::operator()(context& ctx, int nBlocks)
{
  ctx.cmem<strGnnGpuGraphConstructor>().AddBlockSums(ctx, nBlocks);
}

XPU_EXPORT(AddOffsets);
XPU_D void AddOffsets::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().AddOffsets(ctx); }

XPU_EXPORT(CompressAllTripletsOrdered);
XPU_D void CompressAllTripletsOrdered::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().CompressAllTripletsOrdered(ctx);
}

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

  // printf ("iGThread: %d \t", iGThread);

  unsigned int tripletCount = 0;

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
    for (auto iM = iHitStartM; iM < iHitEndM; iM++) {  // hits on next station
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

  const unsigned int iHitL = iGThread / NMaxTripletHit;
  if (iHitL >= fIterationData[0].fNHits) return;
  const int lSta = fvHits[iHitL].Station();
  if (lSta > 9) return;
  const unsigned int nTripletsHitL = fNTriplets[iHitL];
  const int iTriplet               = iGThread % NMaxTripletHit;
  if (iTriplet >= NMaxTripletHit) return;
  if (nTripletsHitL > NMaxTripletHit || nTripletsHitL == 0) return;
  if (iTriplet >= (int) nTripletsHitL) return;

  const std::array<unsigned int, 3> triplet = {iHitL, fTriplets[iHitL][iTriplet][0], fTriplets[iHitL][iTriplet][1]};

  for (int i = 0; i < 3; i++) {
    if (triplet[i] >= fIterationData[0].fNHits) return;
  }
  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  const int nStations        = 12;
  const float threshold_chi2 = 19.5;  // def - 19.5
  const float threshold_qp   = 5.0f;  // def - 5.0f

  // ----- start fixing here
  ca::GpuFieldValue fldB0, fldB1, fldB2;
  ca::GpuFieldRegion fld, fld1;

  kf::GpuTrackKalmanFilter<float> fit;
  // kf::TrackParamBase<float>& tr = fit.Tr();
  TrackParam<float>& tr = fit.Tr();
  fit.SetParticleMass(constants::phys::MuonMass);
  fit.SetDoFitVelocity(true);

  const ca::GpuStation* sta[nStations];
  for (int is = 0; is < nStations; ++is) {
    sta[is] = &fStations_const[is];
  };

  // Spatial-time position of a hit vs. station and track in the portion
  float x[constants::size::MaxNstations];                       // Hit position along the x-axis [cm]
  float y[constants::size::MaxNstations];                       // Hit position along the y-axis [cm]
  ca::MeasurementXy<float> mxy[constants::size::MaxNstations];  // Covariance matrix for x,y
  float z[constants::size::MaxNstations];                       // Hit position along the z-axis (precised) [cm]
  float time[constants::size::MaxNstations];                    // Hit time [ns]
  float dt2[constants::size::MaxNstations];                     // Hit time uncertainty [ns] squared

  float x_first;
  float y_first;
  ca::MeasurementXy<float> mxy_first;
  float time_first;
  float wtime_first;
  float dt2_first;

  float x_last;
  float y_last;
  ca::MeasurementXy<float> mxy_last;
  float time_last;
  float wtime_last;
  float dt2_last;

  float By[constants::size::MaxNstations];
  bool w[constants::size::MaxNstations];
  bool w_time[constants::size::MaxNstations];  // !!!

  float fldZ0;
  float fldZ1;
  float fldZ2;
  float z_start;
  float z_end;

  ca::GpuFieldValue fB[constants::size::MaxNstations];

  float ZSta[constants::size::MaxNstations];
  for (int ista = 0; ista < nStations; ista++) {
    ZSta[ista] = sta[ista]->fZ;
    mxy[ista].SetCov(1., 0., 1.);
  }

  // OT added
  float isPrimary = 0;  // set using fitPV info

  // get hits of current track
  for (int ista = 0; ista < nStations; ista++) {
    w[ista]      = false;
    w_time[ista] = false;
    z[ista]      = ZSta[ista];
  }

  const int nHitsTrack = 3;  // triplet
  int iSta[constants::size::MaxNstations];

  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  for (int ih = 0; ih < nHitsTrack; ih++) {
    const ca::Hit& hit = fvHits[triplet[ih]];
    const int ista     = hit.Station();
    auto detSystemId   = sta[ista]->GetDetectorID();

    // printf("ih: %d, ista: %d \n", ih, ista);

    iSta[ih] = ista;
    w[ista]  = true;
    if (sta[ista]->timeInfo) {
      w_time[ista] = true;
    }
    // subtract misalignment tolerances to get the original hit errors
    float dX2Orig = hit.dX2();  //- fParams[fIteration].GetMisalignmentXsq(detSystemId);
    float dY2Orig = hit.dY2();  // - fParams[fIteration].GetMisalignmentYsq(detSystemId);
    float dXYOrig = hit.dXY();
    // if (dX2Orig < 0. || dY2Orig < 0. || xpu::abs(dXYOrig / xpu::sqrt(dX2Orig * dY2Orig)) > 1.) {
    //   dX2Orig = hit.dX2();
    //   dY2Orig = hit.dY2();
    // }
    float dT2Orig = hit.dT2();  // - fParams[fIteration].GetMisalignmentTsq(detSystemId);
    // if (dT2Orig < 0.) {
    //   dT2Orig = hit.dT2();
    // }

    // printf("ih: %d, ista: %d \n", ih, ista);

    /// crashes on all
    // printf("mis X %f \n", fParams[fIteration].GetMisalignmentXsq(detSystemId));
    // printf("mis X %f \n", fParams[fIteration].GetMisalignmentYsq(detSystemId));
    // printf("mis X %f \n", fParams[fIteration].GetMisalignmentTsq(detSystemId));

    x[ista]    = hit.X();  //x_temp;
    y[ista]    = hit.Y();  //y_temp;
    time[ista] = hit.T();
    dt2[ista]  = dT2Orig;
    if (!sta[ista]->timeInfo) {
      dt2[ista] = 1.e4;
    }
    z[ista]          = hit.Z();
    fB[ista]         = sta[ista]->fieldSlice.GetFieldValue(x[ista], y[ista]);
    mxy[ista].X()    = hit.X();
    mxy[ista].Y()    = hit.Y();
    mxy[ista].Dx2()  = dX2Orig;
    mxy[ista].Dy2()  = dY2Orig;
    mxy[ista].Dxy()  = dXYOrig;
    mxy[ista].NdfX() = 1.;
    mxy[ista].NdfY() = 1.;
  }  // ih

  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  {
    const ca::Hit& f_hit = fvHits[triplet[0]];
    const int f_ista     = f_hit.Station();
    z_start              = z[f_ista];
    x_first              = x[f_ista];
    y_first              = y[f_ista];
    time_first           = time[f_ista];
    wtime_first          = sta[f_ista]->timeInfo ? 1. : 0.;
    dt2_first            = dt2[f_ista];
    mxy_first.X()        = mxy[f_ista].X();
    mxy_first.Y()        = mxy[f_ista].Y();
    mxy_first.Dx2()      = mxy[f_ista].Dx2();
    mxy_first.Dy2()      = mxy[f_ista].Dy2();
    mxy_first.Dxy()      = mxy[f_ista].Dxy();
    mxy_first.NdfX()     = mxy[f_ista].NdfX();
    mxy_first.NdfY()     = mxy[f_ista].NdfY();
  }

  {
    const ca::Hit& l_hit = fvHits[triplet[2]];
    const int l_ista     = l_hit.Station();
    z_end                = z[l_ista];
    x_last               = x[l_ista];
    y_last               = y[l_ista];
    time_last            = time[l_ista];
    wtime_last           = sta[l_ista]->timeInfo ? 1. : 0.;
    dt2_last             = dt2[l_ista];
    mxy_last.X()         = mxy[l_ista].X();
    mxy_last.Y()         = mxy[l_ista].Y();
    mxy_last.Dx2()       = mxy[l_ista].Dx2();
    mxy_last.Dy2()       = mxy[l_ista].Dy2();
    mxy_last.Dxy()       = mxy[l_ista].Dxy();
    mxy_last.NdfX()      = mxy[l_ista].NdfX();
    mxy_last.NdfY()      = mxy[l_ista].NdfY();
  }

  for (int ih = nHitsTrack - 1; ih >= 0; ih--) {
    const int ista = iSta[ih];
    By[ista]       = sta[ista]->fieldSlice.GetFieldValue(0., 0.).y;
  }

  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  fit.GuessTrack(z_end, x, y, z, time, By, w, w_time, nStations);

  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  tr.Qp() = 1. / 1.1;

  for (int iter = 0; iter < 2; iter++) {  // 1.5 iterations

    fit.SetQp0(tr.Qp());

    // fit backward
    int ista = nStations - 1;

    time_last = w_time[ista] ? time_last : 0;
    dt2_last  = w_time[ista] ? dt2_last : 1.e6f;

    tr.ResetErrors(mxy_last.Dx2(), mxy_last.Dy2(), 0.1, 0.1, 1.0, dt2_last, 1.e-2);
    tr.C10()  = mxy_last.Dxy();
    tr.X()    = mxy_last.X();
    tr.Y()    = mxy_last.Y();
    tr.Time() = time_last;
    tr.Vi()   = constants::phys::SpeedOfLightInv;
    tr.InitVelocityRange(0.5);
    tr.Ndf()     = -5. + 2.;
    tr.NdfTime() = -2. + wtime_last;

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

      // printf("intialised: %d , z[ista]: %f , z_start: %f , z_end: %f", initialised, z[ista], z_start, z_end);

      fld1 = fld;

      fit.SetMask(initialised);
      fit.Extrapolate(z[ista], fld1);
      // printf("Extrapolate: %d ", iGThread);
      auto radThick = fMaterialMapTables[fMaterialMap[ista].GetBin(tr.X(), tr.Y())];
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
    // kf::GpuTrackKalmanFilter<float> fitpv = fit;
    // {
    //   fitpv.SetMask(true);

    //   ca::MeasurementXy<float> vtxInfo = fParams_const[fIteration].targetMeasurement;
    //   vtxInfo.SetDx2(1.e-8);
    //   vtxInfo.SetDxy(0.);
    //   vtxInfo.SetDy2(1.e-8);

    //   ca::GpuFieldRegion fldTarget;
    //   ca::GpuFieldValue B[3];

    //   float tx[3] = {(x[1] - x[0]) / (z[1] - z[0]), (x[2] - x[0]) / (z[2] - z[0]), (x[2] - x[1]) / (z[2] - z[1])};
    //   float ty[3] = {(y[1] - y[0]) / (z[1] - z[0]), (y[2] - y[0]) / (z[2] - z[0]), (y[2] - y[1]) / (z[2] - z[1])};
    //   for (int ih = 0; ih < 3; ++ih) {
    //     float dz = (sta[ih]->fZ - z[ih]);
    //     B[ih]    = sta[ih]->fieldSlice.GetFieldValue(x[ih] + tx[ih] * dz, y[ih] + ty[ih] * dz);
    //   };

    //   fld.Set(B[0], sta[0]->fZ, B[1], sta[1]->fZ, B[2], sta[2]->fZ);
    //   fldTarget.Set(fParams[fIterationData[0].fIteration].targB,
    //                 fParams[fIterationData[0].fIteration].GetTargetPositionZ(), B[0], sta[0]->fZ, B[1], sta[1]->fZ);

    //   ca::GpuFieldRegion fldFull(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
    //   fitpv.SetMaxExtrapolationStep(1.);
    //   for (int vtxIter = 0; vtxIter < 2; vtxIter++) {
    //     fitpv.SetQp0(fitpv.Tr().Qp());
    //     fitpv.Tr()      = fit.Tr();
    //     fitpv.Tr().Qp() = fitpv.Qp0();
    //     fitpv.Extrapolate(fParams_const[fIteration].GetTargetPositionZ(), fldFull);
    //     fitpv.FilterXY(vtxInfo);
    //   }
    // }

    // // OT : Use fitPV to determine if primary track
    // if (iter == 1 and fIteration == 1) {  // use iter 1 of KF fit for better fit, for all primary iteration
    //   const auto pv_x = fitpv.Tr().X();   // in cm
    //   const auto pv_y = fitpv.Tr().Y();
    //   const auto pv_z = fitpv.Tr().Z();
    //   if (xpu::isnan(pv_x) || xpu::isnan(pv_y) || xpu::isnan(pv_z) || (xpu::abs(pv_z + 44.0f) > 0.1)) {
    //     isPrimary = 1.0;  // just leave alone. Some of these are apparently useful
    //     continue;
    //   }
    //   const float dist = xpu::sqrt(pv_x * pv_x + pv_y * pv_y);
    //   if (dist > 1.0f) continue;  // 1 cm radius
    //   isPrimary = 1.0;
    // }

    if (iter == 1) {
      break;
    }  // only 1.5 iterations

    // fit forward

    ista = 0;

    tr.ResetErrors(mxy_first.Dx2(), mxy_first.Dy2(), 0.1, 0.1, 1., dt2_first, 1.e-2);
    tr.C10()  = mxy_first.Dxy();
    tr.X()    = mxy_first.X();
    tr.Y()    = mxy_first.Y();
    tr.Time() = time_first;
    tr.Vi()   = constants::phys::SpeedOfLightInv;
    tr.InitVelocityRange(0.5);
    tr.Ndf()     = -5. + 2.;
    tr.NdfTime() = -2. + wtime_first;
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
      auto radThick = fMaterialMapTables[fMaterialMap[ista].GetBin(tr.X(), tr.Y())];
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

  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  /// if track chi2 per dof is larger than threshold. Also kill negative and non-finite values
  /// if track p low than threshold_qp, then kill the track
  /// then remove triplet from list
  const float chi2 = fit.Tr().GetChiSq();
  bool killTrack   = !xpu::isfinite(chi2) || (chi2 < 0) || (chi2 > threshold_chi2);

  // momentum cut to reduce ghosts
  if (xpu::abs(fit.Tr().Qp()) > threshold_qp) {
    killTrack = true;
  }

  /// check isPrimary
  if (fIteration == 1) {
    if (isPrimary != 1.0) killTrack = true;  // not primary track
  }

  const float qp  = fit.Tr().Qp();
  const float Cqp = fit.Tr().C44() + 0.001;  // 0.001 magic number added. (see triplet constructor)
  const float Tx  = fit.Tr().Tx();
  const float C22 = fit.Tr().C22();
  const float Ty  = fit.Tr().Ty();
  const float C33 = fit.Tr().C33();
  const std::array<float, 7> tripletParams{chi2, qp, Cqp, Tx, C22, Ty, C33};
  fvTripletParams[iHitL][iTriplet]   = tripletParams;
  fTripletsSelected[iHitL][iTriplet] = !killTrack;
}  // FitTripletsOT

XPU_D void GnnGpuGraphConstructor::ConstructCandidates(ConstructCandidates::context& ctx) const
{
  // pass
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

// 1) Scan counts per hit -> fOffsets + per-block sums in fBlockOffsets
XPU_D void GnnGpuGraphConstructor::ExclusiveScan(ExclusiveScan::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  const int nHits = fIterationData[0].fNHits;
  if (iGThread >= nHits) return;

  const int tIdx = ctx.thread_idx_x();
  const int bIdx = ctx.block_idx_x();

  ExclusiveScan::scan_t scan{ctx.pos(), ctx.smem()};

  // input = count of valid triplets for this hit
  int input  = fNTriplets[iGThread];
  int result = 0;

  // exclusive prefix sum within the block
  scan.exclusive_sum(input, result);

  xpu::barrier(ctx.pos());

  // write per-thread partial (block-local) result
  fOffsets[iGThread] = result;

  if (bIdx == 0)
    printf("iGThread: %d , result: %d , fNTriplets: %d \n", iGThread, fOffsets[iGThread], fNTriplets[iGThread]);

  // the last thread in the block publishes the block sum
  if (tIdx == kScanBlockSize - 1 || iGThread == nHits - 1) {
    // block sum = last exclusive + its own input
    fBlockOffsets[bIdx] = result + input;
  }
}

// 2) Scan the block sums -> fBlockOffsets now holds global block offsets
XPU_D void GnnGpuGraphConstructor::AddBlockSums(AddBlockSums::context& ctx, int nBlocks) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= nBlocks) return;

  const int tIdx = ctx.thread_idx_x();
  const int bIdx = ctx.block_idx_x();

  AddBlockSums::scan_t scan{ctx.pos(), ctx.smem()};

  int input  = fBlockOffsets[iGThread];
  int result = 0;

  scan.exclusive_sum(input, result);

  xpu::barrier(ctx.pos());

  // overwrite with global block offset (start of this block in the full scan)
  fBlockOffsets[iGThread] = result;

  // keep last value per (meta)block range, mirroring your colleague’s pattern
  if (tIdx == kScanBlockSize - 1 || iGThread == nBlocks - 1) {
    fBlockOffsetsLast[bIdx] = input;  // last block’s sum (inclusive) inside this AddBlockSums launch block
  }
}

// 3) Add the global block offsets to each element to finalize the scan
XPU_D void GnnGpuGraphConstructor::AddOffsets(AddOffsets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  const int nHits = fIterationData[0].fNHits;
  if (iGThread >= nHits) return;

  const int bIdx = ctx.block_idx_x();

  // Add this block's global offset
  fOffsets[iGThread] += fBlockOffsets[bIdx];
}

// 4) Scatter into flat output
XPU_D void GnnGpuGraphConstructor::CompressAllTripletsOrdered(CompressAllTripletsOrdered::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  const int nHits = fIterationData[0].fNHits;
  if (iGThread >= nHits) return;

  const unsigned int count  = fNTriplets[iGThread];
  const unsigned int offset = fOffsets[iGThread];

  // Copy the valid, already-locally-ordered entries
  // Triplet element type: std::array<unsigned int, 2>
  for (unsigned int j = 0; j < count; ++j) {
    fTripletsFlat[offset + j] = fTriplets[iGThread][j];
  }
}
