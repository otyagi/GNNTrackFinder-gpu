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


XPU_D void GnnGpuGraphConstructor::EmbedHits(EmbedHits::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();


  if (iGThread >= fIterationData[0].fNHits) return;

  ca::HitIndex_t ihitl = fgridEntries[iGThread];
  const auto& hitl     = fvHits[ihitl];
  std::array<float, 3> input{hitl.X(), hitl.Y(), hitl.Z() + 44.0f};
  std::array<float, 6> result;
  EmbedSingleHit(input, result);

  embedCoord[iGThread] = result;
}

XPU_D void GnnGpuGraphConstructor::EmbedSingleHit(std::array<float, 3>& input, std::array<float, 6>& result) const
{
  std::array<float, 16> result1;
  affine<16, 3>(embedParameters[0].embedWeights_0, input, embedParameters[0].embedBias_0, result1);
  applyTanH(result1);

  std::array<float, 16> result2;
  affine<16, 16>(embedParameters[0].embedWeights_1, result1, embedParameters[0].embedBias_1, result2);
  applyTanH(result2);

  affine<6, 16>(embedParameters[0].embedWeights_2, result2, embedParameters[0].embedBias_2, result);
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
