/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmTofHitsConverter.h"

#include "CbmDefs.h"
#include "CbmEvent.h"
#include "CbmMCDataArray.h"
#include "CbmMCDataManager.h"
#include <CbmGlobalTrack.h>
#include <CbmTofHit.h>
#include <CbmTrackMatchNew.h>

#include <FairMCPoint.h>
#include <FairRootManager.h>

#include "TClonesArray.h"

#include <AnalysisTree/TaskManager.hpp>
#include <cassert>

#include "AnalysisTree/Matching.hpp"

ClassImp(CbmTofHitsConverter);

void CbmTofHitsConverter::Init()
{
  assert(!out_branch_.empty());
  auto* ioman = FairRootManager::Instance();

  cbm_tof_hits_      = (TClonesArray*) ioman->GetObject("TofHit");
  cbm_global_tracks_ = (TClonesArray*) ioman->GetObject("GlobalTrack");
  cbm_tof_match_     = (TClonesArray*) ioman->GetObject("TofHitMatch");
  //  cbm_tof_points_    = (TClonesArray*) ioman->GetObject("TofPoint");
  //  cbm_mc_tracks_     = (TClonesArray*) ioman->GetObject("MCTrack");

  cbm_mc_manager_     = dynamic_cast<CbmMCDataManager*>(ioman->GetObject("MCDataManager"));
  cbm_mc_tracks_new_  = cbm_mc_manager_->InitBranch("MCTrack");
  cbm_tof_points_new_ = cbm_mc_manager_->InitBranch("TofPoint");

  AnalysisTree::BranchConfig tof_branch(out_branch_, AnalysisTree::DetType::kHit);
  tof_branch.AddField<float>("mass2", "Mass squared");
  tof_branch.AddField<float>("l", "Track lenght");
  tof_branch.AddField<float>("t", "ps(?), Measured time ");
  tof_branch.AddField<float>("qp_tof", "charge * momentum extrapoleted to TOF");
  tof_branch.AddFields<float>({"dx", "dy", "dz"}, "Distance between TOF hit and extrapolated global track, cm");

  i_mass2_ = tof_branch.GetFieldId("mass2");
  i_qp_    = tof_branch.GetFieldId("qp_tof");
  i_dx_    = tof_branch.GetFieldId("dx");
  i_t_     = tof_branch.GetFieldId("t");
  i_l_     = tof_branch.GetFieldId("l");

  auto* man = AnalysisTree::TaskManager::GetInstance();
  man->AddBranch(tof_hits_, tof_branch);
  man->AddMatching(match_to_, out_branch_, vtx_tracks_2_tof_);
  man->AddMatching(out_branch_, mc_tracks_, tof_hits_2_mc_tracks_);
}

void CbmTofHitsConverter::ExtrapolateStraightLine(FairTrackParam* params, float z)
{
  const Float_t Tx    = params->GetTx();
  const Float_t Ty    = params->GetTy();
  const Float_t old_z = params->GetZ();
  const Float_t dz    = z - old_z;

  const Float_t x = params->GetX() + Tx * dz;
  const Float_t y = params->GetY() + Ty * dz;

  params->SetPosition({x, y, z});
}

void CbmTofHitsConverter::ProcessData(CbmEvent* event)
{
  assert(cbm_tof_hits_);
  tof_hits_->ClearChannels();
  vtx_tracks_2_tof_->Clear();
  tof_hits_2_mc_tracks_->Clear();

  auto* out_config_  = AnalysisTree::TaskManager::GetInstance()->GetConfig();
  const auto& branch = out_config_->GetBranchConfig(out_branch_);

  auto rec_tracks_map = GetMatchMap(match_to_);
  auto sim_tracks_map = GetMatchMap(mc_tracks_);

  int file_id {0}, event_id {0};
  if (event && event->GetMatch() && event->GetMatch()->GetNofLinks() > 0) {
    file_id  = event->GetMatch()->GetMatchedLink().GetFile();
    event_id = event->GetMatch()->GetMatchedLink().GetEntry();
  }
  else {
    event_id = FairRootManager::Instance()->GetEntryNr();
  }

  const int n_tracks = event ? event->GetNofData(ECbmDataType::kGlobalTrack) : cbm_global_tracks_->GetEntriesFast();
  //  const int n_tof_hits = event ? event->GetNofData(ECbmDataType::kTofHit) : cbm_tof_hits_->GetEntriesFast(); // FU, 02.08.22

  if (n_tracks <= 0) {
    LOG(warn) << "No global tracks!";
    return;
  }
  tof_hits_->Reserve(n_tracks);

  for (Int_t igt = 0; igt < n_tracks; igt++) {
    const auto trackIndex   = event ? event->GetIndex(ECbmDataType::kGlobalTrack, igt) : igt;
    const auto* globalTrack = static_cast<const CbmGlobalTrack*>(cbm_global_tracks_->At(trackIndex));
    const Int_t tofHitIndex = globalTrack->GetTofHitIndex();
    if (tofHitIndex < 0) continue;

    const auto* tofHit = static_cast<const CbmTofHit*>(cbm_tof_hits_->At(tofHitIndex));

    FairTrackParam param_last = *(globalTrack->GetParamLast());
    TVector3 p_tof;
    param_last.Momentum(p_tof);

    const Float_t p    = p_tof.Mag();
    const Int_t q      = param_last.GetQp() > 0 ? 1 : -1;
    const Float_t l    = globalTrack->GetLength();  // l is calculated by global tracking
    const Float_t time = tofHit->GetTime();
    const Float_t beta = event ? l / ((time - event->GetTzero()) * 29.9792458) : 0;
    const Float_t m2   = event ? p * p * (1. / (beta * beta) - 1.) : -1.;

    const Float_t hitX = tofHit->GetX();
    const Float_t hitY = tofHit->GetY();
    const Float_t hitZ = tofHit->GetZ();

    ExtrapolateStraightLine(&param_last, tofHit->GetZ());

    auto& hit = tof_hits_->AddChannel(branch);
    hit.SetPosition(hitX, hitY, hitZ);
    hit.SetSignal(time);
    hit.SetField(m2, i_mass2_);
    hit.SetField(float(q * p_tof.Mag()), i_qp_);
    hit.SetField(float(param_last.GetX() - hitX), i_dx_);
    hit.SetField(float(param_last.GetY() - hitY), i_dx_ + 1);
    hit.SetField(float(param_last.GetZ() - hitZ), i_dx_ + 2);
    hit.SetField(l, i_l_);
    hit.SetField(time, i_t_);

    if (rec_tracks_map.empty()) { continue; }
    const Int_t stsTrackIndex = globalTrack->GetStsTrackIndex();
    if (rec_tracks_map.find(stsTrackIndex) != rec_tracks_map.end()) {
      vtx_tracks_2_tof_->AddMatch(rec_tracks_map.find(stsTrackIndex)->second, hit.GetId());
    }

    const auto* tofMatch = dynamic_cast<CbmMatch*>(cbm_tof_match_->At(tofHitIndex));
    if (tofMatch && tofMatch->GetNofLinks() > 0) {
      const auto& link = tofMatch->GetMatchedLink();
      if (link.GetFile() != file_id || link.GetEntry() != event_id) {  // match from different event
        LOG(warn) << "match from different event";
        //        continue;
      }
      const auto* tofPoint = dynamic_cast<FairMCPoint*>(cbm_tof_points_new_->Get(link));
      //      const auto* tofPoint = dynamic_cast<FairMCPoint*>(cbm_tof_points_->At(link.GetIndex()));

      if (!tofPoint) { throw std::runtime_error("no TOF point"); }

      Int_t mc_track_id = tofPoint->GetTrackID();
      if (mc_track_id >= 0) {
        auto it = sim_tracks_map.find(mc_track_id);
        if (it != sim_tracks_map.end()) {  // match is found
          tof_hits_2_mc_tracks_->AddMatch(hit.GetId(), it->second);
        }
      }
    }
  }
}


CbmTofHitsConverter::~CbmTofHitsConverter()
{
  delete tof_hits_;
  delete vtx_tracks_2_tof_;
};
