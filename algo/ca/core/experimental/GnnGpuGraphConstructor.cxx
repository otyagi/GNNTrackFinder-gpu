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

XPU_EXPORT(NearestNeighbours_FastPrim);
XPU_D void NearestNeighbours_FastPrim::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().NearestNeighbours_FastPrim(ctx);
}

XPU_EXPORT(NearestNeighbours_Other);
XPU_D void NearestNeighbours_Other::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().NearestNeighbours_Other(ctx);
}

XPU_EXPORT(MakeTripletsOT_FastPrim);
XPU_D void MakeTripletsOT_FastPrim::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().MakeTripletsOT_FastPrim(ctx);
}

XPU_EXPORT(MakeTripletsOT_Other);
XPU_D void MakeTripletsOT_Other::operator()(context& ctx, const int iteration)
{
  ctx.cmem<strGnnGpuGraphConstructor>().MakeTripletsOT_Other(ctx, iteration);
}

XPU_EXPORT(FitTripletsOT_FastPrim);
XPU_D void FitTripletsOT_FastPrim::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().FitTripletsOT_FastPrim(ctx);
}

XPU_EXPORT(FitTripletsOT_Other);
XPU_D void FitTripletsOT_Other::operator()(context& ctx)
{
  ctx.cmem<strGnnGpuGraphConstructor>().FitTripletsOT_Other(ctx);
}

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

XPU_EXPORT(Competition);
XPU_D void Competition::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().Competition(ctx); }

XPU_D void GnnGpuGraphConstructor::EmbedHits(EmbedHits::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fNHits) return;

  const auto& hitl = fvHits[iGThread];

  std::array<float, 3> input{hitl.X(), hitl.Y(), hitl.Z() + 44.0f};
  std::array<float, 6> result;
  EmbedSingleHit(input, result);

  fEmbedCoord[iGThread] = result;
}

XPU_D void GnnGpuGraphConstructor::NearestNeighbours_FastPrim(NearestNeighbours_FastPrim::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fNHits) return;

  const float margin = 2.0f;  // FastPrim
  auto& neighbours   = fDoublets_FastPrim[iGThread];
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
    const float dist = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[ihitm]);

    if (neighCount < kNN_FastPrim) {
      neighbours[neighCount++] = ihitm;
      if (dist > maxDist) {
        maxDist      = dist;
        maxDistIndex = neighCount - 1;
      }
    }
    else if (dist < maxDist) {  // replace hit max distance
      neighbours[maxDistIndex] = ihitm;
      maxDist                  = 0.0f;
      for (int i = 0; i < kNN_FastPrim; i++) {
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

XPU_D void GnnGpuGraphConstructor::NearestNeighbours_Other(NearestNeighbours_Other::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fNHits) return;

  float margin = 5.0f;
  if (fIteration == 1)
    margin = margin_allPrim;
  else if (fIteration == 3)
    margin = margin_allSec;

  auto& neighbours   = fDoublets_Other[iGThread];
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

  int iStaM = iStaL + 1;
  // Find closest hits (upto kNNOrder) which satisfy slope condition
  ca::HitIndex_t iHitStart = fIndexFirstHitStation[iStaM];      // start index
  ca::HitIndex_t iHitEnd   = fIndexFirstHitStation[iStaM + 1];  // end index
  for (std::size_t ihitm = iHitStart; ihitm < iHitEnd; ihitm++) {
    const auto& hitm = fvHits[ihitm];
    // margin
    const float y_m   = hitm.Y();
    const float z_m   = hitm.Z() + 44.0f;
    const float slope = (y_m - y_l) / (z_m - z_l);
    if (xpu::abs(y_l - slope * z_l) > margin) continue;
    const float dist = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[ihitm]);
    if (neighCount < 25) {
      neighbours[neighCount++] = ihitm;
      if (dist > maxDist) {
        maxDist      = dist;
        maxDistIndex = neighCount - 1;
      }
    }
    else if (dist < maxDist) {  // replace hit max distance
      neighbours[maxDistIndex] = ihitm;
      maxDist                  = 0.0f;
      for (int i = 0; i < 25; i++) {
        const float dist_re = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[neighbours[i]]);
        if (dist_re > maxDist) {
          maxDist      = dist_re;
          maxDistIndex = i;
        }
      }
    }
  }  // hits on iStaM

  if (iStaL > 9) {
    fNNeighbours[iGThread] = neighCount;
    return;
  }
  // Doublets with one station skipped
  maxDist      = 0.0f;
  maxDistIndex = 25;
  iStaM        = iStaL + 2;
  // Find closest hits (upto kNNOrder_Jump) which satisfy slope condition
  iHitStart = fIndexFirstHitStation[iStaM];      // start index
  iHitEnd   = fIndexFirstHitStation[iStaM + 1];  // end index
  for (std::size_t ihitm = iHitStart; ihitm < iHitEnd; ihitm++) {
    const auto& hitm  = fvHits[ihitm];
    const float y_m   = hitm.Y();
    const float z_m   = hitm.Z() + 44.0f;
    const float slope = (y_m - y_l) / (z_m - z_l);
    if (xpu::abs(y_l - slope * z_l) > margin) continue;

    const float dist = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[ihitm]);
    if (neighCount < 35) {
      neighbours[neighCount++] = ihitm;
      if (dist > maxDist) {
        maxDist      = dist;
        maxDistIndex = neighCount - 1;
      }
    }
    else if (dist < maxDist) {  // replace hit max distance
      neighbours[maxDistIndex] = ihitm;
      maxDist                  = 0.0f;
      for (int i = 25; i < 35; i++) {
        const float dist_re = hitDistanceSq(fEmbedCoord[iGThread], fEmbedCoord[neighbours[i]]);
        if (dist_re > maxDist) {
          maxDist      = dist_re;
          maxDistIndex = i;
        }
      }
    }
  }  // hits on iStaM


  fNNeighbours[iGThread] = neighCount;

  // printf("iGThread: %d, fNNeighbours: %d \n", iGThread, fNNeighbours[iGThread]);

}  // NearestNeighbours_Other

XPU_D void GnnGpuGraphConstructor::MakeTripletsOT_FastPrim(MakeTripletsOT_FastPrim::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fNHits) return;

  unsigned int tripletCount = 0;
  const float YZCut         = 0.1;  // (radians) def - 0.1 from distributions
  const float XZCut         = 0.1;  // def - 0.1 from distributions

  auto& tripletsLHit       = fTriplets_FastPrim[iGThread];
  const auto& doubletsLHit = fDoublets_FastPrim[iGThread];
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
          const unsigned int iHitR = fDoublets_FastPrim[iHitM][iDoubletM];
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
  // printf ("iGThread: %d, fNTriplets: %d", iGThread, fNTriplets[iGThread]);
}

XPU_D void GnnGpuGraphConstructor::MakeTripletsOT_Other(MakeTripletsOT_Other::context& ctx, const int iteration) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= fNHits) return;

  // printf ("iGThread: %d \t", iGThread);

  unsigned int tripletCount = 0;

  float YZCut_Cons = 0.4;  // (radians) def - 0.4 from distributions
  float XZCut_Cons = 0.8;  // def - 0.8 from distributions

  float YZCut_Jump     = 0.2;   // def - 0.2 - AllPrim
  float XZCut_Jump     = 0.4;   // def - 0.4 - AllPrim
  float jump_margin_yz = 0.5f;  // def - 0.5 - AllPrim

  if (iteration == 3) {
    YZCut_Jump     = 0.1;    // def - 0.2 - AllSec
    XZCut_Jump     = 0.2;    // def - 0.4 - AllSec
    jump_margin_yz = 10.0f;  // def - 0.5 - AllSec
  }

  auto& tripletsLHit       = fTriplets_Other[iGThread];
  const auto& doubletsLHit = fDoublets_Other[iGThread];
  const int nLHitDoublets  = fNNeighbours[iGThread];
  const auto& hitl         = fvHits[iGThread];
  const float x_l          = hitl.X();
  const float y_l          = hitl.Y();
  const float z_l          = hitl.Z();
  const int sta1           = hitl.Station();
  if (sta1 > 9) {
    fNTriplets[iGThread] = tripletCount;
    return;
  }

  for (int iDoubletL = 0; iDoubletL < nLHitDoublets; iDoubletL++) {
    const unsigned int iHitM = doubletsLHit[iDoubletL];
    const int sta2           = fvHits[iHitM].Station();
    if (sta2 > 10) continue;
    const ca::HitIndex_t iHitStartM = fIndexFirstHitStation[sta2];      // start index middle station
    const ca::HitIndex_t iHitEndM   = fIndexFirstHitStation[sta2 + 1];  // end index
    if ((sta2 - sta1) == 1) {                                           // triplet type : [1 2 3/4]
      for (auto iM = iHitStartM; iM < iHitEndM; iM++) {
        if (iHitM == iM) {
          const auto& hitm        = fvHits[iHitM];
          const float x_m         = hitm.X();
          const float y_m         = hitm.Y();
          const float z_m         = hitm.Z();
          const int nMHitDoublets = fNNeighbours[iHitM];
          for (int iDoubletM = 0; iDoubletM < nMHitDoublets; iDoubletM++) {
            const unsigned int iHitR = fDoublets_Other[iHitM][iDoubletM];
            const auto& hitr         = fvHits[iHitR];
            const int sta3           = hitr.Station();
            if (sta3 > 11) continue;
            const float x_r = hitr.X();
            const float y_r = hitr.Y();
            const float z_r = hitr.Z();

            if ((sta3 - sta2) == 1) {  // triplet type : [1 2 3]
              // YZ
              const float angle1YZ    = xpu::atan2(y_m - y_l, z_m - z_l);
              const float angle2YZ    = xpu::atan2(y_r - y_m, z_r - z_m);
              const float angleDiffYZ = angle1YZ - angle2YZ;
              if (angleDiffYZ < -YZCut_Cons || angleDiffYZ > YZCut_Cons) continue;

              // XZ
              const float angle1XZ    = xpu::atan2(x_m - x_l, z_m - z_l);
              const float angle2XZ    = xpu::atan2(x_r - x_m, z_r - z_m);
              const float angleDiffXZ = angle1XZ - angle2XZ;
              if (angleDiffXZ < -XZCut_Cons || angleDiffXZ > XZCut_Cons) continue;

              tripletsLHit[tripletCount++] = std::array<unsigned int, 2>{iHitM, iHitR};
            }
            else if ((sta3 - sta2) == 2) {  // triplet type : [1 2 4]
              // jump triplet has additional constraint
              float y1    = hitl.Y();
              float z1    = hitl.Z() + 44.0f;
              float y3    = hitr.Y();
              float z3    = hitr.Z() + 44.0f;
              float slope = (y3 - y1) / (z3 - z1);
              if (std::abs(y1 - slope * z1) > jump_margin_yz) continue;

              // YZ
              const float angle1YZ    = xpu::atan2(y_m - y_l, z_m - z_l);
              const float angle2YZ    = xpu::atan2(y_r - y_m, z_r - z_m);
              const float angleDiffYZ = angle1YZ - angle2YZ;
              if (angleDiffYZ < -YZCut_Cons || angleDiffYZ > YZCut_Cons) continue;

              // XZ
              const float angle1XZ    = xpu::atan2(x_m - x_l, z_m - z_l);
              const float angle2XZ    = xpu::atan2(x_r - x_m, z_r - z_m);
              const float angleDiffXZ = angle1XZ - angle2XZ;
              if (angleDiffXZ < -XZCut_Cons || angleDiffXZ > XZCut_Cons) continue;

              tripletsLHit[tripletCount++] = std::array<unsigned int, 2>{iHitM, iHitR};
            }
          }
          break;  // only one match possible
        }
      }  // iHitM
    }
    else if ((sta2 - sta1) == 2) {  // triplet type : [1 3 4]
      for (auto iM = iHitStartM; iM < iHitEndM; iM++) {
        if (iHitM == iM) {
          const auto& hitm        = fvHits[iHitM];
          const float x_m         = hitm.X();
          const float y_m         = hitm.Y();
          const float z_m         = hitm.Z();
          const int nMHitDoublets = fNNeighbours[iHitM];
          for (int iDoubletM = 0; iDoubletM < nMHitDoublets; iDoubletM++) {
            const unsigned int iHitR = fDoublets_Other[iHitM][iDoubletM];
            const auto& hitr         = fvHits[iHitR];
            const int sta3           = hitr.Station();
            if (sta3 > 11) continue;
            const float x_r = hitr.X();
            const float y_r = hitr.Y();
            const float z_r = hitr.Z();

            if ((sta3 - sta2) == 1) {  // triplet type : [1 2 3]
              // jump triplet has additional constraint
              float y1    = hitl.Y();
              float z1    = hitl.Z() + 44.0f;
              float y3    = hitr.Y();
              float z3    = hitr.Z() + 44.0f;
              float slope = (y3 - y1) / (z3 - z1);
              if (std::abs(y1 - slope * z1) > jump_margin_yz) continue;

              // YZ
              const float angle1YZ    = xpu::atan2(y_m - y_l, z_m - z_l);
              const float angle2YZ    = xpu::atan2(y_r - y_m, z_r - z_m);
              const float angleDiffYZ = angle1YZ - angle2YZ;
              if (angleDiffYZ < -YZCut_Cons || angleDiffYZ > YZCut_Cons) continue;

              // XZ
              const float angle1XZ    = xpu::atan2(x_m - x_l, z_m - z_l);
              const float angle2XZ    = xpu::atan2(x_r - x_m, z_r - z_m);
              const float angleDiffXZ = angle1XZ - angle2XZ;
              if (angleDiffXZ < -XZCut_Cons || angleDiffXZ > XZCut_Cons) continue;

              tripletsLHit[tripletCount++] = std::array<unsigned int, 2>{iHitM, iHitR};
            }
          }
          break;  // only one match possible
        }
      }  // iHitM
    }
  }
  fNTriplets[iGThread] = tripletCount;

  // printf("iGThread: %d, fNTriplets: %d \n", iGThread, fNTriplets[iGThread]);
}

XPU_D void GnnGpuGraphConstructor::FitTripletsOT_FastPrim(FitTripletsOT_FastPrim::context& ctx) const
{
  const int iGThread       = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  const int NMaxTripletHit = kNN_FastPrim * kNN_FastPrim;
  if (iGThread >= fNHits * NMaxTripletHit) return;

  const unsigned int iHitL = iGThread / NMaxTripletHit;
  if (iHitL >= fNHits) return;
  const int lSta = fvHits[iHitL].Station();
  if (lSta > 9) return;
  const unsigned int nTripletsHitL = fNTriplets[iHitL];
  const int iTriplet               = iGThread % NMaxTripletHit;
  if (iTriplet >= NMaxTripletHit) return;
  if (nTripletsHitL > NMaxTripletHit || nTripletsHitL == 0) return;
  if (iTriplet >= (int) nTripletsHitL) return;

  const std::array<unsigned int, 3> triplet = {iHitL, fTriplets_FastPrim[iHitL][iTriplet][0],
                                               fTriplets_FastPrim[iHitL][iTriplet][1]};

  for (int i = 0; i < 3; i++) {
    if (triplet[i] >= fNHits) return;
  }

  const ca::GpuStation* sta[12];
  for (int is = 0; is < 12; ++is) {
    sta[is] = &fStations_const[is];
  };

  kf::GpuTrackKalmanFilter<float> fit;
  TrackParam<float>& tr = fit.Tr();
  fit.SetParticleMass(constants::phys::MuonMass);
  fit.SetDoFitVelocity(true);

  // ca::Track t;
  //
  kf::TrackParamS empty;  // to reset unused entries
  // t.fParFirst.SetOneGpu(empty);
  // t.fParLast.SetOneGpu(empty);
  //

  const int nHitsTrack = 3;
  if (nHitsTrack <= 0) return;
  // t.fNofHits = nHitsTrack;

  // ------------------------------
  float hx[constants::size::MaxNstations];
  float hy[constants::size::MaxNstations];
  float hz[constants::size::MaxNstations];
  float htime[constants::size::MaxNstations];
  float hdt2[constants::size::MaxNstations];
  ca::MeasurementXy<float> hmxy[constants::size::MaxNstations];
  bool h_has_time[constants::size::MaxNstations];
  ca::GpuFieldValue hFB[constants::size::MaxNstations];
  float hBy[constants::size::MaxNstations];
  int hSta[constants::size::MaxNstations];

  // Initialize
  for (int ih = 0; ih < nHitsTrack; ++ih) {
    const ca::Hit& hit = fvHits[triplet[ih]];
    const int ista     = hit.Station();
    hSta[ih]           = ista;

    auto detSystemId = sta[ista]->GetDetectorID();
    // subtract misalignment tolerances to get the original hit errors
    float dX2Orig = hit.dX2();  //- fParams[fIteration].GetMisalignmentXsq(detSystemId);
    float dY2Orig = hit.dY2();  // - fParams[fIteration].GetMisalignmentYsq(detSystemId);
    float dXYOrig = hit.dXY();
    float dT2Orig = hit.dT2();  // - fParams[fIteration].GetMisalignmentTsq(detSystemId);

    hx[ih]          = hit.X();
    hy[ih]          = hit.Y();
    hz[ih]          = hit.Z();
    htime[ih]       = hit.T();
    h_has_time[ih]  = sta[ista]->timeInfo;
    hdt2[ih]        = h_has_time[ih] ? dT2Orig : 1.e4f;  // big default uncertainty for no-time stations
    hmxy[ih].X()    = hx[ih];
    hmxy[ih].Y()    = hy[ih];
    hmxy[ih].Dx2()  = dX2Orig;
    hmxy[ih].Dy2()  = dY2Orig;
    hmxy[ih].Dxy()  = dXYOrig;
    hmxy[ih].NdfX() = 1.f;
    hmxy[ih].NdfY() = 1.f;

    hFB[ih] = sta[ista]->fieldSlice.GetFieldValue(hx[ih], hy[ih]);
    // TODO: test both ways of getting By and compare pulls
    //    hBy[ih] = sta[ista].fieldSlice.GetFieldValue(0., 0.).GetBy();	// Standartd way
    hBy[ih] = hFB[ih].GetBy();  // More accurate for better guess
  }

  // first and last hit indices
  const int ih_first = 0;
  const int ih_last  = nHitsTrack - 1;

  // --------------------------------
  // INITIAL GUESS
  // --------------------------------
  // GuessTrack now works with hits, not stations
  fit.GuessTrack(hz[ih_last], hx, hy, hz, htime, hBy, nHitsTrack);

  // TODO: properly implement tracking modes for GPU
  //  if (ca::TrackingMode::kGlobal == fIterationData[0].fTrackingMode ||
  //      ca::TrackingMode::kMcbm  == fIterationData[0].fTrackingMode) {
  //    tr.Qp() = 1.f / 1.1f;
  //  }

  // =======================================================
  // 1.5 ITERATIONS
  // =======================================================
  for (int iter = 0; iter < 2; ++iter) {
    fit.SetQp0(tr.Qp());

    // --------------------------
    // BACKWARD
    // --------------------------
    {
      // Initialize at last hit
      const int ih         = ih_last;
      const float dt2_last = xpu::max(hdt2[ih], 1.e-4f);  // FIX: clamp
      tr.ResetErrors(hmxy[ih].Dx2(), hmxy[ih].Dy2(), 0.1f, 0.1f, 1.f, dt2_last, 1.e-2f);
      tr.C10()  = hmxy[ih].Dxy();
      tr.X()    = hmxy[ih].X();
      tr.Y()    = hmxy[ih].Y();
      tr.Time() = h_has_time[ih] ? htime[ih] : 0.f;
      tr.Vi()   = constants::phys::SpeedOfLightInv;
      tr.InitVelocityRange(0.5f);
      tr.Ndf()     = -3.f;
      tr.NdfTime() = -2.f + (h_has_time[ih] ? 1.f : 0.f);

      // Field initialization
      float fldZ0 = hz[ih], fldZ1 = hz[ih], fldZ2 = hz[ih];
      ca::GpuFieldValue fldB0 = hFB[ih], fldB1 = hFB[ih], fldB2 = hFB[ih];
      ca::GpuFieldRegion fld;

      const auto& hit = fvHits[triplet[ih_last]];
      const int ista  = hit.Station();
      fldZ1           = sta[ista]->fZ;                   // use station Z to avoid hit misalignment effects
      fldZ2           = sta[xpu::max(0, ista - 2)]->fZ;  // use station Z to avoid hit misalignment effects
      fldB2           = hFB[ih_last - 2];

      for (int jh = ih - 1; jh >= ih_first; --jh) {
        fldZ0 = hz[jh];
        fldB0 = hFB[jh];
        fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

        fit.SetMask(true);  // only actual hits
        fit.Extrapolate(hz[jh], fld);

        // Material effects
        const int ista = hSta[jh];
        const int bin  = fMaterialMap[ista].GetBin(tr.X(), tr.Y());
        if (bin >= 0) {
          const auto radThick = fMaterialMapTables[bin];
          // const auto radThick = fMaterialMapTables[fMaterialMap[ista].GetBin(tr.X(), tr.Y())];
          // const auto radThick = fMaterialMapTest[ista].GetThicknessX0(tr.GetX(), tr.GetY());
          fit.MultipleScattering(radThick);
          fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);
        }

        // Filtering
        fit.FilterXY(hmxy[jh]);
        if (h_has_time[jh]) fit.FilterTime(htime[jh], hdt2[jh], true);

        // field window shift
        fldB2 = fldB1;
        fldZ2 = fldZ1;
        fldB1 = fldB0;
        fldZ1 = fldZ0;
      }
    }

    // --------------------------
    // Extrapolate to PV
    // --------------------------
    // kf::GpuTrackKalmanFilter fitpv = fit;
    // {
    //   fitpv.SetMask(true);
    //   ca::MeasurementXy<float> vtxInfo = cParams.targetMeasurement;
    //   vtxInfo.SetDx2(1.e-8f);
    //   vtxInfo.SetDxy(0.f);
    //   vtxInfo.SetDy2(1.e-8f);

    //   // Ideally, use global field here; for now use the last used field window 'fld'.
    //   // If global field is available, replace with fldFull.
    //   ca::GpuFieldRegion fld;  // default constructor
    //   fld.Set(hFB[ih_last], hz[ih_last], hFB[ih_last], hz[ih_last], hFB[ih_last], hz[ih_last]);
    //   fitpv.Extrapolate(cParams.GetTargetPositionZ(), fld);
    //   // fitpv.FilterXY(vtxInfo);
    // }

    // t.fParFirst.SetOneGpu(fit.Tr());
    // t.fParPV.SetOneGpu(fitpv.Tr());

    if (iter == 1) break;  // 1.5 iterations only

    // --------------------------
    // FORWARD
    // --------------------------
    {
      const int ih          = ih_first;
      const float dt2_first = xpu::max(hdt2[ih], 1.e-4f);
      tr.ResetErrors(hmxy[ih].Dx2(), hmxy[ih].Dy2(), 0.1f, 0.1f, 1.f, dt2_first, 1.e-2f);
      tr.C10()  = hmxy[ih].Dxy();
      tr.X()    = hmxy[ih].X();
      tr.Y()    = hmxy[ih].Y();
      tr.Time() = h_has_time[ih] ? htime[ih] : 0.f;
      tr.Vi()   = constants::phys::SpeedOfLightInv;
      tr.InitVelocityRange(0.5f);
      tr.Ndf()     = -3.f;
      tr.NdfTime() = -2.f + (h_has_time[ih] ? 1.f : 0.f);

      fit.SetQp0(tr.Qp());

      float fldZ0 = hz[ih], fldZ1 = hz[ih], fldZ2 = hz[ih];
      ca::GpuFieldValue fldB0 = hFB[ih], fldB1 = hFB[ih], fldB2 = hFB[ih];
      ca::GpuFieldRegion fld;

      const auto& hit = fvHits[triplet[ih_first]];
      const int ista  = hit.Station();
      fldZ1           = hz[ih];      //sta[ista].fZ; // use station Z to avoid hit misalignment effects
      fldZ2           = hz[ih + 2];  //sta[xpu::min(0, ista + 2)].fZ; // use station Z to avoid hit misalignment effects
      fldB2           = hFB[ih_first + 2];

      for (int jh = ih + 1; jh <= ih_last; ++jh) {
        const int jh2 = xpu::min(ih_last, jh + 2);

        fldZ0 = hz[jh];
        fldB0 = hFB[jh];
        fld.Set(fldB0, fldZ0, fldB1, fldZ1, fldB2, fldZ2);

        fit.SetMask(true);
        fit.Extrapolate(hz[jh], fld);

        const int ista = hSta[jh];
        const int bin  = fMaterialMap[ista].GetBin(tr.X(), tr.Y());
        if (bin >= 0) {
          const auto radThick = fMaterialMapTables[bin];
          // const auto radThick = fMaterialMapTest[ista].GetThicknessX0(tr.X(), tr.Y());
          fit.MultipleScattering(radThick);
          fit.EnergyLossCorrection(radThick, kf::FitDirection::kDownstream);
        }

        fit.FilterXY(hmxy[jh]);
        if (h_has_time[jh]) fit.FilterTime(htime[jh], hdt2[jh], true);

        fldB2 = fldB1;
        fldZ2 = fldZ1;
        fldB1 = fldB0;
        fldZ1 = fldZ0;
      }
    }

    // t.fParLast.SetOneGpu(fit.Tr());
  }  // iter

  /// if track chi2 per dof is larger than threshold. Also kill negative and non-finite values
  /// if track p low than threshold_qp, then kill the track
  /// then remove triplet from list
  const float threshold_chi2 = 19.5;  // def - 19.5
  const float threshold_qp   = 5.0f;  // def - 5.0f

  const float chi2 = fit.Tr().GetChiSq();
  bool killTrack   = !xpu::isfinite(chi2) || (chi2 < 0) || (chi2 > threshold_chi2);

  // momentum cut to reduce ghosts
  if (xpu::abs(fit.Tr().Qp()) > threshold_qp) {
    killTrack = true;
  }

  // printf("iGThread: %d, %d, %d, %d, chi2= %.4f, q/p= %.4f, killed= %d \n", iGThread, triplet[0], triplet[1], triplet[2],
  //        chi2, fit.Tr().Qp(), killTrack);

  const float qp  = fit.Tr().Qp();
  const float Cqp = fit.Tr().C44() + 0.001;  // 0.001 magic number added. (see triplet constructor)
  const float Tx  = fit.Tr().Tx();
  const float C22 = fit.Tr().C22();
  const float Ty  = fit.Tr().Ty();
  const float C33 = fit.Tr().C33();
  const std::array<float, 7> tripletParams{chi2, qp, Cqp, Tx, C22, Ty, C33};
  fvTripletParams_FastPrim[iHitL][iTriplet]   = tripletParams;
  fTripletsSelected_FastPrim[iHitL][iTriplet] = !killTrack;
}  // FitTripletsOT_FastPrim

XPU_D void GnnGpuGraphConstructor::FitTripletsOT_Other(FitTripletsOT_Other::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  // printf("iGThread: %d\n", iGThread);

  const int NMaxTripletHit = kNN_Other * kNN_Other;
  if (iGThread >= fNHits * NMaxTripletHit) return;

  const unsigned int iHitL = iGThread / NMaxTripletHit;
  if (iHitL >= fNHits) return;
  const int lSta = fvHits[iHitL].Station();
  if (lSta > 9) return;
  const unsigned int nTripletsHitL = fNTriplets[iHitL];
  const int iTriplet               = iGThread % NMaxTripletHit;
  if (iTriplet >= NMaxTripletHit) return;
  if (nTripletsHitL > NMaxTripletHit || nTripletsHitL == 0) return;
  if (iTriplet >= (int) nTripletsHitL) return;

  const std::array<unsigned int, 3> triplet = {iHitL, fTriplets_Other[iHitL][iTriplet][0],
                                               fTriplets_Other[iHitL][iTriplet][1]};

  for (int i = 0; i < 3; i++) {
    if (triplet[i] >= fNHits) return;
  }
  // printf("iGThread: %d, iHitL: %d, iTriplet: %d \n", iGThread, iHitL, iTriplet);

  const int nStations = 12;

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

  fit.GuessTrack(z_end, x, y, z, time, By, nStations);

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
    kf::GpuTrackKalmanFilter<float> fitpv = fit;
    {
      fitpv.SetMask(true);
      ca::MeasurementXy<float> vtxInfo = fParams_const[fIteration].targetMeasurement;
      vtxInfo.SetDx2(1.e-8);
      vtxInfo.SetDxy(0.);
      vtxInfo.SetDy2(1.e-8);

      ca::GpuFieldRegion fldTarget;
      ca::GpuFieldValue B[3];

      float tx[3] = {(x[1] - x[0]) / (z[1] - z[0]), (x[2] - x[0]) / (z[2] - z[0]), (x[2] - x[1]) / (z[2] - z[1])};
      float ty[3] = {(y[1] - y[0]) / (z[1] - z[0]), (y[2] - y[0]) / (z[2] - z[0]), (y[2] - y[1]) / (z[2] - z[1])};
      for (int ih = 0; ih < 3; ++ih) {
        float dz = (sta[ih]->fZ - z[ih]);
        B[ih]    = sta[ih]->fieldSlice.GetFieldValue(x[ih] + tx[ih] * dz, y[ih] + ty[ih] * dz);
      };

      fld.Set(B[0], sta[0]->fZ, B[1], sta[1]->fZ, B[2], sta[2]->fZ);
      fldTarget.Set(fParams[fIteration].targB, fParams[fIteration].GetTargetPositionZ(), B[0], sta[0]->fZ, B[1],
                    sta[1]->fZ);

      // Ideally, use global field here; for now use the last used field window 'fld'.
      // If global field is available, replace with fldFull.
      // ca::GpuFieldRegion fld;  // default constructor
      // fld.Set(hFB[ih_last], hz[ih_last], hFB[ih_last], hz[ih_last], hFB[ih_last], hz[ih_last]);
      fitpv.Extrapolate(fParams_const[fIteration].GetTargetPositionZ(), fld);
      // fitpv.FilterXY(vtxInfo);
    }

    // OT : Use fitPV to determine if primary track
    if (iter == 1 and fIteration == 1) {  // use iter 1 of KF fit for better fit, for all primary iteration
      const auto pv_x = fitpv.Tr().X();   // in cm
      const auto pv_y = fitpv.Tr().Y();
      const auto pv_z = fitpv.Tr().Z();
      if (xpu::isnan(pv_x) || xpu::isnan(pv_y) || xpu::isnan(pv_z) || (xpu::abs(pv_z + 44.0f) > 0.1)) {
        isPrimary = 1.0;  // just leave alone. Some of these are apparently useful
        continue;
      }
      const float dist = xpu::sqrt(pv_x * pv_x + pv_y * pv_y);
      if (dist > 1.0f) continue;  // 1 cm radius
      isPrimary = 1.0;
    }

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
  float threshold_chi2 = 50.0f;
  float threshold_qp   = 10.0f;

  if (fIteration == 1) {     // All primary
    threshold_chi2 = 5.0f;   // def - 5
    threshold_qp   = 10.0f;  // def - 10
  }
  else if (fIteration == 3) {  // Secondary
    threshold_chi2 = 50.0f;    // def - 10.0f
    threshold_qp   = 10.0f;    // def - 10.0f
  }

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

  // printf("iGThread: %d, %d, %d, %d, chi2= %.4f, q/p= %.4f, killed= %d \n", iGThread, triplet[0], triplet[1],
  //  triplet[2], chi2, fit.Tr().Qp(), killTrack);

  const float qp  = fit.Tr().Qp();
  const float Cqp = fit.Tr().C44() + 0.001;  // 0.001 magic number added. (see triplet constructor)
  const float Tx  = fit.Tr().Tx();
  const float C22 = fit.Tr().C22();
  const float Ty  = fit.Tr().Ty();
  const float C33 = fit.Tr().C33();
  const std::array<float, 7> tripletParams{chi2, qp, Cqp, Tx, C22, Ty, C33};
  fvTripletParams_Other[iHitL][iTriplet]   = tripletParams;
  fTripletsSelected_Other[iHitL][iTriplet] = !killTrack;
}  // FitTripletsOT_Other

XPU_D void GnnGpuGraphConstructor::Competition(Competition::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  if (iGThread >= 1) return;

  printf("In GPU competition\n");
  printf("Num tracks: %d", fNTracks);
  printf("back key hit 0: %d", fvHitsAll[0].BackKey());

  for (int i = 0; i < 5; i++) {
    printf("Track %d \n", i);
    // printf("nhits: %d \n", fTrackNumHits[i]);
    // printf("chi2: %d \n", fTrackAndScores[i].second);
    printf("selected: %d \n", fSelectedTrackIndexes[i]);
    printf("hit0: %d \n", fTrackAndScores[i].first[0]);
    printf("hit1: %d \n", fTrackAndScores[i].first[1]);
    printf("hit2: %d \n", fTrackAndScores[i].first[2]);
    // printf("Track %d: chi2= %d, nHits= %d\n", i, fTrackAndScores[i].second, fTrackNumHits[i]);
    // for (int iHit = 0; iHit < 12; iHit++)
    //   printf("%d ", fTrackAndScores[i].first[iHit]);
    // printf("\n");
  }

  // for (std::size_t iTrack = 0; iTrack < fNTracks; iTrack++) {
  //   // check that all hits are not used
  //   auto& track    = fTrackAndScores[iTrack].first;
  //   int selected   = 1;
  //   uint nUsedHits = 0;
  //   std::array<int, 12> usedHitIDs;
  //   std::array<int, 12> usedHitIndexesInTrack;
  //   for (int i = 0; i < 12; i++) {
  //     usedHitIDs[i]            = -1;
  //     usedHitIndexesInTrack[i] = -1;
  //   }
  //   for (std::size_t iHit = 0; iHit < track.size(); iHit++) {
  //     if (track[iHit] == -1) break;  // end of track
  //     const ca::Hit& hit = fvHitsAll[track[iHit]];
  //     if (fHitKeyFlags[hit.FrontKey()] || fHitKeyFlags[hit.BackKey()]) {
  //       usedHitIDs[nUsedHits]            = track[iHit];
  //       usedHitIndexesInTrack[nUsedHits] = iHit;
  //       nUsedHits++;
  //     }
  //   }
  //   if (nUsedHits == 0) {  // clean tracks. Mark all hits as used
  //     for (const auto& hit : track) {
  //       if (hit == -1) break;
  //       fHitKeyFlags[fvHitsAll[hit].FrontKey()] = 1;
  //       fHitKeyFlags[fvHitsAll[hit].BackKey()]  = 1;
  //     }
  //     continue;
  //   }
  //   else if (nUsedHits > 0) {  // some hits used but still >=4 hits left
  //     if (track.size() - nUsedHits >= 4) {
  //       // remove used hits.
  //       for (int iUsedHit = 0; iUsedHit < usedHitIndexesInTrack.size(); iUsedHit++) {
  //         if (usedHitIndexesInTrack[iUsedHit] != -1) track[iUsedHit] = -1;
  //       }

  //       // mark remaining hits as used
  //       for (const auto& hit : track) {
  //         if (hit == -1) continue;
  //         fHitKeyFlags[fvHitsAll[hit].FrontKey()] = 1;
  //         fHitKeyFlags[fvHitsAll[hit].BackKey()]  = 1;
  //       }
  //       continue;
  //     }
  //     else {
  //       selected = 0;  // remove track if begging not successful
  //     }
  //   }

  //   if ((selected == 0)
  //       && (track.size() - nUsedHits == 3)) {  // 'beg' for hit from longer accepted track only if one hit required
  //     for (std::size_t iBeg = 0; iBeg < iTrack; iBeg++) {  // track to beg from
  //       if (fTrackAndScores[iBeg].first.size() <= fTrackAndScores[iTrack].first.size())
  //         continue;                                         // only beg from longer tracks
  //       if (fTrackAndScores[iBeg].first.size() < 5) break;  // atleast 4 hits must be left after donation
  //       if (fTrackAndScores[iBeg].second < fTrackAndScores[iTrack].second)
  //         continue;                // dont donate to higher chi2 beggar
  //       if (selected == 1) break;  // no need for more begging

  //       auto& begTrack = fTrackAndScores[iBeg].first;
  //       for (std::size_t iBegHit = 0; iBegHit < begTrack.size(); iBegHit++) {
  //         const auto begHit = begTrack[iBegHit];
  //         if (begHit == -1) continue;
  //         if (begHit == usedHitIDs[0])
  //           continue;  // dont let exact hit be borrowed. Mostly tracks with one hit missing beg
  //         if (fvHitsAll[begHit].FrontKey() == fvHitsAll[usedHitIDs[0]].FrontKey()
  //             || fvHitsAll[begHit].BackKey() == fvHitsAll[usedHitIDs[0]].BackKey()) {  // only one track will match
  //           // remove iBegHit from begTrack
  //           begTrack[iBegHit] = -1;
  //           // Probably redundant.
  //           fHitKeyFlags[fvHitsAll[begHit].FrontKey()] = 1;
  //           fHitKeyFlags[fvHitsAll[begHit].BackKey()]  = 1;

  //           selected = 1;
  //           break;
  //         }
  //       }
  //     }
  //   }
  //   fSelectedTrackIndexes[iTrack] = selected;
  //   int trackLength               = 0;
  //   for (int iHit = 0; iHit < track.size(); iHit++) {
  //     if (track[iHit] != -1) trackLength++;
  //   }
  //   fTrackNumHits[iTrack] = trackLength;
  //   // mark all hits as used
  //   for (const auto& hit : track) {
  //     if (hit == -1) continue;
  //     fHitKeyFlags[fvHitsAll[hit].FrontKey()] = 1;
  //     fHitKeyFlags[fvHitsAll[hit].BackKey()]  = 1;
  //   }
  // }  // iTrack
}  // Competition


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
  // const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  // const int nHits = fNHits;
  // if (iGThread >= nHits) return;

  // const int tIdx = ctx.thread_idx_x();
  // const int bIdx = ctx.block_idx_x();

  // ExclusiveScan::scan_t scan{ctx.pos(), ctx.smem()};

  // // input = count of valid triplets for this hit
  // int input  = fNTriplets[iGThread];
  // int result = 0;

  // // exclusive prefix sum within the block
  // scan.exclusive_sum(input, result);

  // xpu::barrier(ctx.pos());

  // // write per-thread partial (block-local) result
  // fOffsets[iGThread] = result;

  // if (bIdx == 0)
  //   printf("iGThread: %d , result: %d , fNTriplets: %d \n", iGThread, fOffsets[iGThread], fNTriplets[iGThread]);

  // // the last thread in the block publishes the block sum
  // if (tIdx == kScanBlockSize - 1 || iGThread == nHits - 1) {
  //   // block sum = last exclusive + its own input
  //   fBlockOffsets[bIdx] = result + input;
  // }
}

// 2) Scan the block sums -> fBlockOffsets now holds global block offsets
XPU_D void GnnGpuGraphConstructor::AddBlockSums(AddBlockSums::context& ctx, int nBlocks) const
{
  // const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
  // if (iGThread >= nBlocks) return;

  // const int tIdx = ctx.thread_idx_x();
  // const int bIdx = ctx.block_idx_x();

  // AddBlockSums::scan_t scan{ctx.pos(), ctx.smem()};

  // int input  = fBlockOffsets[iGThread];
  // int result = 0;

  // scan.exclusive_sum(input, result);

  // xpu::barrier(ctx.pos());

  // // overwrite with global block offset (start of this block in the full scan)
  // fBlockOffsets[iGThread] = result;

  // // keep last value per (meta)block range, mirroring your colleague’s pattern
  // if (tIdx == kScanBlockSize - 1 || iGThread == nBlocks - 1) {
  //   fBlockOffsetsLast[bIdx] = input;  // last block’s sum (inclusive) inside this AddBlockSums launch block
  // }
}

// 3) Add the global block offsets to each element to finalize the scan
XPU_D void GnnGpuGraphConstructor::AddOffsets(AddOffsets::context& ctx) const
{
  // const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  // const int nHits = fNHits;
  // if (iGThread >= nHits) return;

  // const int bIdx = ctx.block_idx_x();

  // // Add this block's global offset
  // fOffsets[iGThread] += fBlockOffsets[bIdx];
}

// 4) Scatter into flat output
XPU_D void GnnGpuGraphConstructor::CompressAllTripletsOrdered(CompressAllTripletsOrdered::context& ctx) const
{
  // const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  // const int nHits = fNHits;
  // if (iGThread >= nHits) return;

  // const unsigned int count  = fNTriplets[iGThread];
  // const unsigned int offset = fOffsets[iGThread];

  // // Copy the valid, already-locally-ordered entries
  // // Triplet element type: std::array<unsigned int, 2>
  // for (unsigned int j = 0; j < count; ++j) {
  //   fTripletsFlat[offset + j] = fTriplets[iGThread][j];
  // }
}

XPU_D void GnnGpuGraphConstructor::ConstructCandidates(ConstructCandidates::context& ctx) const
{
  // pass
}