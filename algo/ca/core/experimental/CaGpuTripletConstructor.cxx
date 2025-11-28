/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file CaGpuTripletConstructor.cxx
/// \brief The class contains data and kernels for running CA tracking on CPU and GPU using XPU libraries


#include "CaGpuTripletConstructor.h"

using namespace cbm::algo::ca;

// Export Constants
XPU_EXPORT(strGpuTripletConstructor);

// Kernel Definitions
//XPU_EXPORT(TestFunc);
//XPU_D void TestFunc::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().TestFunc(ctx); }

XPU_EXPORT(MakeSinglets);
XPU_D void MakeSinglets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().MakeSinglets(ctx); }

XPU_EXPORT(MakeDoublets);
XPU_D void MakeDoublets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().MakeDoublets(ctx); }

XPU_EXPORT(CompressDoublets);
XPU_D void CompressDoublets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().CompressDoublets(ctx); }

XPU_EXPORT(FitDoublets);
XPU_D void FitDoublets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().FitDoublets(ctx); }

XPU_EXPORT(MakeTriplets);
XPU_D void MakeTriplets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().MakeTriplets(ctx); }

XPU_EXPORT(CompressTriplets);
XPU_D void CompressTriplets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().CompressTriplets(ctx); }

XPU_EXPORT(FitTriplets);
XPU_D void FitTriplets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().FitTriplets(ctx); }

XPU_EXPORT(SortTriplets);
XPU_D void SortTriplets::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().SortTripletsFunc(ctx); }

//XPU_D void GpuTripletConstructor::TestFunc(TestFunc::context& ctx) const
//{
//  const int iGThread    = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
//}

XPU_D void GpuTripletConstructor::MakeSinglets(MakeSinglets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  if (iGThread >= fIterationData[0].fNHits) return;

  ca::HitIndex_t ihitl = fgridEntries[iGThread];  //iGThread;
  int ista             = fvHits[ihitl].Station();

  //  if(ista < 0 || ista > (int)fParams[fIterationData[0].fIteration].fNStations - 3) return;
  if (ista < 0 || ista > (int) fParams_const[fIteration].fNStations - 3) return;

  //  const ca::Station<float>* sta_l = &fParams[fIterationData[0].fIteration].GetStation(ista);
  //  const ca::Station<float>* sta_m = &fParams[fIterationData[0].fIteration].GetStation(ista + 1);
  const ca::GpuStation* sta_l = &fStations_const[ista];
  const ca::GpuStation* sta_m = &fStations_const[ista + 1];

  float stal_l_timeInfo = sta_l->timeInfo;
  float sta_m_z         = sta_m->fZ;

  const auto& hitl = fvHits[ihitl];

  //  const float dzli = 1.f / (hitl.Z() - fParams[fIterationData[0].fIteration].GetTargetPositionZ());
  //  const float tx = (hitl.X() - fParams[fIterationData[0].fIteration].GetTargetPositionX()) * dzli;
  //  const float ty = (hitl.Y() - fParams[fIterationData[0].fIteration].GetTargetPositionY()) * dzli;
  const float dzli = 1.f / (hitl.Z() - fParams_const[fIteration].GetTargetPositionZ());
  const float tx   = (hitl.X() - fParams_const[fIteration].GetTargetPositionX()) * dzli;
  const float ty   = (hitl.Y() - fParams_const[fIteration].GetTargetPositionY()) * dzli;

  kf::TrackParamBase<float>& fit_params = fvTrackParams[ihitl];

  fit_params.X()    = hitl.X();
  fit_params.Y()    = hitl.Y();
  fit_params.Z()    = hitl.Z();
  fit_params.Tx()   = tx;
  fit_params.Ty()   = ty;
  fit_params.Qp()   = 0.;
  fit_params.Time() = hitl.T();
  //  fit_params.Vi()   = fParams[fIterationData[0].fIteration].SpeedOfLightInv;//0.03335641;//constants::phys::SpeedOfLightInv;
  fit_params.Vi() = constants::phys::SpeedOfLightInv;  //fParams_const[fIteration].SpeedOfLightInv;

  //  float txErr2           = fParams[fIterationData[0].fIteration].maxSlopePV * fParams[fIterationData[0].fIteration].maxSlopePV / 9.;
  //  float qpErr2           = fParams[fIterationData[0].fIteration].maxQp * fParams[fIterationData[0].fIteration].maxQp / 9.;
  float txErr2 = fParams_const[fIteration].maxSlopePV * fParams_const[fIteration].maxSlopePV / 9.;
  float qpErr2 = fParams_const[fIteration].maxQp * fParams_const[fIteration].maxQp / 9.;

  fit_params.ResetErrors(1., 1., txErr2, txErr2, qpErr2, (stal_l_timeInfo ? hitl.dT2() : 1.e6), 1.e10);

  fit_params.InitVelocityRange(1. / /*fParams[fIterationData[0].fIteration]*/ fParams_const[fIteration].maxQp);

  fit_params.C(0, 0) = hitl.dX2();
  fit_params.C(1, 0) = hitl.dXY();
  fit_params.C(1, 1) = hitl.dY2();

  //  fit_params.Ndf()     = fParams[fIterationData[0].fIteration].primaryFlag ? 2. : 0.;
  fit_params.Ndf()     = fParams_const[fIteration].primaryFlag ? 2. : 0.;
  fit_params.NdfTime() = (stal_l_timeInfo ? 0 : -1);

  //  ca::GpuFieldRegion fld0;
  {
    int sta1 = ista;
    if (sta1 == 0) sta1 = 1;
    int sta0 = sta1 / 2;  // station between ista and the target
    //    const ca::GpuFieldValue &B0 = fParams[fIterationData[0].fIteration].GetStation(sta0).fieldSlice.GetFieldValueForLine(fit_params);
    //    const ca::GpuFieldValue &B1 = fParams[fIterationData[0].fIteration].GetStation(sta1).fieldSlice.GetFieldValueForLine(fit_params);
    const ca::GpuFieldValue& B0 = fStations_const[sta0].fieldSlice.GetFieldValueForLine(fit_params);
    const ca::GpuFieldValue& B1 = fStations_const[sta1].fieldSlice.GetFieldValueForLine(fit_params);
    //fld0.Set(fParams[fIterationData[0].fIteration].targB, fParams[fIterationData[0].fIteration].GetTargetPositionZ(), B0, fParams[fIterationData[0].fIteration].GetStation(sta0).fZ, B1, fParams[fIterationData[0].fIteration].GetStation(sta1).fZ);
    ctx.smem().fld0_shared[ctx.thread_idx_x()].Set(fParams_const[fIteration].targB,
                                                   fParams_const[fIteration].GetTargetPositionZ(), B0,
                                                   fStations_const[sta0].fZ, B1, fStations_const[sta1].fZ);
  }
  //  ca::GpuFieldRegion fld1;
  {  // field, made by the left hit, the middle station and the right station
    //    const ca::GpuFieldValue &B0 = fParams[fIterationData[0].fIteration].GetStation(ista)*/fStations_const[ista]/*ctx.smem().fStations_shared[ista]*/.fieldSlice.GetFieldValueForLine(fit_params);
    //    const ca::GpuFieldValue &B1 = fParams[fIterationData[0].fIteration].GetStation(ista + 1).fieldSlice.GetFieldValueForLine(fit_params);
    //    const ca::GpuFieldValue &B2 = fParams[fIterationData[0].fIteration].GetStation(ista + 2).fieldSlice.GetFieldValueForLine(fit_params);
    const ca::GpuFieldValue& B0 = fStations_const[ista].fieldSlice.GetFieldValueForLine(fit_params);
    const ca::GpuFieldValue& B1 = fStations_const[ista + 1].fieldSlice.GetFieldValueForLine(fit_params);
    const ca::GpuFieldValue& B2 = fStations_const[ista + 2].fieldSlice.GetFieldValueForLine(fit_params);
    //    fld1.Set(B0, fParams[fIterationData[0].fIteration].GetStation(ista).fZ, B1, fParams[fIterationData[0].fIteration].GetStation(ista + 1).fZ, B2, fParams[fIterationData[0].fIteration].GetStation(ista + 2).fZ);
    ctx.smem().fld1_shared[ctx.thread_idx_x()].Set(B0, fStations_const[ista].fZ, B1, fStations_const[ista + 1].fZ, B2,
                                                   fStations_const[ista + 2].fZ);
  }

  FilterWithTargetAtLine(&fit_params, &/*fld0*/ ctx.smem().fld0_shared[ctx.thread_idx_x()]);

  MultipleScattering(&fit_params, fMaterialMapTables[fMaterialMap[ista].GetBin(fit_params.GetX(), fit_params.GetY())],
                     /*fParams[fIterationData[0].fIteration]*/ fParams_const[fIteration].maxQp);

  // extrapolate to the middle hit
  ExtrapolateStep(&fit_params, sta_m_z, 0.f, &/*fld1*/ ctx.smem().fld1_shared[ctx.thread_idx_x()]);

  //  ExtrapolateLine(tr_par, sta_m->fZ, F1);	//TODO: check if ExtrapolateStep is enough
}

XPU_D void GpuTripletConstructor::MakeDoublets(MakeDoublets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fIterationData[0].fNHits) return;

#if 0
  if (ctx.thread_idx_x() == 0) {
      ctx.smem().counter = 0;
  }
#endif

  ca::HitIndex_t ihitl = fgridEntries[iGThread];  //iGThread;
  int ista             = fvHits[ihitl].Station();

  if (ista > (int) fParams[fIterationData[0].fIteration].fNStations - 3) return;

  int collectedHits = 0;
  int maxNHits      = constants::gpu::MaxDoubletsFromHit;  //fParams[fIterationData[0].fIteration].maxDoubletsFromHit;

  CollectHits(iGThread, ista + 1, ihitl, maxNHits, &collectedHits);

  xpu::atomic_add(&(fIterationData[0].fNDoublets), collectedHits);

#if 0
  xpu::atomic_add(&(ctx.smem().counter), collectedHits);

  xpu::barrier(ctx.pos());
  if(ctx.thread_idx_x() == 0) {
    xpu::atomic_add(&(fIterationData[0].fNDoublets), ctx.smem().counter);
  }
#endif
}

XPU_D void GpuTripletConstructor::CompressDoublets(CompressDoublets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  //  if( iGThread >= fIterationData[0].fNHits * fParams[fIterationData[0].fIteration].maxDoubletsFromHit ) return;
  if (iGThread >= fIterationData[0].fNHits * constants::gpu::MaxDoubletsFromHit) return;

  if (fHitDoublets[iGThread].hit2 != 0) {
    int index                          = xpu::atomic_add(&(fIterationData[0].fNDoublets_counter), 1);
    fHitDoubletsCompressed[index].hit1 = fHitDoublets[iGThread].hit1;
    fHitDoubletsCompressed[index].hit2 = fHitDoublets[iGThread].hit2;
  }
}

XPU_D void GpuTripletConstructor::FitDoublets(FitDoublets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fIterationData[0].fNDoublets) return;

  int hit_l = fHitDoubletsCompressed[iGThread].hit1;
  int hit_m = fHitDoubletsCompressed[iGThread].hit2;

  if (hit_l < 0 || hit_m < 0) return;

  int ista = fvHits[hit_l].Station();

  if (ista < 0 || ista > (int) fParams[fIterationData[0].fIteration].fNStations - 3) return;

  const ca::GpuStation& sta_l = fParams[fIterationData[0].fIteration].GetStation(ista);
  const ca::GpuStation& sta_m = fParams[fIterationData[0].fIteration].GetStation(ista + 1);

  auto fit_params = fvTrackParams[hit_l];

  bool isMomentumFitted =
    (fParams[fIterationData[0].fIteration].isTargetField || (sta_l.fieldStatus != 0) || (sta_m.fieldStatus != 0));
  float maxQp = fParams[fIterationData[0].fIteration].maxQp;

  // fit doublet
  const ca::Hit& hitm = fvHits[hit_m];

  float z_2 = hitm.Z();
  ca::MeasurementXy<float> m_2(hitm.X(), hitm.Y(), hitm.dX2(), hitm.dY2(), hitm.dXY(), 1.f, 1.f);
  float t_2   = hitm.T();
  float dt2_2 = hitm.dT2();

  // add the middle hit
  ExtrapolateLineNoField(&fit_params, z_2);

  FilterXY(&fit_params, m_2);

  FilterTime(&fit_params, t_2, dt2_2, sta_m.timeInfo);

  //      fFit.SetQp0(isMomentumFitted ? fFit.Tr().GetQp() : maxQp);	//TODO: check why it is required
  const float qp0 = isMomentumFitted ? fit_params.GetQp() : maxQp;

  MultipleScattering(&fit_params,
                     fMaterialMapTables[fMaterialMap[ista + 1].GetBin(fit_params.GetX(), fit_params.GetY())], qp0);

  fvTrackParamsDoublets[iGThread] = fit_params;
  //      fFit.SetQp0(fFit.Tr().Qp());	//TODO: check why it is required and how it is used later, have to be stored different way
}

XPU_D void GpuTripletConstructor::MakeTriplets(MakeTriplets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fIterationData[0].fNDoublets) return;

  int hit_l = fHitDoubletsCompressed[iGThread].hit1;
  //  int hit_m = fHitDoubletsCompressed[iGThread].hit2;

#if 0
  if (ctx.thread_idx_x() == 0) {
      ctx.smem().counter = 0;
  }
#endif

  int ista = fvHits[hit_l].Station();

  if (ista < 0 || ista > (int) fParams[fIterationData[0].fIteration].fNStations - 3) return;

  //
  ista = fvHits[hit_l].Station();
  //

  //  const ca::Station<float>& sta_l = fParams[fIterationData[0].fIteration].GetStation(ista);
  //  const ca::Station<float>& sta_m = fParams[fIterationData[0].fIteration].GetStation(ista + 1);
  const ca::GpuStation& sta_r = fParams[fIterationData[0].fIteration].GetStation(ista + 2);

  auto& fit_params = fvTrackParamsDoublets[iGThread];

  ca::GpuFieldRegion
    fld1;  //TODO: with fit_params(doublets) this is not fFldL from orig, but it should be more precise, need to be checker and clarified
  {        // field, made by the left hit, the middle station and the right station
    ca::GpuFieldValue B0 =
      fParams[fIterationData[0].fIteration].GetStation(ista).fieldSlice.GetFieldValueForLine(fit_params);
    ca::GpuFieldValue B1 =
      fParams[fIterationData[0].fIteration].GetStation(ista + 1).fieldSlice.GetFieldValueForLine(fit_params);
    ca::GpuFieldValue B2 =
      fParams[fIterationData[0].fIteration].GetStation(ista + 2).fieldSlice.GetFieldValueForLine(fit_params);
    fld1.Set(B0, fParams[fIterationData[0].fIteration].GetStation(ista).fZ, B1,
             fParams[fIterationData[0].fIteration].GetStation(ista + 1).fZ, B2,
             fParams[fIterationData[0].fIteration].GetStation(ista + 2).fZ);
  }

  ExtrapolateStep(&fit_params, sta_r.fZ, 0.f, &fld1);

  //TODO: add isDoubletGood check if needed

  int collectedHits = 0;
  int maxNHits =
    constants::gpu::MaxTripletsFromDoublet;  //fParams[fIterationData[0].fIteration].maxTripletsFromDoublet;

  CollectHitsTriplets(iGThread, ista + 2, maxNHits, &collectedHits);

  xpu::atomic_add(&(fIterationData[0].fNTriplets), collectedHits);

#if 0
  xpu::atomic_add(&(ctx.smem().counter), collectedHits);

  xpu::barrier(ctx.pos());

  if (ctx.thread_idx_x () == 0) {
    fIterationData[0].fNTriplets += ctx.smem().counter;
  }
#endif
}

XPU_D void GpuTripletConstructor::CompressTriplets(CompressTriplets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  //  if( iGThread >= fIterationData[0].fNDoublets * fParams[fIterationData[0].fIteration].maxTripletsFromDoublet ) return;
  if (iGThread >= fIterationData[0].fNDoublets * constants::gpu::MaxTripletsFromDoublet) return;

  if (fHitTriplets[iGThread].hit3 != 0) {
    int index                          = xpu::atomic_add(&(fIterationData[0].fNTriplets_counter), 1);
    fHitTripletsCompressed[index].hit1 = fHitTriplets[iGThread].hit1;
    fHitTripletsCompressed[index].hit2 = fHitTriplets[iGThread].hit2;
    fHitTripletsCompressed[index].hit3 = fHitTriplets[iGThread].hit3;
  }
}

XPU_D void GpuTripletConstructor::FitTriplets(FitTriplets::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  if (iGThread >= fIterationData[0].fNTriplets) return;

  int hit_l = fHitTripletsCompressed[iGThread].hit1;

  int ista[3] = {-1, -1, -1};

  ista[0] = fvHits[hit_l].Station();
  ista[1] = ista[0] + 1;
  ista[2] = ista[1] + 1;

  //  ca::GpuStation sta[3];
  const ca::GpuStation* sta[3];
  for (int is = 0; is < 3; ++is) {
    //    sta[is] = fParams[fIterationData[0].fIteration].GetStation(ista[is]);
    sta[is] = &fStations_const[ista[is]];
  };

  bool isMomentumFitted = ((sta[0]->fieldStatus != 0) || (sta[1]->fieldStatus != 0) || (sta[2]->fieldStatus != 0));

  float ndfTrackModel = 4;                    // straight line
  ndfTrackModel += isMomentumFitted ? 1 : 0;  // track with momentum

  const float maxQp = fParams[fIterationData[0].fIteration].maxQp;

  ca::HitIndex_t ihit[3] = {fHitTripletsCompressed[iGThread].hit1, fHitTripletsCompressed[iGThread].hit2,
                            fHitTripletsCompressed[iGThread].hit3};

  float x[3], y[3], z[3], t[3];
  float dt2[3];
  ca::MeasurementXy<float> mxy[3];

  for (int ih = 0; ih < 3; ++ih) {
    const ca::Hit& hit = fvHits[ihit[ih]];
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
  //  auto *fit_params = &ctx.smem().fit_params_shared[ctx.thread_idx_x()];	//TODO: shared memory cannot be used for classes with preinitialized data

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
      FilterWithTargetAtLine(&fit_params, &fldTarget);
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
  fvTriplets[iGThread] =
    ca::Triplet(ihit[0], ihit[1], ihit[2], ista[0], ista[1], ista[2], 0, 0, 0, chi2, qp, Cqp, fit_params.Tx(),
                fit_params.C(2, 2), fit_params.Ty(), fit_params.C(3, 3), isMomentumFitted);

  if constexpr (constants::gpu::GpuSortTriplets) {
    fTripletSortHelper[iGThread].tr_id = iGThread;
    //    fTripletSortHelper[iGThread].lsta  = ista[0];
    fTripletSortHelper[iGThread].lhit = ihit[0];
  }
}

XPU_D void GpuTripletConstructor::SortTripletsFunc(SortTriplets::context& ctx) const
{
  //TODO: implement sorting of triplets on GPU or make tracking order independent
}

XPU_D void GpuTripletConstructor::CollectHits(int iThread, int iSta, int iHit, int maxNHits, int* collectedHits) const
{
  // Collect hits in the station iSta and make doublets
  const ca::GpuStation& sta            = fParams[fIterationData[0].fIteration].GetStation(iSta);
  kf::TrackParamBase<float> fit_params = fvTrackParams[iHit];

  fit_params.ChiSq() = 0.;

  const float Pick_m22 = fParams[fIterationData[0].fIteration].doubletChi2Cut - fit_params.GetChiSq();

  const float timeError2 = fit_params.C(5, 5);
  const float time       = fit_params.Time();

  const auto& gpuGrid = fvGpuGrid[iSta];

  const float pick_range_x = xpu::sqrt(Pick_m22 * fit_params.C(0, 0)) + gpuGrid.GetMaxRangeX()
                             + fParams[fIterationData[0].fIteration].maxDZ * abs(fit_params.Tx());
  const float pick_range_y = xpu::sqrt(Pick_m22 * fit_params.C(1, 1)) + gpuGrid.GetMaxRangeY()
                             + fParams[fIterationData[0].fIteration].maxDZ * abs(fit_params.Ty());

  ca::GpuGridArea areaGpu(gpuGrid, &(fgridFirstBinEntryIndex[fvGpuGrid[iSta].GetBinEntryIndexStart()]),
                          &(fgridEntries[fvGpuGrid[iSta].GetEntriesStart()]), fit_params.X(), fit_params.Y(),
                          pick_range_x, pick_range_y);
  unsigned int ih_gpu = 0;

  while (areaGpu.GetNextObjectId(ih_gpu) && (*collectedHits < maxNHits)) {  // loop over station hits
                                                                            //    if (fvbHitSuppressed[ih]) {
                                                                            //      continue;
                                                                            //    }
    const ca::Hit& hit = fvHits[ih_gpu];

    // check time-boundaries
    if ((sta.timeInfo) && (fit_params.NdfTime() >= 0)) {
      if (xpu::abs(time - hit.T()) > 1.4 * (3.5 * xpu::sqrt(timeError2) + hit.RangeT())) {
        continue;
      }
      // if (fabs(time - hit.T()) > 30) continue;
    }

    // - check whether hit belong to the window ( track position +\- errors ) -

    // check y-boundaries
    float y = fit_params.Y() + fit_params.Ty() * (hit.Z() - fit_params.Z());
    float C11 =
      fit_params.C(1, 1)
      + (hit.Z() - fit_params.Z()) * (2 * fit_params.C(3, 1) + (hit.Z() - fit_params.Z()) * fit_params.C(3, 3));

    float dy_est = xpu::sqrt(Pick_m22 * C11) + hit.RangeY();

    const float dY = hit.Y() - y;

    if (xpu::abs(dY) > dy_est) {
      continue;
    }

    // check x-boundaries
    float x = fit_params.X() + fit_params.Tx() * (hit.Z() - fit_params.Z());
    float C00 =
      fit_params.C(0, 0)
      + (hit.Z() - fit_params.Z()) * (2 * fit_params.C(2, 0) + (hit.Z() - fit_params.Z()) * fit_params.C(2, 2));

    float dx_est = xpu::sqrt(Pick_m22 * C00) + hit.RangeX();

    const float dX = hit.X() - x;

    if (xpu::abs(dX) > dx_est) {
      continue;
    }

    // check chi2
    float C10 = fit_params.C(1, 0)
                + (hit.Z() - fit_params.Z())
                    * (fit_params.C(2, 1) + fit_params.C(3, 0) + (hit.Z() - fit_params.Z()) * fit_params.C(3, 2));

    ca::MeasurementXy<float> mxy(hit.X(), hit.Y(), hit.dX2(), hit.dY2(), hit.dXY(), 1.f, 1.f);

    float chi2x = 0.;
    {  // filter X measurement
      const float zeta = x - mxy.X();

      const float F0 = C00;
      const float F1 = C10;

      const float HCH = F0;

      const float wi     = 1.f / (mxy.Dx2() + HCH);
      const float zetawi = zeta * wi;
      chi2x              = mxy.NdfX() * zeta * zetawi;

      const float K1 = F1 * wi;

      x -= F0 * zetawi;
      y -= F1 * zetawi;

      C00 -= F0 * F0 * wi;
      C10 -= K1 * F0;
      C11 -= K1 * F1;
    }

    float chi2u = 0.;
    {  // filter U measurement, we need only chi2 here
      const float cosPhi = -mxy.Dxy() / mxy.Dx2();
      const float u      = cosPhi * mxy.X() + mxy.Y();
      const float du2    = mxy.Dy2() + cosPhi * mxy.Dxy();

      const float zeta = cosPhi * x + y - u;

      const float F0 = cosPhi * C00 + C10;
      const float F1 = cosPhi * C10 + C11;

      const float HCH = (F0 * cosPhi + F1);

      chi2u += mxy.NdfY() * zeta * zeta / (du2 + HCH);
    }

    if (chi2x > fParams[fIterationData[0].fIteration].doubletChi2Cut) {
      continue;
    }
    if (chi2x + chi2u > fParams[fIterationData[0].fIteration].doubletChi2Cut) {
      continue;
    }
    int id                = iThread * maxNHits + *collectedHits;
    fHitDoublets[id].hit1 = iHit;
    fHitDoublets[id].hit2 = ih_gpu;
    *collectedHits += 1;
    //TODO: add clone hit suppression -> should be done after doublet fitting actually
  }  // loop over the hits in the area
}

XPU_D void GpuTripletConstructor::CollectHitsTriplets(int iThread, int iSta, int maxNHits, int* collectedHits) const
{
  const ca::GpuStation& sta            = fParams[fIterationData[0].fIteration].GetStation(iSta);
  kf::TrackParamBase<float> fit_params = fvTrackParamsDoublets[iThread];

  fit_params.ChiSq() = 0.;

  const float Pick_m22 = fParams[fIterationData[0].fIteration].tripletChi2Cut - fit_params.GetChiSq();

  const float timeError2 = fit_params.C(5, 5);
  const float time       = fit_params.Time();

  const auto& gpuGrid = fvGpuGrid[iSta];

  const float pick_range_x = xpu::sqrt(Pick_m22 * fit_params.C(0, 0)) + gpuGrid.GetMaxRangeX()
                             + fParams[fIterationData[0].fIteration].maxDZ * abs(fit_params.Tx());
  const float pick_range_y = xpu::sqrt(Pick_m22 * fit_params.C(1, 1)) + gpuGrid.GetMaxRangeY()
                             + fParams[fIterationData[0].fIteration].maxDZ * abs(fit_params.Ty());

  ca::GpuGridArea areaGpu(gpuGrid, &(fgridFirstBinEntryIndex[fvGpuGrid[iSta].GetBinEntryIndexStart()]),
                          &(fgridEntries[fvGpuGrid[iSta].GetEntriesStart()]), fit_params.X(), fit_params.Y(),
                          pick_range_x, pick_range_y);
  unsigned int ih_gpu = 0;

  while (areaGpu.GetNextObjectId(ih_gpu) && (*collectedHits < maxNHits)) {  // loop over station hits
                                                                            //    if (fvbHitSuppressed[ih]) {
                                                                            //      continue;
                                                                            //    }
    const ca::Hit& hit = fvHits[ih_gpu];

    // check time-boundaries

    //TODO: move hardcoded cuts to parameters
    if ((sta.timeInfo) && (fit_params.NdfTime() >= 0)) {
      if (xpu::abs(time - hit.T()) > 1.4 * (3.5 * xpu::sqrt(timeError2) + hit.RangeT())) {
        continue;
      }
      // if (fabs(time - hit.T()) > 30) continue;
    }

    // - check whether hit belong to the window ( track position +\- errors ) -

    // check y-boundaries
    float y = fit_params.Y() + fit_params.Ty() * (hit.Z() - fit_params.Z());
    float C11 =
      fit_params.C(1, 1)
      + (hit.Z() - fit_params.Z()) * (2 * fit_params.C(3, 1) + (hit.Z() - fit_params.Z()) * fit_params.C(3, 3));

    float dy_est = xpu::sqrt(Pick_m22 * C11) + hit.RangeY();

    const float dY = hit.Y() - y;

    if (xpu::abs(dY) > dy_est) {
      continue;
    }

    // check x-boundaries
    float x = fit_params.X() + fit_params.Tx() * (hit.Z() - fit_params.Z());
    float C00 =
      fit_params.C(0, 0)
      + (hit.Z() - fit_params.Z()) * (2 * fit_params.C(2, 0) + (hit.Z() - fit_params.Z()) * fit_params.C(2, 2));

    float dx_est = xpu::sqrt(Pick_m22 * C00) + hit.RangeX();

    const float dX = hit.X() - x;

    if (fabs(dX) > dx_est) {
      continue;
    }

    // check chi2
    float C10 = fit_params.C(1, 0)
                + (hit.Z() - fit_params.Z())
                    * (fit_params.C(2, 1) + fit_params.C(3, 0) + (hit.Z() - fit_params.Z()) * fit_params.C(3, 2));

    ca::MeasurementXy<float> mxy(hit.X(), hit.Y(), hit.dX2(), hit.dY2(), hit.dXY(), 1.f, 1.f);

    float chi2x = 0.;
    {  // filter X measurement
      const float zeta = x - mxy.X();

      const float F0 = C00;
      const float F1 = C10;

      const float HCH = F0;

      const float wi     = 1.f / (mxy.Dx2() + HCH);
      const float zetawi = zeta * wi;
      chi2x              = mxy.NdfX() * zeta * zetawi;

      const float K1 = F1 * wi;

      x -= F0 * zetawi;
      y -= F1 * zetawi;

      C00 -= F0 * F0 * wi;
      C10 -= K1 * F0;
      C11 -= K1 * F1;
    }

    float chi2u = 0.;
    {  // filter U measurement, we need only chi2 here
      const float cosPhi = -mxy.Dxy() / mxy.Dx2();
      const float u      = cosPhi * mxy.X() + mxy.Y();
      const float du2    = mxy.Dy2() + cosPhi * mxy.Dxy();

      const float zeta = cosPhi * x + y - u;

      const float F0 = cosPhi * C00 + C10;
      const float F1 = cosPhi * C10 + C11;

      const float HCH = (F0 * cosPhi + F1);

      chi2u += mxy.NdfY() * zeta * zeta / (du2 + HCH);
    }

    if (chi2x > fParams[fIterationData[0].fIteration].tripletChi2Cut) {
      continue;
    }
    if (chi2x + chi2u > fParams[fIterationData[0].fIteration].tripletChi2Cut) {
      continue;
    }

    int id                = iThread * maxNHits + *collectedHits;
    fHitTriplets[id].hit1 = fHitDoubletsCompressed[iThread].hit1;
    fHitTriplets[id].hit2 = fHitDoubletsCompressed[iThread].hit2;
    fHitTriplets[id].hit3 = ih_gpu;
    *collectedHits += 1;
  }  // loop over the hits in the area
  //  if( *collectedHits > 2 ) printf("- iThread: %d; collectedHits: %d\n", iThread, *collectedHits);
}

XPU_D void GpuTripletConstructor::FilterWithTargetAtLine(kf::TrackParamBase<float>* tr_par,
                                                         const ca::GpuFieldRegion* F0) const
{
  // Add the target constraint to a straight line track

  float eXY[2];
  float Jx[7], Jy[7];

  GetExtrapolatedXYline(tr_par,
                        /*fParams[fIterationData[0].fIteration]*/ fParams_const[fIteration].GetTargetPositionZ(), F0,
                        eXY /*, &eY*/, Jx, Jy);

  FilterExtrapolatedXY(tr_par, &(/*fParams[fIterationData[0].fIteration]*/ fParams_const[fIteration].targetMeasurement),
                       eXY /*, eY*/, Jx, Jy);
}

XPU_D void GpuTripletConstructor::GetExtrapolatedXYline(
  kf::TrackParamBase<float>* tr_par, float z, const ca::GpuFieldRegion* F, float* extrXY,  // float* extrY,
  //                                   std::array<float, ca::TrackParamBase<float>::kNtrackParam>* Jx,
  //                                   std::array<float, ca::TrackParamBase<float>::kNtrackParam>* Jy) const
  float* Jx, float* Jy) const
{
  // extrapolate track assuming it is straight (qp==0)
  // return the extrapolated X, Y and the derivatives of the extrapolated X and Y

  const float c_light(0.000299792458), c1(1.), c2i(0.5), c6i(/*1. / 6.*/ 0.166667),
    c12i(/*1. / 12.*/ 0.083333);  //TODO: test constants

  const float tx = tr_par->GetTx();
  const float ty = tr_par->GetTy();
  const float dz = z - tr_par->GetZ();

  const float dz2     = dz * dz;
  const float dzc6i   = dz * c6i;
  const float dz2c12i = dz2 * c12i;

  const float xx = tx * tx;
  const float yy = ty * ty;
  const float xy = tx * ty;

  const float Ay = -xx - c1;
  const float Bx = yy + c1;

  const float ctdz2 = c_light * xpu::sqrt(c1 + xx + yy) * dz2;

  const float Sx = F->cx0 * c2i + F->cx1 * dzc6i + F->cx2 * dz2c12i;
  const float Sy = F->cy0 * c2i + F->cy1 * dzc6i + F->cy2 * dz2c12i;
  const float Sz = F->cz0 * c2i + F->cz1 * dzc6i + F->cz2 * dz2c12i;

  extrXY[0] = tr_par->GetX() + tx * dz;
  extrXY[1] = tr_par->GetY() + ty * dz;

  //  Jx->fill(0.);  // for a case
  //  Jy->fill(0.);

  Jx[0] = 1.;
  Jx[1] = 0.;
  Jx[2] = dz;
  Jx[3] = 0.;
  Jx[4] = ctdz2 * (Sx * xy + Sy * Ay + Sz * ty);
  Jx[5] = 0.;
  Jx[6] = 0.;

  Jy[0] = 0.;
  Jy[1] = 1.;
  Jy[2] = 0.;
  Jy[3] = dz;
  Jy[4] = ctdz2 * (Sx * Bx - Sy * xy - Sz * tx);
  Jy[5] = 0.;
  //  Jx[6] = 0.;	//TODO: is it a bug???
  Jy[6] = 0.;
}

XPU_D void GpuTripletConstructor::FilterExtrapolatedXY(
  kf::TrackParamBase<float>* tr_par, const ca::MeasurementXy<float>* m, float* extrXY,  // fvec extrY,
  //			      std::array<float, ca::TrackParamBase<float>::kNtrackParam>* Jx,
  //			      std::array<float, ca::TrackParamBase<float>::kNtrackParam>* Jy) const
  float* Jx, float* Jy) const
{
  // add a 2-D measurenent (x,y) at some z, that differs from fTr.GetZ()
  // extrX, extrY are extrapolated track parameters at z, Jx, Jy are derivatives of the extrapolation

  // ! it is assumed that in the track covariance matrix all non-diagonal covariances are 0
  // ! except of C10

  //  TrackParamV& T = fTr;

  //zeta0 = T.x + Jx[2]*T.Tx() + Jx[3]*T.Ty() + Jx[4]*T.qp - x;
  //zeta1 = T.y + Jy[2]*T.Tx() + Jy[3]*T.Ty() + Jy[4]*T.qp - y;

  const float zeta0 = extrXY[0] - m->X();
  const float zeta1 = extrXY[1] - m->Y();

  // H = 1 0 Jx[2] Jx[3] Jx[4] 0
  //     0 1 Jy[2] Jy[3] Jy[4] 0

  // F = CH'
  const float F00 = tr_par->C(0, 0);
  const float F01 = tr_par->C(1, 0);  //TODO: is it a bug???
  const float F10 = F01;              //tr_par->C10();
  const float F11 = tr_par->C(1, 1);
  const float F20 = Jx[2] * tr_par->C(2, 2);
  const float F21 = Jy[2] * tr_par->C(2, 2);
  const float F30 = Jx[3] * tr_par->C(3, 3);
  const float F31 = Jy[3] * tr_par->C(3, 3);
  const float F40 = Jx[4] * tr_par->C(4, 4);
  const float F41 = Jy[4] * tr_par->C(4, 4);

  // Jx[5,6] and Jy[5,6] are 0.

  float S00 = m->Dx2() + F00 + Jx[2] * F20 + Jx[3] * F30 + Jx[4] * F40;
  float S10 = m->Dxy() + F10 + Jy[2] * F20 + Jy[3] * F30 + Jy[4] * F40;
  float S11 = m->Dy2() + F11 + Jy[2] * F21 + Jy[3] * F31 + Jy[4] * F41;

  const float si     = 1. / (S00 * S11 - S10 * S10);
  const float S00tmp = S00;
  S00                = si * S11;
  S10                = -si * S10;
  S11                = si * S00tmp;

  tr_par->ChiSq() += zeta0 * zeta0 * S00 + 2. * zeta0 * zeta1 * S10 + zeta1 * zeta1 * S11;
  tr_par->Ndf() += m->NdfX() + m->NdfY();

  const float K00 = F00 * S00 + F01 * S10;
  const float K01 = F00 * S10 + F01 * S11;
  const float K10 = F10 * S00 + F11 * S10;
  const float K11 = F10 * S10 + F11 * S11;
  const float K20 = F20 * S00 + F21 * S10;
  const float K21 = F20 * S10 + F21 * S11;
  const float K30 = F30 * S00 + F31 * S10;
  const float K31 = F30 * S10 + F31 * S11;
  const float K40 = F40 * S00 + F41 * S10;
  const float K41 = F40 * S10 + F41 * S11;

  tr_par->X() -= K00 * zeta0 + K01 * zeta1;
  tr_par->Y() -= K10 * zeta0 + K11 * zeta1;
  tr_par->Tx() -= K20 * zeta0 + K21 * zeta1;
  tr_par->Ty() -= K30 * zeta0 + K31 * zeta1;
  tr_par->Qp() -= K40 * zeta0 + K41 * zeta1;

  tr_par->C(0, 0) -= (K00 * F00 + K01 * F01);
  tr_par->C(1, 0) -= (K10 * F00 + K11 * F01);
  tr_par->C(1, 1) -= (K10 * F10 + K11 * F11);
  tr_par->C(2, 0) = -(K20 * F00 + K21 * F01);
  tr_par->C(2, 1) = -(K20 * F10 + K21 * F11);
  tr_par->C(2, 2) -= (K20 * F20 + K21 * F21);
  tr_par->C(3, 0) = -(K30 * F00 + K31 * F01);
  tr_par->C(3, 1) = -(K30 * F10 + K31 * F11);
  tr_par->C(3, 2) = -(K30 * F20 + K31 * F21);
  tr_par->C(3, 3) -= (K30 * F30 + K31 * F31);
  tr_par->C(4, 0) = -(K40 * F00 + K41 * F01);
  tr_par->C(4, 1) = -(K40 * F10 + K41 * F11);
  tr_par->C(4, 2) = -(K40 * F20 + K41 * F21);
  tr_par->C(4, 3) = -(K40 * F30 + K41 * F31);
  tr_par->C(4, 4) -= (K40 * F40 + K41 * F41);
}

XPU_D void GpuTripletConstructor::MultipleScattering(kf::TrackParamBase<float>* tr_par, float radThick, float qp0) const
{
  float tx    = tr_par->Tx();
  float ty    = tr_par->Ty();
  float txtx  = tx * tx;
  float tyty  = ty * ty;
  float txtx1 = txtx + 1.f;
  float h     = txtx + tyty;
  float t     = xpu::sqrt(txtx1 + tyty);
  float h2    = h * h;
  float qp0t  = qp0 * t;

  float c1 = 0.0136f, c2 = c1 * 0.038f, c3 = c2 * 0.5f, c4 = -c3 * 0.5f, c5 = c3 * 0.333333f, c6 = -c3 * 0.25f;

  float s0 = (c1 + c2 * xpu::log(radThick) + c3 * h + h2 * (c4 + c5 * h + c6 * h2)) * qp0t;
  //  float a = ((t + fParams[fIterationData[0].fIteration].particleMass * fParams[fIterationData[0].fIteration].particleMass * qp0 * qp0t) * radThick * s0 * s0);
  float a = ((t + fParams_const[fIteration].particleMass * fParams_const[fIteration].particleMass * qp0 * qp0t)
             * radThick * s0 * s0);
  tr_par->C(2, 2) += txtx1 * a;
  tr_par->C(3, 2) += tx * ty * a;
  tr_par->C(3, 3) += (1. + tyty) * a;
}

XPU_D void GpuTripletConstructor::ExtrapolateStep(kf::TrackParamBase<float>* tr_par, float z_out, float qp0,
                                                  const ca::GpuFieldRegion* Field) const
{
  const float c_light = 0.000299792458;

  //----------------------------------------------------------------
  const float h = z_out - tr_par->GetZ();

  const float stepDz[5] = {0., 0., h * 0.5f, h * 0.5f, h};

  float f[5][7]    = {{0.}};    // ( d*/dz  ) [step]
  float F[5][7][7] = {{{0.}}};  // ( d *new [step] / d *old  )

  //   Runge-Kutta steps
  //
  const float r0[7] = {tr_par->X(), tr_par->Y(), tr_par->Tx(), tr_par->Ty(), qp0, tr_par->Time(), tr_par->Vi()};
  float R0[7][7]    = {{0.}};
  for (int i = 0; i < 7; ++i) {
    R0[i][i] = 1.;
  }

  for (int step = 1; step <= 4; ++step) {

    float rstep[7] = {0.};
    for (int i = 0; i < 7; ++i) {
      rstep[i] = r0[i] + stepDz[step] * f[step - 1][i];
    }
    const float z       = tr_par->GetZ() + stepDz[step];
    ca::GpuFieldValue B = Field->Get(rstep[0], rstep[1], z);
    const float tx2     = rstep[2] * rstep[2];
    const float ty2     = rstep[3] * rstep[3];
    const float txty    = rstep[2] * rstep[3];
    const float L2      = 1.f + tx2 + ty2;
    const float L2i     = 1.f / L2;
    const float L       = xpu::sqrt(L2);
    const float cL      = c_light * L;
    const float cLqp0   = cL * qp0;

    f[step][0]    = rstep[2];
    F[step][0][2] = 1.;

    f[step][1]    = rstep[3];
    F[step][1][3] = 1.;

    const float f2tmp = txty * B.x - (1. + tx2) * B.y + rstep[3] * B.z;
    f[step][2]        = cLqp0 * f2tmp;

    F[step][2][2] = cLqp0 * (rstep[2] * f2tmp * L2i + rstep[3] * B.x - 2. * rstep[2] * B.y);
    F[step][2][3] = cLqp0 * (rstep[3] * f2tmp * L2i + rstep[2] * B.x + B.z);
    F[step][2][4] = cL * f2tmp;

    const float f3tmp = -txty * B.y - rstep[2] * B.z + (1. + ty2) * B.x;
    f[step][3]        = cLqp0 * f3tmp;
    F[step][3][2]     = cLqp0 * (rstep[2] * f3tmp * L2i - rstep[3] * B.y - B.z);
    F[step][3][3]     = cLqp0 * (rstep[3] * f3tmp * L2i + 2. * rstep[3] * B.x - rstep[2] * B.y);
    F[step][3][4]     = cL * f3tmp;

    f[step][4] = 0.;

    //    if (fDoFitVelocity) {	//TODO: check and switch it on back if needed (for triplets, probably, not needed)
    ////      fvec vi       = rstep[6];
    f[step][5]     = rstep[6] * L;
    const float Li = 1.f / L;
    F[step][5][2]  = rstep[6] * rstep[2] * Li;  // / L;
    F[step][5][3]  = rstep[6] * rstep[3] * Li;  // / L;
    F[step][5][4]  = 0.;
    F[step][5][5]  = 0.;
    F[step][5][6]  = L;
    //    }
    //    else {
    //      const float vi       = xpu::sqrt(1. + fSettings[0].particleMass * fSettings[0].particleMass * qp0 * qp0) * fSettings[0].SpeedOfLightInv;
    //      f[step][5]    = rstep[6] * L;
    //      F[step][5][2] = rstep[6] * rstep[2] / L;
    //      F[step][5][3] = rstep[6] * rstep[3] / L;
    //      F[step][5][4] =
    //	  fSettings[0].particleMass * fSettings[0].particleMass * qp0 * L /
    //	  xpu::sqrt(1. + fSettings[0].particleMass * fSettings[0].particleMass * qp0 * qp0) * fSettings[0].SpeedOfLightInv;
    //      F[step][5][5] = 0.;
    //      F[step][5][6] = 0.;
    //    }

    f[step][6] = 0.;
  }  // end of Runge-Kutta step

  float r[7]    = {0.};    // extrapolated parameters
  float R[7][7] = {{0.}};  // Jacobian of the extrapolation

  float stepW[5];  // = {0., h / 6.f, h / 3.f, h / 3.f, h / 6.};
  stepW[0] = 0.;
  stepW[1] = h / 6.f;
  stepW[2] = h / 3.f;
  stepW[3] = h / 3.f;
  stepW[4] = h / 6.f;
  //  const float stepW[5] = {0., h * 0.16666667f, h * 0.33333334f, h * 0.33333334f, h * 0.16666667f};
  //  const float stepW[5] = {0., h * 0.166667f, h * 0.333334f, h * 0.333334f, h * 0.166667f};	//TODO: test constants: leads to a drop in efficiency

  float k[5][7][7] = {{0.}};
  #pragma unroll
  for (int step = 1; step <= 4; ++step) {
    #pragma unroll
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 7; j++) {
        k[step][i][j] = F[step][i][j];
        for (int m = 0; m < 7; m++) {
          k[step][i][j] += stepDz[step] * F[step][i][m] * k[step - 1][m][j];
        }
      }
    }
  }

  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      R[i][j] = R0[i][j];
      for (int step = 1; step <= 4; step++) {
        R[i][j] += stepW[step] * k[step][i][j];
      }
    }
  }

  const float dqp = tr_par->Qp() - qp0;

  for (int i = 0; i < 7; i++) {
    r[i] = r0[i];
    for (int step = 1; step <= 4; step++) {
      r[i] += stepW[step] * f[step][i];
    }
    // take into account linearisation at fQp0
    r[i] += R[i][4] * dqp;
  }

  // update parameters

  tr_par->X()    = r[0];
  tr_par->Y()    = r[1];
  tr_par->Tx()   = r[2];
  tr_par->Ty()   = r[3];
  tr_par->Qp()   = r[4];
  tr_par->Time() = r[5];
  tr_par->Vi()   = r[6];

  //fTr.Vi()( fTr.Vi() < fvec(TrackParamV::kClightNsInv) ) = fvec(TrackParamV::kClightNsInv);
  tr_par->Z() = z_out;  //zMasked;

  //          covariance matrix transport
  float C[7][7];
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      C[i][j] = tr_par->C(i, j);
    }
  }
  float RC[7][7];
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      RC[i][j] = 0.;
      for (int m = 0; m < 7; m++) {
        RC[i][j] += R[i][m] * C[m][j];
      }
    }
  }
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 7; j++) {
      float Cij = 0.f;
      for (int m = 0; m < 7; m++) {
        Cij += RC[i][m] * R[j][m];
      }
      tr_par->C(i, j) = Cij;
    }
  }
}

XPU_D void GpuTripletConstructor::ExtrapolateLineNoField(kf::TrackParamBase<float>* tr_par, float zOut) const
{
  // extrapolate the track assuming no field

  const float dz = zOut - tr_par->GetZ();

  const float tx = tr_par->GetTx();
  const float ty = tr_par->GetTy();
  const float vi = tr_par->GetVi();

  const float L = xpu::sqrt(1.f + tx * tx + ty * ty);

  const float j52 = dz * tx * vi / L;
  const float j53 = dz * ty * vi / L;
  const float j56 = dz * L;

  // transport parameters

  tr_par->X() += tx * dz;
  tr_par->Y() += ty * dz;
  tr_par->Time() += L * vi * dz;
  tr_par->Z() = zOut;

  // transport covariance matrix (see details in CaTrackFit)

  const float jc00 = tr_par->C(0, 0) + dz * tr_par->C(2, 0);
  const float jc02 = tr_par->C(0, 2) + dz * tr_par->C(2, 2);

  const float jc10 = tr_par->C(1, 0) + dz * tr_par->C(3, 0);
  const float jc11 = tr_par->C(1, 1) + dz * tr_par->C(3, 1);
  const float jc12 = tr_par->C(1, 2) + dz * tr_par->C(3, 2);
  const float jc13 = tr_par->C(1, 3) + dz * tr_par->C(3, 3);

  const float jc50 = tr_par->C(5, 0) + j52 * tr_par->C(2, 0) + j53 * tr_par->C(3, 0) + j56 * tr_par->C(6, 0);
  const float jc51 = tr_par->C(5, 1) + j52 * tr_par->C(2, 1) + j53 * tr_par->C(3, 1) + j56 * tr_par->C(6, 1);
  const float jc52 = tr_par->C(5, 2) + j52 * tr_par->C(2, 2) + j53 * tr_par->C(3, 2) + j56 * tr_par->C(6, 2);
  const float jc53 = tr_par->C(5, 3) + j52 * tr_par->C(2, 3) + j53 * tr_par->C(3, 3) + j56 * tr_par->C(6, 3);
  const float jc54 = tr_par->C(5, 4) + j52 * tr_par->C(2, 4) + j53 * tr_par->C(3, 4) + j56 * tr_par->C(6, 4);
  const float jc55 = tr_par->C(5, 5) + j52 * tr_par->C(2, 5) + j53 * tr_par->C(3, 5) + j56 * tr_par->C(6, 5);
  const float jc56 = tr_par->C(5, 6) + j52 * tr_par->C(2, 6) + j53 * tr_par->C(3, 6) + j56 * tr_par->C(6, 6);

  tr_par->C(0, 0) = jc00 + jc02 * dz;
  tr_par->C(1, 0) = jc10 + jc12 * dz;
  //  tr_par->C(2, 0) = tr_par->C(2, 0) + tr_par->C(2, 2) * dz;
  //  tr_par->C(3, 0) = tr_par->C(3, 0) + tr_par->C(3, 2) * dz;
  //  tr_par->C(4, 0) = tr_par->C(4, 0) + tr_par->C(4, 2) * dz;
  tr_par->C(2, 0) += tr_par->C(2, 2) * dz;
  tr_par->C(3, 0) += tr_par->C(3, 2) * dz;
  tr_par->C(4, 0) += tr_par->C(4, 2) * dz;
  tr_par->C(5, 0) = jc50 + jc52 * dz;
  //  tr_par->C(6, 0) = tr_par->C(6, 0) + tr_par->C(6, 2) * dz;
  tr_par->C(6, 0) += tr_par->C(6, 2) * dz;

  tr_par->C(1, 1) = jc11 + jc13 * dz;
  //  tr_par->C21() = tr_par->C21() + tr_par->C23() * dz;
  //  tr_par->C31() = tr_par->C31() + tr_par->C33() * dz;
  //  tr_par->C41() = tr_par->C41() + tr_par->C43() * dz;
  tr_par->C(2, 1) += tr_par->C(2, 3) * dz;
  tr_par->C(3, 1) += tr_par->C(3, 3) * dz;
  tr_par->C(4, 1) += tr_par->C(4, 3) * dz;
  tr_par->C(5, 1) = jc51 + jc53 * dz;
  //  tr_par->C61() = tr_par->C61() + tr_par->C63() * dz;
  tr_par->C(6, 1) += tr_par->C(6, 3) * dz;

  tr_par->C(5, 2) = jc52;
  tr_par->C(5, 3) = jc53;
  tr_par->C(5, 4) = jc54;
  tr_par->C(5, 5) = jc55 + jc52 * j52 + jc53 * j53 + jc56 * j56;
  //  tr_par->C65() = tr_par->C65() + tr_par->C62() * j52 + tr_par->C63() * j53 + tr_par->C66() * j56;
  tr_par->C(6, 5) += tr_par->C(6, 2) * j52 + tr_par->C(6, 3) * j53 + tr_par->C(6, 6) * j56;
}

XPU_D void GpuTripletConstructor::FilterXY(kf::TrackParamBase<float>* tr_par, const ca::MeasurementXy<float>& mxy) const
{
  {
    kf::MeasurementU<float> mx;
    mx.SetCosPhi(1.f);
    mx.SetSinPhi(0.f);
    mx.SetU(mxy.X());
    mx.SetDu2(mxy.Dx2());
    mx.SetNdf(mxy.NdfX());

    kf::MeasurementU<float> mu;
    mu.SetCosPhi(-mxy.Dxy() / mxy.Dx2());
    mu.SetSinPhi(1.f);
    mu.SetU(mu.CosPhi() * mxy.X() + mxy.Y());
    mu.SetDu2(mxy.Dy2() - mxy.Dxy() * mxy.Dxy() / mxy.Dx2());
    mu.SetNdf(mxy.NdfY());

    Filter1d(tr_par, mx);
    Filter1d(tr_par, mu);
    return;
  }
}

XPU_D void GpuTripletConstructor::Filter1d(kf::TrackParamBase<float>* tr_par, const kf::MeasurementU<float>& m) const
{
  float zeta, HCH;
  float F0, F1, F2, F3, F4, F5, F6;
  float K1, K2, K3, K4, K5, K6;

  zeta = m.CosPhi() * tr_par->X() + m.SinPhi() * tr_par->Y() - m.U();

  // F = CH'
  F0 = m.CosPhi() * tr_par->C(0, 0) + m.SinPhi() * tr_par->C(1, 0);
  F1 = m.CosPhi() * tr_par->C(1, 0) + m.SinPhi() * tr_par->C(1, 1);

  HCH = F0 * m.CosPhi() + F1 * m.SinPhi();

  F2 = m.CosPhi() * tr_par->C(2, 0) + m.SinPhi() * tr_par->C(2, 1);
  F3 = m.CosPhi() * tr_par->C(3, 0) + m.SinPhi() * tr_par->C(3, 1);
  F4 = m.CosPhi() * tr_par->C(4, 0) + m.SinPhi() * tr_par->C(4, 1);
  F5 = m.CosPhi() * tr_par->C(5, 0) + m.SinPhi() * tr_par->C(5, 1);
  F6 = m.CosPhi() * tr_par->C(6, 0) + m.SinPhi() * tr_par->C(6, 1);

  const bool maskDoFilter = (HCH < m.Du2() * 16.f);

  // correction to HCH is needed for the case when sigma2 is so small
  // with respect to HCH that it disappears due to the roundoff error
  //    fvec wi     = fMaskF / (m.Du2() + fvec(1.0000001) * HCH);
  //    fvec zetawi = fMaskF * zeta / (iif(maskDoFilter, m.Du2(), fvec::Zero()) + HCH);

  //TODO: (wi, zetawi): GPU results are different from CPU ones and depends on the order of the operations
  //TODO: need to compare effects made on the tracking efficiency and parameters quality
  float wi           = 1.f / (m.Du2() + 1.0000001f * HCH);
  const float zetawi = zeta / (maskDoFilter ? m.Du2() : 0.f + HCH);

  wi = m.Du2() > 0.f ? wi : 0.f;

  tr_par->ChiSq() += m.Ndf() * zeta * zeta * wi;
  tr_par->Ndf() += m.Ndf();

  K1 = F1 * wi;
  K2 = F2 * wi;
  K3 = F3 * wi;
  K4 = F4 * wi;
  K5 = F5 * wi;
  K6 = F6 * wi;

  tr_par->X() -= F0 * zetawi;
  tr_par->Y() -= F1 * zetawi;
  tr_par->Tx() -= F2 * zetawi;
  tr_par->Ty() -= F3 * zetawi;
  tr_par->Qp() -= F4 * zetawi;
  tr_par->Time() -= F5 * zetawi;
  tr_par->Vi() -= F6 * zetawi;

  tr_par->C(0, 0) -= F0 * F0 * wi;
  tr_par->C(1, 0) -= K1 * F0;
  tr_par->C(1, 1) -= K1 * F1;

  tr_par->C(2, 0) -= K2 * F0;
  tr_par->C(2, 1) -= K2 * F1;
  tr_par->C(2, 2) -= K2 * F2;

  tr_par->C(3, 0) -= K3 * F0;
  tr_par->C(3, 1) -= K3 * F1;
  tr_par->C(3, 2) -= K3 * F2;
  tr_par->C(3, 3) -= K3 * F3;

  tr_par->C(4, 0) -= K4 * F0;
  tr_par->C(4, 1) -= K4 * F1;
  tr_par->C(4, 2) -= K4 * F2;
  tr_par->C(4, 3) -= K4 * F3;
  tr_par->C(4, 4) -= K4 * F4;

  tr_par->C(5, 0) -= K5 * F0;
  tr_par->C(5, 1) -= K5 * F1;
  tr_par->C(5, 2) -= K5 * F2;
  tr_par->C(5, 3) -= K5 * F3;
  tr_par->C(5, 4) -= K5 * F4;
  tr_par->C(5, 5) -= K5 * F5;

  tr_par->C(6, 0) -= K6 * F0;
  tr_par->C(6, 1) -= K6 * F1;
  tr_par->C(6, 2) -= K6 * F2;
  tr_par->C(6, 3) -= K6 * F3;
  tr_par->C(6, 4) -= K6 * F4;
  tr_par->C(6, 5) -= K6 * F5;
  tr_par->C(6, 6) -= K6 * F6;
}

//TODO: update this function
XPU_D void GpuTripletConstructor::FilterTime(kf::TrackParamBase<float>* tr_par, float t, float dt2,
                                             float timeInfo) const
{
  // filter track with a time measurement

  // F = CH'
  const float F0 = tr_par->C(5, 0);
  const float F1 = tr_par->C(5, 1);
  const float F2 = tr_par->C(5, 2);
  const float F3 = tr_par->C(5, 3);
  const float F4 = tr_par->C(5, 4);
  const float F5 = tr_par->C(5, 5);
  const float F6 = tr_par->C(6, 5);

  const float HCH = F5;  //tr_par->C55();

  // when dt0 is much smaller than current time error,
  // set track time exactly to the measurement value without filtering
  // it helps to keep the initial time errors reasonably small
  // the calculations in the covariance matrix are not affected

  const bool maskDoFilter = (HCH < dt2 * 16.f) && timeInfo;

  float w = 1.f;

  if (timeInfo <= 0.f) w = 0.f;

  //  float wi = 1.f / (dt2 + 1.0000001f * HCH);
  float hch_d = 1.0000001f * HCH;
  float den   = dt2 + hch_d;
  float wi    = w / den;

  const float zeta   = tr_par->Time() - t;
  const float zetawi = w * zeta / (maskDoFilter ? dt2 : 0.f + HCH);

  tr_par->ChiSqTime() += maskDoFilter ? (zeta * zeta * wi) : 0.f;
  tr_par->NdfTime() += w;

  const float K1 = F1 * wi;
  const float K2 = F2 * wi;
  const float K3 = F3 * wi;
  const float K4 = F4 * wi;
  const float K5 = F5 * wi;
  const float K6 = F6 * wi;

  tr_par->X() -= F0 * zetawi;
  tr_par->Y() -= F1 * zetawi;
  tr_par->Tx() -= F2 * zetawi;
  tr_par->Ty() -= F3 * zetawi;
  tr_par->Qp() -= F4 * zetawi;
  tr_par->Time() -= F5 * zetawi;
  tr_par->Vi() -= F6 * zetawi;

  tr_par->C(0, 0) -= F0 * F0 * wi;

  tr_par->C(1, 0) -= K1 * F0;
  tr_par->C(1, 1) -= K1 * F1;

  tr_par->C(2, 0) -= K2 * F0;
  tr_par->C(2, 1) -= K2 * F1;
  tr_par->C(2, 2) -= K2 * F2;

  tr_par->C(3, 0) -= K3 * F0;
  tr_par->C(3, 1) -= K3 * F1;
  tr_par->C(3, 2) -= K3 * F2;
  tr_par->C(3, 3) -= K3 * F3;

  tr_par->C(4, 0) -= K4 * F0;
  tr_par->C(4, 1) -= K4 * F1;
  tr_par->C(4, 2) -= K4 * F2;
  tr_par->C(4, 3) -= K4 * F3;
  tr_par->C(4, 4) -= K4 * F4;

  tr_par->C(5, 0) -= K5 * F0;
  tr_par->C(5, 1) -= K5 * F1;
  tr_par->C(5, 2) -= K5 * F2;
  tr_par->C(5, 3) -= K5 * F3;
  tr_par->C(5, 4) -= K5 * F4;
  tr_par->C(5, 5) -= K5 * F5;

  tr_par->C(6, 0) -= K6 * F0;
  tr_par->C(6, 1) -= K6 * F1;
  tr_par->C(6, 2) -= K6 * F2;
  tr_par->C(6, 3) -= K6 * F3;
  tr_par->C(6, 4) -= K6 * F4;
  tr_par->C(6, 5) -= K6 * F5;
  tr_par->C(6, 6) -= K6 * F6;
}

XPU_D void GpuTripletConstructor::EnergyLossCorrection(kf::TrackParamBase<float>* tr_par, float radThick,
                                                       float upstreamDirection) const
{
  const float qp02 = xpu::max(tr_par->Qp() * tr_par->Qp(), 0.01f);
  const float p2   = 1.f / qp02;
  //  const float mass2 = fParams[fIterationData[0].fIteration].particleMass * fParams[fIterationData[0].fIteration].particleMass;	//TODO: fMass2 = 0.01116, fSettings[0].particleMass^2 = 0.011172, might lead to a different result
  const float mass2   = fParams_const[0].particleMass * fParams_const[0].particleMass;
  const float r_mass2 = 1.f / mass2;
  const float E2      = mass2 + p2;

  const float bg2 = p2 * r_mass2;
  //ApproximateBetheBloch
  const float kp0 = 2.33f;
  const float kp1 = 0.20f;
  const float kp2 = 3.00f;
  const float kp3 = 173e-9f;
  const float kp4 = 0.49848f;

  const float mK   = 0.307075e-3f;
  const float _2me = 1.022e-3f;
  const float rho  = kp0;
  const float x0   = kp1 * 2.303f;
  const float x1   = kp2 * 2.303f;
  const float mI   = kp3;
  const float mZA  = kp4;
  const float maxT = _2me * bg2;

  //*** Density effect
  float d2         = 0.f;
  const float x    = 0.5f * xpu::log(bg2);
  const float lhwI = xpu::log(28.816f * 1e-9f * xpu::sqrt(rho * mZA) / mI);

  if (x > x1) {
    d2 = lhwI + x - 0.5f;
  }
  if ((x > x0) && (x1 > x)) {
    const float r = (x1 - x) / (x1 - x0);
    d2            = lhwI + x - 0.5f + (0.5f - lhwI - x0) * r * r * r;
  }

  const float bethe =
    mK * mZA * (1.f + bg2) / bg2 * (0.5f * xpu::log(_2me * bg2 * maxT / (mI * mI)) - bg2 / (1.f + bg2) - d2);

  const float tr = xpu::sqrt(1.f + tr_par->Tx() * tr_par->Tx() + tr_par->Ty() * tr_par->Ty());

  const float dE = bethe * radThick * tr * 2.33f * 9.34961f;

  const float ECorrected  = xpu::sqrt(E2) + upstreamDirection * dE;
  const float E2Corrected = ECorrected * ECorrected;

  float corr = xpu::sqrt(p2 / (E2Corrected - mass2));
  if (xpu::isnan(corr) || xpu::isinf(corr)) {
    corr = 1.f;
  }

  tr_par->Qp() *= corr;
  tr_par->C(4, 0) *= corr;
  tr_par->C(4, 1) *= corr;
  tr_par->C(4, 2) *= corr;
  tr_par->C(4, 3) *= corr;
  tr_par->C(4, 4) *= corr * corr;
  tr_par->C(5, 4) *= corr;
}
