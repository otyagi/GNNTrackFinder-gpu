/* Copyright (C) 2006-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov, Volker Friese, Denis Bertini [committer] */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction 
 *  
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de 
 *
 *====================================================================
 *
 *  CbmL1StsTrackFinder source file
 *
 *====================================================================
 */
#include "CbmL1StsTrackFinder.h"

#include "CbmEvent.h"
#include "CbmKfUtil.h"
#include "CbmStsHit.h"
#include "CbmStsTrack.h"
#include "FairHit.h"
#include "FairMCPoint.h"
#include "FairRootManager.h"
#include "TClonesArray.h"
#include "TDatabasePDG.h"

#include <iostream>
#include <vector>

using std::vector;

ClassImp(CbmL1StsTrackFinder)

  // -----   Default constructor   -------------------------------------------
  CbmL1StsTrackFinder::CbmL1StsTrackFinder()
{
  fName = "STS Track Finder L1";
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmL1StsTrackFinder::~CbmL1StsTrackFinder() {}
// -------------------------------------------------------------------------


// -----   Public method Init   --------------------------------------------
void CbmL1StsTrackFinder::Init() {}
// -------------------------------------------------------------------------


// -----   Copy tracks to output array   -----------------------------------
Int_t CbmL1StsTrackFinder::CopyL1Tracks(CbmEvent* event)
{
  CbmL1* L1 = CbmL1::Instance();
  if (!L1) return 0;

  Int_t trackIndex = fTracks->GetEntriesFast();
  Int_t nTracks    = 0;
  LOG(debug) << "Copy L1 tracks : " << L1->fvRecoTracks.size() << " tracks in L1";
  for (vector<CbmL1Track>::iterator it = L1->fvRecoTracks.begin(); it != L1->fvRecoTracks.end(); ++it) {
    CbmL1Track& T = *it;
    new ((*fTracks)[trackIndex]) CbmStsTrack();
    nTracks++;
    if (event) event->AddData(ECbmDataType::kStsTrack, trackIndex);
    CbmStsTrack* t = dynamic_cast<CbmStsTrack*>(fTracks->At(trackIndex++));
    t->SetFlag(0);
    t->SetParamFirst(cbm::kf::ConvertTrackParam(T));
    t->SetParamLast(cbm::kf::ConvertTrackParam(T.TLast));
    t->SetChiSq(T.GetChiSq());
    t->SetNDF(T.GetNdf());
    t->SetChiSqTime(T.GetChiSqTime());
    t->SetNdfTime(T.GetNdfTime());
    t->SetPidHypo(T.GetQp() >= 0 ? 211 : -211);
    t->SetStartTime(T.Tpv.GetTime());
    t->SetStartTimeError(T.Tpv.GetTimeError());
    t->SetFirstHitTime(T.GetTime());
    t->SetFirstHitTimeError(T.GetTimeError());
    t->SetLastHitTime(T.TLast.GetTime());
    t->SetLastHitTimeError(T.TLast.GetTimeError());

    for (vector<int>::iterator ih = it->Hits.begin(); ih != it->Hits.end(); ++ih) {
      CbmL1HitId& h = L1->fvExternalHits[*ih];
      if (h.detId == 0) {
        t->AddMvdHit(h.hitId);
      }
      else if (h.detId == 1) {
        t->AddStsHit(h.hitId);
      }
    }
  }

  return nTracks;
}
// -------------------------------------------------------------------------


// -----   Public method DoFind   ------------------------------------------
Int_t CbmL1StsTrackFinder::DoFind()
{

  if (!fTracks) {
    LOG(error) << "-E- CbmL1StsTrackFinder::DoFind: "
               << "Track array missing! ";
    return -1;
  }

  CbmL1* L1 = CbmL1::Instance();
  if (!L1) return 0;

  L1->Reconstruct();
  int ntracks = CopyL1Tracks();

  return ntracks;
}
// -------------------------------------------------------------------------


// -----   Track finding in one event   ------------------------------------
Int_t CbmL1StsTrackFinder::FindTracks(CbmEvent* event)
{

  CbmL1* l1 = CbmL1::Instance();
  if (!l1) return 0;

  l1->Reconstruct(event);
  int nTracks = CopyL1Tracks(event);

  return nTracks;
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void SetDefaultParticlePDG(int pdg = 211)
{
  /// set a default particle mass for the track fit
  /// it is used during reconstruction for the multiple scattering estimation
  CbmL1* l1 = CbmL1::Instance();
  if (!l1 || !l1->fpAlgo) {
    LOG(fatal) << "L1 instance doesn't exist or is not initialised";
    return;
  }
  auto* p = TDatabasePDG::Instance()->GetParticle(pdg);
  if (!p) {
    LOG(fatal) << "Particle with pdg " << pdg << " doesn't exist";
    return;
  }
  l1->fpAlgo->SetDefaultParticleMass(p->Mass());
}
// -------------------------------------------------------------------------
