/* Copyright (C) 2020-2021 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Viktor Klochkov [committer] */

#include "CbmFsdHitsConverter.h"

#include "AnalysisTree/Matching.hpp"
#include "CbmDefs.h"
#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmFsdPoint.h"
#include "CbmGlobalTrack.h"
#include "CbmKFTrack.h"
#include "CbmMCDataArray.h"
#include "CbmMCDataManager.h"
#include "CbmMCTrack.h"
#include "TClonesArray.h"
#include <CbmFsdDigi.h>
#include <CbmFsdHit.h>
#include <CbmGlobalTrack.h>
#include <CbmStsTrack.h>
#include <CbmTofHit.h>
#include <CbmTrackMatchNew.h>

#include <FairMCPoint.h>
#include <FairRootManager.h>
#include <Logger.h>

#include <TGeoManager.h>  // for TGeoManager, gGeoManager
#include <TGeoMatrix.h>   // for TGeoMatrix
#include <TGeoNode.h>     // for TGeoIterator, TGeoNode
#include <TGeoShape.h>    // for TGeoBBox etc.
#include <TGeoVolume.h>   // for TGeoVolume

#include <AnalysisTree/TaskManager.hpp>
#include <cassert>

ClassImp(CbmFsdHitsConverter);

void CbmFsdHitsConverter::Init()
{
  LOG(debug) << " THIS IS A TEST -> CbmFsdHitsConverter is being initialized LUKAS" << std::endl;

  assert(!out_branch_.empty());
  auto* ioman = FairRootManager::Instance();

  cbm_fsd_hits_ = (TClonesArray*) ioman->GetObject("FsdHit");

  cbm_global_tracks_ = (TClonesArray*) ioman->GetObject("GlobalTrack");
  cbm_sts_tracks_    = (TClonesArray*) ioman->GetObject("StsTrack");
  cbm_tof_hits_      = (TClonesArray*) ioman->GetObject("TofHit");
  cbm_fsd_hitmatch_  = (TClonesArray*) ioman->GetObject("FsdHitMatch");
  cbm_mc_tracks_     = (TClonesArray*) ioman->GetObject("MCTrack");

  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsMatchPresent(ECbmModuleId::kFsd)) {
    LOG(error) << " No FsdDigiMatch input array present !!";
  }
  //cbm_fsd_digis_     = (TClonesArray*) ioman->GetObject("FsdDigi");
  //cbm_fsd_digimatch_ = (TClonesArray*) ioman->GetObject("FsdDigiMatch");

  cbm_mc_manager_ = dynamic_cast<CbmMCDataManager*>(ioman->GetObject("MCDataManager"));
  //cbm_mc_tracks_new_  = cbm_mc_manager_->InitBranch("MCTrack");
  cbm_fsd_points_new_ = cbm_mc_manager_->InitBranch("FsdPoint");

  AnalysisTree::BranchConfig fsd_branch(out_branch_, AnalysisTree::DetType::kHit);

  fsd_branch.AddField<float>("dEdx", "Energy deposition of given FSD hit [GeV]");
  fsd_branch.AddField<float>("t", "Reconstructed time of given FSD hit [ps]");
  fsd_branch.AddField<float>("mass2", "Calculated mass squared from extrapolated global track  (by CbmKFTrack) to FSD "
                                      "plane and FSD hit time [GeV^2/c^4]");
  fsd_branch.AddField<float>("l", "Lenght of the extrapolated global track (by CbmKFTrack) to FSD plane [cm]");
  fsd_branch.AddField<float>(
    "qp", "charge * momentum  of the extrapoleted global track (by CbmKFTrack) to FSD plane [GeV/c]");
  fsd_branch.AddFields<float>(
    {"dx", "dy", "dz"},
    "Component of a 3D distance between FSD hit and extrapolated global track (by CbmKFTrack) [cm]");
  fsd_branch.AddField<float>(
    "chi2GtrackHit", "chi2 between extrapolated global track  (by CbmKFTrack) to FSD plane (?FIXED Z?) and FSD hit");
  fsd_branch.AddField<int>(
    "bestMatchedGtrack2HitId",
    "Index of best match between extrapolated global track (by CbmKFTrack) and FSD hit based on  min chi2GtrackHit");
  fsd_branch.AddField<int>("multMCtracks", "number of MC particles that cotributed by energy deposition to FSD hit");
  fsd_branch.AddField<float>("maxWeightMCtrack",
                             "weight of matched link from Hit to Point (?highest energy deposition?)");
  fsd_branch.AddField<float>("dtHitPoint", "Time difference between FSD hit and matched MC point [ps]");
  fsd_branch.AddFields<float>({"dxHitPoint", "dyHitPoint", "dzHitPoint"},
                              "Component of a 3D distance between FSD hit and matched MC point [cm]");
  fsd_branch.AddFields<float>({"xPoint", "yPoint", "zPoint"}, "MC point distribution [cm]");
  fsd_branch.AddFields<float>({"pxPoint", "pyPoint", "pzPoint"}, "MC point momentum");
  fsd_branch.AddField<float>({"phiPoint"}, "Angle of the point");
  fsd_branch.AddField<float>({"lengthPoint"}, "Lenght of the point");
  fsd_branch.AddField<float>({"tPoint"}, "Time of the point");
  fsd_branch.AddField<float>({"elossPoint"}, "Energy loss of the point");
  fsd_branch.AddField<float>({"dist_middle_x"}, "Absolute value of the hit distance x from zero");
  fsd_branch.AddField<float>({"dist_middle_y"}, "Absolute value of the hit distance y from zero");


  i_edep_              = fsd_branch.GetFieldId("dEdx");
  i_t_                 = fsd_branch.GetFieldId("t");
  i_mass2_             = fsd_branch.GetFieldId("mass2");
  i_qp_                = fsd_branch.GetFieldId("qp");
  i_dx_                = fsd_branch.GetFieldId("dx");
  i_l_                 = fsd_branch.GetFieldId("l");
  i_dtHP_              = fsd_branch.GetFieldId("dtHitPoint");
  i_dxHP_              = fsd_branch.GetFieldId("dxHitPoint");
  i_chi2_              = fsd_branch.GetFieldId("chi2GtrackHit");
  i_bestMatchedGTrack_ = fsd_branch.GetFieldId("bestMatchedGtrack2HitId");
  i_multMC_            = fsd_branch.GetFieldId("multMCtracks");
  i_topW_              = fsd_branch.GetFieldId("maxWeightMCtrack");
  i_xpoint_            = fsd_branch.GetFieldId("xPoint");
  i_pxpoint_           = fsd_branch.GetFieldId("pxPoint");
  i_phipoint_          = fsd_branch.GetFieldId("phiPoint");
  i_lengthpoint_       = fsd_branch.GetFieldId("lengthPoint");
  i_tpoint_            = fsd_branch.GetFieldId("tPoint");
  i_eloss_             = fsd_branch.GetFieldId("elossPoint");
  i_dist_middle_x_     = fsd_branch.GetFieldId("dist_middle_x");
  i_dist_middle_y_     = fsd_branch.GetFieldId("dist_middle_y");


  if (fsdgtrack_minChi2_ == 0) fsdgtrack_minChi2_ = -1.;
  if (fsdgtrack_maxChi2_ == 0) fsdgtrack_maxChi2_ = 10000.;

  auto* man = AnalysisTree::TaskManager::GetInstance();
  man->AddBranch(fsd_hits_, fsd_branch);
  man->AddMatching(match_to_, out_branch_, vtx_tracks_2_fsd_);
  man->AddMatching(out_branch_, mc_tracks_, fsd_hits_2_mc_tracks_);
}

FairTrackParam CbmFsdHitsConverter::ExtrapolateGtrack(Double_t zpos, FairTrackParam params)
{
  // follow inspiration from CbmL1TofMerger
  FairTrackParam returnParams;
  CbmKFTrack kfTrack;
  kfTrack.SetTrackParam(params);
  kfTrack.Extrapolate(zpos);  //this will do straight track extrapolation
  kfTrack.GetTrackParam(returnParams);
  return returnParams;
}


Double_t CbmFsdHitsConverter::Chi2FsdhitGtrack(CbmFsdHit* hit, FairTrackParam inputParams)
{
  FairTrackParam params = ExtrapolateGtrack(hit->GetZ(), inputParams);

  Float_t dX = params.GetX() - hit->GetX();
  Float_t dY = params.GetY() - hit->GetY();

  Double_t cxx  = params.GetCovariance(0, 0) + hit->GetDx() * hit->GetDx();
  Double_t cxy  = params.GetCovariance(0, 1) + hit->GetDxy() * hit->GetDxy();
  Double_t cyy  = params.GetCovariance(1, 1) + hit->GetDy() * hit->GetDy();
  Double_t chi2 = 0.5 * (dX * dX * cxx - 2 * dX * dY * cxy + dY * dY * cyy) / (cxx * cyy - cxy * cxy);

  return chi2;
}

void CbmFsdHitsConverter::ProcessData(CbmEvent* event)
{
  assert(cbm_fsd_hits_);
  fsd_hits_->ClearChannels();
  vtx_tracks_2_fsd_->Clear();
  fsd_hits_2_mc_tracks_->Clear();

  auto* out_config_  = AnalysisTree::TaskManager::GetInstance()->GetConfig();
  const auto& branch = out_config_->GetBranchConfig(out_branch_);

  auto rec_tracks_map = GetMatchMap(match_to_);
  auto sim_tracks_map = GetMatchMap(mc_tracks_);

  int file_id{0}, event_id{0};
  if (event) {
    auto match = event->GetMatch();
    if (!match) return;
    file_id  = event->GetMatch()->GetMatchedLink().GetFile();
    event_id = event->GetMatch()->GetMatchedLink().GetEntry();
  }
  else {
    event_id = FairRootManager::Instance()->GetEntryNr();
  }

  const int n_fsd_hits = event ? event->GetNofData(ECbmDataType::kFsdHit) : cbm_fsd_hits_->GetEntriesFast();
  if (n_fsd_hits <= 0) {
    LOG(warn) << "No FSD hits!";
    return;
  }

  const int n_tracks = event ? event->GetNofData(ECbmDataType::kGlobalTrack) : cbm_global_tracks_->GetEntriesFast();
  if (n_tracks <= 0) {
    LOG(warn) << "No Global Tracks!";
    return;
  }

  fsd_hits_->Reserve(n_fsd_hits);


  // first loop over all FSD hits
  for (Int_t ifh = 0; ifh < n_fsd_hits; ifh++) {
    const auto fsdHI = event ? event->GetIndex(ECbmDataType::kFsdHit, ifh) : ifh;
    auto* fsdHit     = static_cast<CbmFsdHit*>(cbm_fsd_hits_->At(fsdHI));

    auto& hit = fsd_hits_->AddChannel(branch);

    const Float_t hitX  = fsdHit->GetX();
    const Float_t hitY  = fsdHit->GetY();
    const Float_t hitZ  = fsdHit->GetZ();
    const Float_t eLoss = fsdHit->GetEdep();
    const Float_t time  = fsdHit->GetTime();

    const Float_t dist_x = std::fabs(fsdHit->GetX());
    const Float_t dist_y = std::fabs(fsdHit->GetY());

    hit.SetField(dist_x, i_dist_middle_x_);
    hit.SetField(dist_y, i_dist_middle_y_);

    hit.SetPosition(hitX, hitY, hitZ);
    hit.SetSignal(time);
    hit.SetField(eLoss, i_edep_);

    Float_t phi_hit = atan2(fsdHit->GetY(), fsdHit->GetX());
    if (phi_hit < 0) {
      phi_hit = phi_hit + 2 * 3.1415;
    }


    Int_t multMC            = 0;
    Float_t highestWeight   = 0.;
    const auto* fsdHitMatch = dynamic_cast<CbmMatch*>(cbm_fsd_hitmatch_->At(fsdHI));
    if (fsdHitMatch && fsdHitMatch->GetNofLinks() > 0) {
      highestWeight = fsdHitMatch->GetMatchedLink().GetWeight();

      for (int32_t ilDigi = 0; ilDigi < fsdHitMatch->GetNofLinks(); ilDigi++) {
        const auto& digiLink = fsdHitMatch->GetLink(ilDigi);
        if (digiLink.GetFile() != file_id || digiLink.GetEntry() != event_id) {  // match from different event
          continue;
        }

        const auto* fsdDigiMatch = fDigiMan->GetMatch(ECbmModuleId::kFsd, digiLink.GetIndex());
        if (fsdDigiMatch && fsdDigiMatch->GetNofLinks() > 0) {
          for (int32_t ilPoint = 0; ilPoint < fsdDigiMatch->GetNofLinks(); ilPoint++) {
            const auto& pointLink = fsdDigiMatch->GetLink(ilPoint);
            if (pointLink.GetFile() != file_id || pointLink.GetEntry() != event_id) {  // match from different event
              continue;
            }

            multMC++;  // increase counter of MC multiplicity
            // add matching between FSD hit and MC track
            //const auto* fsdPoint = dynamic_cast<FairMCPoint*>(cbm_fsd_points_new_->Get(pointLink));
            const auto* fsdPoint = dynamic_cast<FairMCPoint*>(cbm_fsd_points_new_->Get(digiLink));
            Int_t mc_track_id    = fsdPoint->GetTrackID();
            if (mc_track_id >= 0) {
              auto it = sim_tracks_map.find(mc_track_id);
              if (it != sim_tracks_map.end()) {  // match is found
                fsd_hits_2_mc_tracks_->AddMatch(hit.GetId(), it->second);
              }
            }
            // for the "matched" links store the difference between Hit and Point
            /////if (fsdHitMatch->GetMatchedLink().GetIndex() == ilDigi
            /////    && fsdDigiMatch->GetMatchedLink().GetIndex() == ilPoint) {
            //if(fsdPoint->GetLength() > 0.1){
            hit.SetField(float(fsdPoint->GetTime() - time), i_dtHP_);
            hit.SetField(float(fsdPoint->GetX() - hitX), i_dxHP_);
            hit.SetField(float(fsdPoint->GetY() - hitY), i_dxHP_ + 1);
            hit.SetField(float(fsdPoint->GetZ() - hitZ), i_dxHP_ + 2);
            hit.SetField(float(fsdPoint->GetEnergyLoss()), i_eloss_);

            hit.SetField(float(fsdPoint->GetX()), i_xpoint_);
            hit.SetField(float(fsdPoint->GetY()), i_xpoint_ + 1);
            hit.SetField(float(fsdPoint->GetZ()), i_xpoint_ + 2);
            hit.SetField(float(fsdPoint->GetLength()), i_lengthpoint_);
            hit.SetField(float(fsdPoint->GetPx()), i_pxpoint_);
            hit.SetField(float(fsdPoint->GetPy()), i_pxpoint_ + 1);
            hit.SetField(float(fsdPoint->GetPz()), i_pxpoint_ + 2);

            Float_t phi_point = atan2(fsdPoint->GetY(), fsdPoint->GetX());
            if (phi_point < 0) {
              phi_point = phi_point + 2 * 3.1415;
            }
            hit.SetField(float(phi_point), i_phipoint_);
            hit.SetField(float(fsdPoint->GetTime()), i_tpoint_);
            /////}
            //}

          }  // end of loop over links to points
        }
      }  // end of loop over links to digis
    }    // end of sanity check of FSD hit matches

    const Int_t hitMCmult      = multMC;
    const Float_t hitTopWeight = highestWeight;
    hit.SetField(hitMCmult, i_multMC_);
    hit.SetField(hitTopWeight, i_topW_);

    // now matching with Global (reco) tracks
    Int_t bestMatchedIndex = -1;
    Double_t bestChi2      = 0.;
    for (Int_t igt = 0; igt < n_tracks; igt++) {
      const auto trackIndex     = event ? event->GetIndex(ECbmDataType::kGlobalTrack, igt) : igt;
      const auto* globalTrack   = static_cast<const CbmGlobalTrack*>(cbm_global_tracks_->At(trackIndex));
      FairTrackParam param_last = *(globalTrack->GetParamLast());

      Double_t matchingChi2 = Chi2FsdhitGtrack(fsdHit, param_last);

      if (bestChi2 > matchingChi2 || bestMatchedIndex < 0) {
        bestChi2         = matchingChi2;
        bestMatchedIndex = trackIndex;
      }
    }  // end of loop over GlobalTracks

    hit.SetField(static_cast<Float_t>(bestChi2), i_chi2_);
    hit.SetField(static_cast<Int_t>(bestMatchedIndex), i_bestMatchedGTrack_);

    if ((bestMatchedIndex >= 0) && (bestChi2 > fsdgtrack_minChi2_) && (bestChi2 < fsdgtrack_maxChi2_)) {
      const auto* globalTrack   = static_cast<const CbmGlobalTrack*>(cbm_global_tracks_->At(bestMatchedIndex));
      FairTrackParam param_last = *(globalTrack->GetParamLast());
      FairTrackParam param_fsd  = ExtrapolateGtrack(hitZ, param_last);

      TVector3 p_fsd;
      param_fsd.Momentum(p_fsd);

      const Float_t p    = p_fsd.Mag();
      const Int_t q      = param_fsd.GetQp() > 0 ? 1 : -1;
      const Float_t l    = globalTrack->GetLength();  // l is calculated by global tracking
      const Float_t beta = event ? l / ((time - event->GetTzero()) * 29.9792458) : 0;
      const Float_t m2   = event ? p * p * (1. / (beta * beta) - 1.) : -1.;

      hit.SetField(m2, i_mass2_);
      hit.SetField(float(q * p), i_qp_);
      hit.SetField(float(param_fsd.GetX() - hitX), i_dx_);
      hit.SetField(float(param_fsd.GetY() - hitY), i_dx_ + 1);
      hit.SetField(float(param_fsd.GetZ() - hitZ), i_dx_ + 2);
      hit.SetField(l, i_l_);
      hit.SetField(time, i_t_);

      if (rec_tracks_map.empty()) {
        continue;
      }
      const Int_t stsTrackIndex = globalTrack->GetStsTrackIndex();
      if (rec_tracks_map.find(stsTrackIndex) != rec_tracks_map.end()) {
        vtx_tracks_2_fsd_->AddMatch(rec_tracks_map.find(stsTrackIndex)->second, hit.GetId());
      }
    }
  }  // end of loop over FSD hits
}


CbmFsdHitsConverter::~CbmFsdHitsConverter()
{
  delete fsd_hits_;
  delete vtx_tracks_2_fsd_;
};
 
