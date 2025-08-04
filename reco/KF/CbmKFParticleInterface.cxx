/* Copyright (C) 2014-2015 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak [committer] */

//-----------------------------------------------------------
//-----------------------------------------------------------

// Cbm Headers ----------------------
#include "CbmKFParticleInterface.h"

#include "CbmKFVertex.h"
#include "CbmL1PFFitter.h"
#include "CbmStsTrack.h"

//KF Particle headers
#include "KFPTrackVector.h"
#include "KFParticle.h"
#include "KFParticleSIMD.h"

//ROOT headers
#include "TClonesArray.h"  //to get arrays from the FairRootManager
#include "TMath.h"         //to calculate Prob function
#include "TStopwatch.h"    //to measure the time

//c++ and std headers
#include <cmath>
#include <iostream>
#include <vector>
using std::vector;

ClassImp(CbmKFParticleInterface);

void CbmKFParticleInterface::SetKFParticleFromStsTrack(CbmStsTrack* track, KFParticle* particle, Int_t pdg,
                                                       Bool_t firstPoint)
{
  vector<CbmStsTrack> vRTracks(1);
  vRTracks[0] = *track;

  CbmL1PFFitter fitter;
  vector<float> vChiToPrimVtx;
  CbmKFVertex kfVertex;

  vector<CbmL1PFFitter::PFFieldRegion> vField;
  fitter.GetChiToVertex(vRTracks, vField, vChiToPrimVtx, kfVertex, -3);

  KFPTrackVector tracks;
  tracks.Resize(1);
  //fill vector with tracks
  for (Int_t iTr = 0; iTr < 1; iTr++) {
    const FairTrackParam* parameters;

    if (firstPoint) {
      parameters = vRTracks[iTr].GetParamFirst();
    }
    else {
      parameters = vRTracks[iTr].GetParamLast();
    }

    float par[6] = {0.f};

    Double_t V[15] = {0.f};

    for (Int_t i = 0, iCov = 0; i < 5; i++) {
      for (Int_t j = 0; j <= i; j++, iCov++) {
        V[iCov] = parameters->GetCovariance(i, j);
      }
    }

    float a = parameters->GetTx(), b = parameters->GetTy(), qp = parameters->GetQp();

    Int_t q = 0;
    if (qp > 0.f) {
      q = 1;
    }
    if (qp < 0.f) {
      q = -1;
    }
    if (TMath::Abs(pdg) == 1000020030 || TMath::Abs(pdg) == 1000020040) {
      q *= 2;
    }

    float c2 = 1.f / (1.f + a * a + b * b);
    float pq = 1.f / qp * TMath::Abs(q);
    float p2 = pq * pq;
    float pz = sqrt(p2 * c2);
    float px = a * pz;
    float py = b * pz;

    float H[3] = {-px * c2, -py * c2, -pz * pq};

    par[0] = parameters->GetX();
    par[1] = parameters->GetY();
    par[2] = parameters->GetZ();
    par[3] = px;
    par[4] = py;
    par[5] = pz;

    float cxpz  = H[0] * V[3] + H[1] * V[6] + H[2] * V[10];
    float cypz  = H[0] * V[4] + H[1] * V[7] + H[2] * V[11];
    float capz  = H[0] * V[5] + H[1] * V[8] + H[2] * V[12];
    float cbpz  = H[0] * V[8] + H[1] * V[9] + H[2] * V[13];
    float cpzpz = H[0] * H[0] * V[5] + H[1] * H[1] * V[9] + H[2] * H[2] * V[14]
                  + 2 * (H[0] * H[1] * V[8] + H[0] * H[2] * V[12] + H[1] * H[2] * V[13]);

    float cov[21];
    cov[0]  = V[0];
    cov[1]  = V[1];
    cov[2]  = V[2];
    cov[3]  = 0.f;
    cov[4]  = 0.f;
    cov[5]  = 0.f;
    cov[6]  = V[3] * pz + a * cxpz;
    cov[7]  = V[4] * pz + a * cypz;
    cov[8]  = 0.f;
    cov[9]  = V[5] * pz * pz + 2.f * a * pz * capz + a * a * cpzpz;
    cov[10] = V[6] * pz + b * cxpz;
    cov[11] = V[7] * pz + b * cypz;
    cov[12] = 0.f;
    cov[13] = V[8] * pz * pz + a * pz * cbpz + b * pz * capz + a * b * cpzpz;
    cov[14] = V[9] * pz * pz + 2.f * b * pz * cbpz + b * b * cpzpz;
    cov[15] = cxpz;
    cov[16] = cypz;
    cov[17] = 0.f;
    cov[18] = capz * pz + a * cpzpz;
    cov[19] = cbpz * pz + b * cpzpz;
    cov[20] = cpzpz;

    for (Int_t iP = 0; iP < 6; iP++) {
      tracks.SetParameter(par[iP], iP, iTr);
    }
    for (Int_t iC = 0; iC < 21; iC++) {
      tracks.SetCovariance(cov[iC], iC, iTr);
    }
    for (Int_t iF = 0; iF < 10; iF++) {
      tracks.SetFieldCoefficient(vField[iTr].fP[iF], iF, iTr);
    }
    tracks.SetId(1, iTr);
    tracks.SetPDG(pdg, iTr);
    tracks.SetQ(q, iTr);
    tracks.SetPVIndex(-1, iTr);
  }

  int_v pdgSIMD(pdg);
  unsigned int index = 0;
  uint_v indexSIMD(index);

  KFParticleSIMD particleSIMD(tracks, indexSIMD, pdgSIMD);
  particleSIMD.GetKFParticle(*particle, 0);

  particle->NDF()  = track->GetNDF();
  particle->Chi2() = track->GetChiSq();
}

void CbmKFParticleInterface::ExtrapolateTrackToPV(const CbmStsTrack* track, CbmVertex* pv, FairTrackParam* paramAtPV,
                                                  float& chiPrim)
{
  vector<CbmStsTrack> vRTracks(1);
  vRTracks[0] = *track;

  CbmL1PFFitter fitter;
  vector<float> vChiToPrimVtx;
  CbmKFVertex kfVertex;
  assert(pv);
  if (pv) {
    kfVertex = CbmKFVertex(*pv);
  }

  vector<CbmL1PFFitter::PFFieldRegion> vField;
  fitter.GetChiToVertex(vRTracks, vField, vChiToPrimVtx, kfVertex, 1000000.f);

  chiPrim    = vChiToPrimVtx[0];
  *paramAtPV = *(vRTracks[0].GetParamFirst());
}
