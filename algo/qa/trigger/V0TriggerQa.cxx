/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   V0TriggerQa.cxx
/// \brief  A V0-trigger QA (implementation)
/// \since  06.03.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "qa/trigger/V0TriggerQa.h"

#include "qa/Histogram.h"

using cbm::algo::evbuild::V0TriggerQa;

// ---------------------------------------------------------------------------------------------------------------------
//
void V0TriggerQa::Init()
{
  // Histograms
  fphPairDeltaT  = MakeObj<qa::H1D>("v0trigger_pair_delta_t", "Time difference of track pair;#Delta t [ns];Counts",
                                   kPairDeltaTB, kPairDeltaTL, kPairDeltaTU);
  fphPairZVertex = MakeObj<qa::H1D>("v0trigger_pair_z_vertex", "z-verex of track pair;z [cm];Counts", kPairZVertexB,
                                    kPairZVertexL, kPairZVertexU);
  fphPairDca     = MakeObj<qa::H1D>("v0trigger_pair_dca", "Track pair distance of closest approach;DCA [cm];Counts",
                                kPairDcaB, kPairDcaL, kPairDcaU);

  // Canvas
  auto canv = qa::CanvasConfig(GetTaskName(), "V0 Trigger summary", 3, 1);
  canv.AddPadConfig(qa::PadConfig(fphPairDeltaT, "hist"));
  canv.AddPadConfig(qa::PadConfig(fphPairZVertex, "hist"));
  canv.AddPadConfig(qa::PadConfig(fphPairDca, "hist"));
  AddCanvasConfig(canv);
}
