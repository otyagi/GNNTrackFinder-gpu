/* Copyright (C) 2011-2016 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak [committer] */

/*
 *=====================================================
 *
 *  CBM Level 1 Reconstruction
 *
 *  Authors: M.Zyzak
 *
 *  e-mail :
 *
 *=====================================================
 *
 *  Finds Particles: Lambdas, K0
 *
 */

#ifndef _CbmL1PFFitter_h_
#define _CbmL1PFFitter_h_

#include "CaSimd.h"
#include "KfFieldRegion.h"

#include <vector>

class CbmMvdHit;
class CbmStsHit;
class CbmStsTrack;
class CbmKFVertex;
class TClonesArray;

class CbmL1PFFitter {
 public:
  // A container for parameters of kf::FieldRegion
  struct PFFieldRegion {
    PFFieldRegion() {}
    PFFieldRegion(const cbm::algo::kf::FieldRegion<cbm::algo::kf::fvec>&, int i);
    void setFromL1FieldRegion(const cbm::algo::kf::FieldRegion<cbm::algo::kf::fvec>&, int i);
    void getL1FieldRegion(cbm::algo::kf::FieldRegion<cbm::algo::kf::fvec>&, int i);

    float fP[10]{0.};
  };

  CbmL1PFFitter();
  ~CbmL1PFFitter();

  //functions for fitting CbmStsTrack
  void Fit(std::vector<CbmStsTrack>& Tracks, const std::vector<CbmMvdHit>& vMvdHits,
           const std::vector<CbmStsHit>& vStsHits, const std::vector<int>& pidHypo);
  void Fit(std::vector<CbmStsTrack>& Tracks, const std::vector<int>& pidHypo);
  void CalculateFieldRegion(std::vector<CbmStsTrack>& Tracks, std::vector<PFFieldRegion>& Field);
  void CalculateFieldRegionAtLastPoint(std::vector<CbmStsTrack>& Tracks, std::vector<PFFieldRegion>& field);
  void GetChiToVertex(std::vector<CbmStsTrack>& Tracks, std::vector<PFFieldRegion>& field, std::vector<float>& chiToVtx,
                      CbmKFVertex& primVtx, float chiPrim = -1);

 private:
  void Initialize();
  int GetMvdStationIndex(const CbmMvdHit* h);
  int GetStsStationIndex(const CbmStsHit* h);

 private:
  bool fIsInitialised     = {false};    // is the fitter initialised
  int fNmvdStationsActive = {0};        // n MVD stations
  int fNstsStationsActive = {0};        // n STS stations
  TClonesArray* fMvdHitArray{nullptr};  // pointer to MVD hits
  TClonesArray* fStsHitArray{nullptr};  // pointer to STS hits
};
#endif
