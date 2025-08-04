/* Copyright (C) 2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmTrdTracksConverter.h"

#include "CbmDefs.h"
#include "CbmEvent.h"
#include <CbmGlobalTrack.h>
#include <CbmTrdHit.h>
#include <CbmTrdTrack.h>

#include <FairMCPoint.h>
#include <FairRootManager.h>

#include "TClonesArray.h"

#include <AnalysisTree/TaskManager.hpp>
#include <cassert>

#include "AnalysisTree/Matching.hpp"

ClassImp(CbmTrdTracksConverter);


void CbmTrdTracksConverter::Init()
{
  assert(!out_branch_.empty());
  auto* ioman = FairRootManager::Instance();

  cbm_trd_tracks_    = (TClonesArray*) ioman->GetObject("TrdTrack");
  cbm_global_tracks_ = (TClonesArray*) ioman->GetObject("GlobalTrack");
  cbm_trd_hits_      = (TClonesArray*) ioman->GetObject("TrdHit");

  AnalysisTree::BranchConfig trd_branch(out_branch_, AnalysisTree::DetType::kTrack);
  trd_branch.AddFields<float>({"energy_loss_0", "energy_loss_1", "energy_loss_2", "energy_loss_3"},
                              "keV(?), Energy loss per TRD station");
  trd_branch.AddFields<float>({"pid_like_e", "pid_like_pi", "pid_like_k", "pid_like_p"},
                              "Probability to be a given particle specie");
  trd_branch.AddField<float>("chi2_ov_ndf", "chi2 divided by NDF of the track fit");
  trd_branch.AddFields<float>({"pT_out", "p_out"}, "Momentum at last point (?)");
  trd_branch.AddField<int>("n_hits", "Number of hits");

  i_e_loss_i_    = trd_branch.GetFieldId("energy_loss_0");
  i_pid_like_    = trd_branch.GetFieldId("pid_like_e");
  i_chi2_ov_ndf_ = trd_branch.GetFieldId("chi2_ov_ndf");
  i_pT_out_      = trd_branch.GetFieldId("pT_out");
  i_n_hits_      = trd_branch.GetFieldId("n_hits");

  auto* man = AnalysisTree::TaskManager::GetInstance();
  man->AddBranch(trd_tracks_, trd_branch);
  man->AddMatching(match_to_, out_branch_, vtx_tracks_2_trd_);
}

void CbmTrdTracksConverter::ProcessData(CbmEvent* event)
{
  assert(cbm_trd_tracks_);
  trd_tracks_->ClearChannels();
  vtx_tracks_2_trd_->Clear();

  auto* out_config_  = AnalysisTree::TaskManager::GetInstance()->GetConfig();
  const auto& branch = out_config_->GetBranchConfig(out_branch_);

  const auto it = indexes_map_->find(match_to_);
  if (it == indexes_map_->end()) { throw std::runtime_error(match_to_ + " is not found to match with TRD tracks"); }
  auto rec_tracks_map = it->second;

  const int n_tracks = event ? event->GetNofData(ECbmDataType::kGlobalTrack) : cbm_global_tracks_->GetEntriesFast();
  if (n_tracks <= 0) {
    LOG(warn) << "No global tracks!";
    return;
  }

  trd_tracks_->Reserve(n_tracks);

  for (Int_t igt = 0; igt < n_tracks; igt++) {
    const auto trackIndex = event ? event->GetIndex(ECbmDataType::kGlobalTrack, igt) : igt;

    const auto* global_track = static_cast<const CbmGlobalTrack*>(cbm_global_tracks_->At(trackIndex));

    Int_t itrd = global_track->GetTrdTrackIndex();
    if (itrd < 0) continue;

    auto trd_track = static_cast<CbmTrdTrack*>(cbm_trd_tracks_->At(itrd));
    if (trd_track == nullptr) {
      LOG(warn) << "No TRD track!";
      continue;
    }
    auto& track = trd_tracks_->AddChannel(branch);
    TVector3 mom, mom_last;
    trd_track->GetParamFirst()->Momentum(mom);
    trd_track->GetParamLast()->Momentum(mom_last);

    track.SetMomentum3(mom);

    track.SetField(float(trd_track->GetPidLikeEL()), i_pid_like_);
    track.SetField(float(trd_track->GetPidLikePI()), i_pid_like_ + 1);
    track.SetField(float(trd_track->GetPidLikeKA()), i_pid_like_ + 2);
    track.SetField(float(trd_track->GetPidLikePR()), i_pid_like_ + 3);

    track.SetField(float(trd_track->GetNDF() > 0. ? trd_track->GetChiSq() / trd_track->GetNDF() : -999.),
                   i_chi2_ov_ndf_);

    track.SetField(float(mom_last.Pt()), i_pT_out_);
    track.SetField(float(mom_last.Mag()), i_pT_out_ + 1);

    for (int i = 0; i < 4; ++i) {
      track.SetField(0.f, i_e_loss_i_ + i);
    }

    int trd_hits = trd_track->GetNofHits();
    for (Int_t ihit = 0; ihit < trd_track->GetNofHits(); ihit++) {
      Int_t idx = trd_track->GetHitIndex(ihit);
      auto* hit = (CbmTrdHit*) cbm_trd_hits_->At(idx);
      if (hit) {
        if (hit->GetELoss() > 0) {
          track.SetField(float(hit->GetELoss() * 1e6), i_e_loss_i_ + hit->GetPlaneId());
        }
        else {
          trd_hits--;
        }
      }
    }

    track.SetField(trd_hits, i_n_hits_);

    if (rec_tracks_map.empty()) { continue; }
    const Int_t stsTrackIndex = global_track->GetStsTrackIndex();
    if (rec_tracks_map.find(stsTrackIndex) != rec_tracks_map.end()) {
      vtx_tracks_2_trd_->AddMatch(rec_tracks_map.find(stsTrackIndex)->second, track.GetId());
    }
  }
}

CbmTrdTracksConverter::~CbmTrdTracksConverter()
{
  delete trd_tracks_;
  delete vtx_tracks_2_trd_;
};
