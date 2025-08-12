/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GnnGpuGraphConstructor.cxx
/// \brief The class contains data and kernels for running GNN tracking on CPU and GPU using XPU libraries

#include "GnnGpuGraphConstructor.h"

using namespace cbm::algo::ca;

// Export Constants
XPU_EXPORT(strGnnGpuGraphConstructor);

// Kernel Definitions
//XPU_EXPORT(TestFunc);
//XPU_D void TestFunc::operator()(context& ctx) { ctx.cmem<strGpuTripletConstructor>().TestFunc(ctx); }

XPU_EXPORT(EmbedHits);
XPU_D void EmbedHits::operator()(context& ctx) { ctx.cmem<strGnnGpuGraphConstructor>().EmbedHits(ctx); }

//XPU_D void GpuTripletConstructor::TestFunc(TestFunc::context& ctx) const
//{
//  const int iGThread    = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();
//}

XPU_D void GnnGpuGraphConstructor::EmbedHits(EmbedHits::context& ctx) const
{
  const int iGThread = ctx.block_dim_x() * ctx.block_idx_x() + ctx.thread_idx_x();

  if (iGThread >= fIterationData[0].fNHits) return;

  ca::HitIndex_t ihitl = fgridEntries[iGThread];  //iGThread;
  int ista             = fvHits[ihitl].Station();


}
