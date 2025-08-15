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

  const int kNNOrder = 20;    // FastPrim
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
