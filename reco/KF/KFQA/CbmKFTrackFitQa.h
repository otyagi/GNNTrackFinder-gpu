/* Copyright (C) 2011-2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak [committer] */

/*
 *====================================================================
 *
 *  KF Fit performance 
 *
 *====================================================================
 */

#ifndef _CbmKFTrackFitQa_h_
#define _CbmKFTrackFitQa_h_

#include "CbmKFTrErrMCPoints.h"
#include "CbmKFTrack.h"
#include "CbmMCTrack.h"
#include "CbmMvdHit.h"
#include "CbmMvdPoint.h"
#include "CbmStsCluster.h"
#include "CbmStsDigi.h"
#include "CbmStsHit.h"
#include "CbmStsPoint.h"
#include "CbmStsTrack.h"
#include "CbmTrackMatch.h"
#include "FairTask.h"
#include "TClonesArray.h"
#include "TH1.h"
#include "TProfile.h"

#include <iostream>
#include <vector>

class CbmKFTrackFitQa : public FairTask {
 public:
  CbmKFTrackFitQa();
  ~CbmKFTrackFitQa();

  void StsHitMatch();
  void FillHitHistos();
  void FillHistoAtParticleVertex(CbmMCTrack* track_mc, CbmKFTrack* track_kf);
  void FillHistoAtFirstPoint(CbmKFTrErrMCPoints* mc_points, CbmMCTrack* track_mc, CbmKFTrack* track_kf);
  void KFWrite();
  void FindBackTracks(CbmKFTrErrMCPoints* mc_points, CbmMCTrack* track_mc, CbmKFTrack* track_kf, int iEvent);
  void FindBackTracks();
  void Save();

  void SetOutFile(TString outname) { outfileName = outname; }

  virtual InitStatus ReInit();
  virtual InitStatus Init();
  void Exec(Option_t* option);
  void Finish();

 private:
  // Arrays of points, tracks, etc

  TClonesArray* listStsPts;
  TClonesArray* listMvdPts;
  TClonesArray* listMCTracks;
  TClonesArray* listStsTracksMatch;
  TClonesArray* listStsTracks;
  TClonesArray* listStsHits;
  TClonesArray* listMvdHits;
  TClonesArray* listMvdHitMatches;
  TClonesArray* listStsClusters;
  TClonesArray* listStsDigi;
  TClonesArray* listStsDigiMatch;


  // Names of files
  TString outfileName;

  std::vector<int> vStsHitMatch;

  // histograms

  //hit parameters
  TH1F* res_STShit_x;
  TH1F* res_STShit_y;
  TH1F* pull_STShit_x;
  TH1F* pull_STShit_y;

  TH1F* res_MVDhit_x;
  TH1F* res_MVDhit_y;
  TH1F* pull_MVDhit_x;
  TH1F* pull_MVDhit_y;


  //track parameters at the particle vertex
  TH1F* res_AtPV_x;
  TH1F* res_AtPV_y;
  TH1F* res_AtPV_tx;
  TH1F* res_AtPV_ty;
  TH1F* res_AtPV_qp;

  TH1F* pull_AtPV_x;
  TH1F* pull_AtPV_y;
  TH1F* pull_AtPV_tx;
  TH1F* pull_AtPV_ty;
  TH1F* pull_AtPV_qp;

  //track parameters at the first MC point of the track
  TH1F* res_AtFP_x;
  TH1F* res_AtFP_y;
  TH1F* res_AtFP_tx;
  TH1F* res_AtFP_ty;
  TH1F* res_AtFP_qp;

  TH1F* pull_AtFP_x;
  TH1F* pull_AtFP_y;
  TH1F* pull_AtFP_tx;
  TH1F* pull_AtFP_ty;
  TH1F* pull_AtFP_qp;

  //Q detertmination quality
  TProfile* q_QA;
  TProfile* dp_p;

  TH1F* ggg;

  int Nback;
  //FILE *fBack;

  ClassDef(CbmKFTrackFitQa, 1);

 private:
  CbmKFTrackFitQa(const CbmKFTrackFitQa&);
  void operator=(const CbmKFTrackFitQa&);
};

#endif  // _CbmKFTrackFitQa_h_
