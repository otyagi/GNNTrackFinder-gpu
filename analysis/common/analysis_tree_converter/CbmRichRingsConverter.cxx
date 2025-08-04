/* Copyright (C) 2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmRichRingsConverter.h"

#include "CbmDefs.h"
#include "CbmEvent.h"
#include <CbmGlobalTrack.h>
#include <CbmRichRing.h>

#include <FairMCPoint.h>
#include <FairRootManager.h>

#include "TClonesArray.h"

#include <AnalysisTree/TaskManager.hpp>
#include <cassert>
#include <vector>

#include "AnalysisTree/Matching.hpp"

void CbmRichRingsConverter::Init()
{

  assert(!out_branch_.empty());
  auto* ioman = FairRootManager::Instance();

  cbm_rich_rings_    = (TClonesArray*) ioman->GetObject("RichRing");
  cbm_global_tracks_ = (TClonesArray*) ioman->GetObject("GlobalTrack");

  AnalysisTree::BranchConfig rich_branch(out_branch_, AnalysisTree::DetType::kHit);
  rich_branch.AddField<float>("radius");
  rich_branch.AddFields<int>({"n_hits", "n_hits_on_ring"});
  rich_branch.AddFields<float>({"axis_a", "axis_b"});
  rich_branch.AddFields<float>({"center_x", "center_y"});
  rich_branch.AddField<float>("chi2_ov_ndf", "chi2/ndf ring fit");
  rich_branch.AddField<float>("phi_ellipse", "phi rotation angle of ellipse");
  rich_branch.AddField<float>("radial_pos", "sqrt(x**2+abs(y-110)**2)");
  rich_branch.AddField<float>("radial_angle", "(0||1||2)*pi +- atan( abs((+-100-y)/-x) )");

  i_r_            = rich_branch.GetFieldId("radius");
  i_n_hits_       = rich_branch.GetFieldId("n_hits");
  i_axis_         = rich_branch.GetFieldId("axis_a");
  i_center_       = rich_branch.GetFieldId("center_x");
  i_chi2_         = rich_branch.GetFieldId("chi2_ov_ndf");
  i_radial_angle_ = rich_branch.GetFieldId("radial_angle");
  i_radial_pos_   = rich_branch.GetFieldId("radial_pos");
  i_phi_ellipse_  = rich_branch.GetFieldId("phi_ellipse");

  auto* man = AnalysisTree::TaskManager::GetInstance();
  man->AddBranch(rich_rings_, rich_branch);
  man->AddMatching(match_to_, out_branch_, vtx_tracks_2_rich_);
}

void CbmRichRingsConverter::ProcessData(CbmEvent* event)
{
  assert(cbm_rich_rings_);
  rich_rings_->ClearChannels();
  vtx_tracks_2_rich_->Clear();

  auto* out_config_  = AnalysisTree::TaskManager::GetInstance()->GetConfig();
  const auto& branch = out_config_->GetBranchConfig(out_branch_);

  const auto it = indexes_map_->find(match_to_);
  if (it == indexes_map_->end()) { throw std::runtime_error(match_to_ + " is not found to match with RICH rings"); }
  auto rec_tracks_map = it->second;

  const int n_tracks = event ? event->GetNofData(ECbmDataType::kGlobalTrack) : cbm_global_tracks_->GetEntriesFast();
  if (n_tracks <= 0) {
    LOG(warn) << "No global tracks!";
    return;
  }
  rich_rings_->Reserve(n_tracks);

  for (Int_t igt = 0; igt < n_tracks; igt++) {
    const auto trackIndex    = event ? event->GetIndex(ECbmDataType::kGlobalTrack, igt) : igt;
    const auto* global_track = static_cast<const CbmGlobalTrack*>(cbm_global_tracks_->At(trackIndex));

    Int_t i_rich = global_track->GetRichRingIndex();
    if (i_rich < 0) continue;
    auto rich_ring = static_cast<CbmRichRing*>(cbm_rich_rings_->At(i_rich));
    if (rich_ring == nullptr) {
      LOG(warn) << "No RICH ring!";
      continue;
    }

    auto& ring = rich_rings_->AddChannel(branch);
    ring.SetPosition(rich_ring->GetCenterX(), rich_ring->GetCenterY(), 0.f);
    ring.SetField(int(rich_ring->GetNofHits()), i_n_hits_);
    ring.SetField(int(rich_ring->GetNofHitsOnRing()), i_n_hits_ + 1);
    ring.SetField(float(rich_ring->GetAaxis()), i_axis_);
    ring.SetField(float(rich_ring->GetBaxis()), i_axis_ + 1);
    ring.SetField(float(rich_ring->GetCenterX()), i_center_);
    ring.SetField(float(rich_ring->GetCenterY()), i_center_ + 1);
    ring.SetField(float(rich_ring->GetRadius()), i_r_);
    ring.SetField(float(rich_ring->GetNDF() > 0. ? rich_ring->GetChi2() / rich_ring->GetNDF() : -999.), i_chi2_);
    ring.SetField(float(rich_ring->GetRadialAngle()), i_radial_angle_);
    ring.SetField(float(rich_ring->GetRadialPosition()), i_radial_pos_);
    ring.SetField(float(rich_ring->GetPhi()), i_phi_ellipse_);

    if (rec_tracks_map.empty()) { continue; }
    const Int_t stsTrackIndex = global_track->GetStsTrackIndex();
    if (rec_tracks_map.find(stsTrackIndex) != rec_tracks_map.end()) {
      vtx_tracks_2_rich_->AddMatch(rec_tracks_map.find(stsTrackIndex)->second, ring.GetId());
    }
  }
}

CbmRichRingsConverter::~CbmRichRingsConverter()
{
  delete rich_rings_;
  delete vtx_tracks_2_rich_;
};
