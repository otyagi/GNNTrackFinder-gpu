/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Lukas Chlad [committer] */

#ifndef ANALYSIS_TREE_FSDHITSCONVERTER_H
#define ANALYSIS_TREE_FSDHITSCONVERTER_H

#include "AnalysisTree/Detector.hpp"
#include "CbmConverterTask.h"

class TClonesArray;
class FairTrackParam;
class CbmMCDataManager;
class CbmMCDataArray;
class CbmDigiManager;
class CbmFsdHit;

namespace AnalysisTree
{
  class Matching;
}

class CbmFsdHitsConverter final : public CbmConverterTask {
 public:
  explicit CbmFsdHitsConverter(std::string out_branch_name, std::string match_to = "")
    : CbmConverterTask(std::move(out_branch_name), std::move(match_to)){};

  ~CbmFsdHitsConverter() final;

  void Init() final;
  void ProcessData(CbmEvent* event) final;
  void Finish() final{};

  Double_t GetMinChi2GtrackHit() { return fsdgtrack_minChi2_; };
  Double_t GetMaxChi2GtrackHit() { return fsdgtrack_maxChi2_; };
  void SetMinChi2GtrackHit(Double_t chi2) { fsdgtrack_minChi2_ = chi2; };
  void SetMaxChi2GtrackHit(Double_t chi2) { fsdgtrack_maxChi2_ = chi2; };

 private:
  FairTrackParam ExtrapolateGtrack(Double_t zpos, FairTrackParam params);
  Double_t Chi2FsdhitGtrack(CbmFsdHit* hit, FairTrackParam inputParams);

  const std::map<int, int>& GetMatchMap(const std::string& name) const
  {
    const auto& it = indexes_map_->find(name);
    if (it == indexes_map_->end()) {
      throw std::runtime_error(name + " is not found to match with FSD hits");
    }
    return it->second;
  }
  std::string mc_tracks_{"SimParticles"};

  TClonesArray* cbm_global_tracks_{nullptr};
  TClonesArray* cbm_sts_tracks_{nullptr};
  TClonesArray* cbm_tof_hits_{nullptr};
  TClonesArray* cbm_fsd_hits_{nullptr};
  TClonesArray* cbm_fsd_hitmatch_{nullptr};
  TClonesArray* cbm_mc_tracks_{nullptr};

  CbmDigiManager* fDigiMan{nullptr};
  //TClonesArray* cbm_fsd_digimatch_ {nullptr};
  //TClonesArray* cbm_fsd_digis_ {nullptr};

  CbmMCDataManager* cbm_mc_manager_{nullptr};
  //CbmMCDataArray* cbm_mc_tracks_new_ {nullptr};
  CbmMCDataArray* cbm_fsd_points_new_{nullptr};

  AnalysisTree::HitDetector* fsd_hits_{nullptr};
  AnalysisTree::Matching* vtx_tracks_2_fsd_{
    nullptr};  // Matching of Reconstructed Global Tracks (extrapolated via CbmKFTrack to fixed Z) to FSD hits, the matching can be configured by cutting on chi2
  AnalysisTree::Matching* fsd_hits_2_mc_tracks_{
    nullptr};  // Matching of MC Tracks with highest weight (largest energy deposition) to FSD hits

  int i_mass2_{AnalysisTree::UndefValueInt};
  int i_qp_{AnalysisTree::UndefValueInt};
  int i_dx_{AnalysisTree::UndefValueInt};
  int i_t_{AnalysisTree::UndefValueInt};
  int i_l_{AnalysisTree::UndefValueInt};
  int i_edep_{AnalysisTree::UndefValueInt};
  int i_chi2_{AnalysisTree::UndefValueInt};
  int i_bestMatchedGTrack_{AnalysisTree::UndefValueInt};
  int i_multMC_{AnalysisTree::UndefValueInt};
  int i_topW_{AnalysisTree::UndefValueInt};
  int i_dxHP_{AnalysisTree::UndefValueInt};
  int i_dtHP_{AnalysisTree::UndefValueInt};

  int i_xpoint_{AnalysisTree::UndefValueInt};
  int i_pxpoint_{AnalysisTree::UndefValueInt};
  int i_phipoint_{AnalysisTree::UndefValueInt};
  int i_lengthpoint_{AnalysisTree::UndefValueInt};
  int i_tpoint_{AnalysisTree::UndefValueInt};
  int i_eloss_{AnalysisTree::UndefValueInt};
  int i_dist_middle_x_{AnalysisTree::UndefValueInt};
  int i_dist_middle_y_{AnalysisTree::UndefValueInt};


  Double_t fsdgtrack_minChi2_{0.};
  Double_t fsdgtrack_maxChi2_{0.};

  ClassDef(CbmFsdHitsConverter, 1)
};

#endif  // ANALYSIS_TREE_FSDHITSCONVERTER_H
