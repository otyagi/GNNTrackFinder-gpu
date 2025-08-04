/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer], Norbert Herrmann */
#include "CbmTimesliceRecoTracks.h"

#include "CbmEvent.h"             // For CbmEvent
#include "CbmGlobalTrack.h"       // for CbmGlobalTrack
#include "CbmHit.h"               // for HitType, kMUCHPIXELHIT, kRICHHIT
#include "CbmPixelHit.h"          // for CbmPixelHit
#include "CbmRichRing.h"          // for CbmRichRing
#include "CbmStsTrack.h"          // for CbmStsTrack
#include "CbmTrack.h"             // for CbmTrack
#include <CbmTimesliceManager.h>  // for CbmTimesliceManager

#include <FairRootManager.h>  // for FairRootManager
#include <FairTask.h>         // for FairTask, InitStatus, kERROR, kSUCCESS
#include <FairTrackParam.h>   // for FairTrackParam
#include <Logger.h>           // for LOG, Logger

#include <Rtypes.h>               // for ClassImp
#include <TClonesArray.h>         // for TClonesArray
#include <TEveManager.h>          // for TEveManager, gEve
#include <TEvePathMark.h>         // for TEvePathMark
#include <TEveTrack.h>            // for TEveTrackList, TEveTrack
#include <TEveTrackPropagator.h>  // for TEveTrackPropagator
#include <TEveVector.h>           // for TEveVector, TEveVectorT
#include <TGenericClassInfo.h>    // for TGenericClassInfo
#include <TParticle.h>            // for TParticle
#include <TString.h>              // for TString
#include <TVector3.h>             // for TVector3

#include <stdio.h>   // for sprintf
#include <string.h>  // for strcmp

// -------------------------------------------------------------------------
InitStatus CbmTimesliceRecoTracks::Init()
{
  LOG(debug) << "CbmTimesliceRecoTracks::Init()";
  FairRootManager* fManager = FairRootManager::Instance();

  fCbmEvents     = dynamic_cast<TClonesArray*>(fManager->GetObject("CbmEvent"));
  fGlobalTracks  = dynamic_cast<TClonesArray*>(fManager->GetObject("GlobalTrack"));
  fStsTracks     = dynamic_cast<TClonesArray*>(fManager->GetObject("StsTrack"));
  fMvdHits       = dynamic_cast<TClonesArray*>(fManager->GetObject("MvdHit"));
  fStsHits       = dynamic_cast<TClonesArray*>(fManager->GetObject("StsHit"));
  fRichRings     = dynamic_cast<TClonesArray*>(fManager->GetObject("RichRing"));
  fRichHits      = dynamic_cast<TClonesArray*>(fManager->GetObject("RichHit"));
  fMuchPixelHits = dynamic_cast<TClonesArray*>(fManager->GetObject("MuchPixelHit"));
  fMuchTracks    = dynamic_cast<TClonesArray*>(fManager->GetObject("MuchTrack"));
  fTrdHits       = dynamic_cast<TClonesArray*>(fManager->GetObject("TrdHit"));
  fTrdTracks     = dynamic_cast<TClonesArray*>(fManager->GetObject("TrdTrack"));
  fTofHits       = dynamic_cast<TClonesArray*>(fManager->GetObject("TofHit"));
  fTofTracks     = dynamic_cast<TClonesArray*>(fManager->GetObject("TofTrack"));

  if (nullptr == fTofTracks) {
    LOG(warn) << "TofTracks not found";
  }

  if (nullptr == fCbmEvents) {
    LOG(fatal) << "CbmTimesliceRecoTracks::Init() => CbmEvents branch not found! Task will be deactivated";
    SetActive(kFALSE);
  }

  LOG(debug1) << "CbmTimesliceRecoTracks::Init() get track list" << fStsTracks;
  LOG(debug1) << "CbmTimesliceRecoTracks::Init()  create propagator";
  fEventManager = CbmTimesliceManager::Instance();
  LOG(debug1) << "CbmTimesliceRecoTracks::Init() got instance of CbmTimesliceManager ";

  if (IsActive()) { return kSUCCESS; }
  else {
    return kERROR;
  }
}

void CbmTimesliceRecoTracks::HandlePixelHit(TEveTrack* eveTrack, Int_t& n, const CbmPixelHit* hit, TEveVector* pMom = 0)
{
  eveTrack->SetPoint(n, hit->GetX(), hit->GetY(), hit->GetZ());
  TEveVector pos = TEveVector(hit->GetX(), hit->GetY(), hit->GetZ());
  TEvePathMark path;
  path.fV    = pos;
  path.fTime = 0;

  if (pMom) path.fP = *pMom;

  eveTrack->AddPathMark(path);
  ++n;
}

void CbmTimesliceRecoTracks::HandleTrack(TEveTrack* eveTrack, Int_t& n, const CbmTrack* recoTrack)
{
  Int_t nofHits = recoTrack->GetNofHits();

  for (Int_t i = 0; i < nofHits; ++i) {
    HitType hitType             = recoTrack->GetHitType(i);
    Int_t hitIndex              = recoTrack->GetHitIndex(i);
    const CbmPixelHit* pixelHit = 0;
    LOG(debug4) << "HandleTrack: hit " << i << ", type " << hitType << ", ind " << hitIndex;

    switch (hitType) {
      case kRICHHIT: pixelHit = static_cast<const CbmPixelHit*>(fRichHits->At(hitIndex)); break;

      case kMUCHPIXELHIT: pixelHit = static_cast<const CbmPixelHit*>(fMuchPixelHits->At(hitIndex)); break;

      case kTRDHIT: pixelHit = static_cast<const CbmPixelHit*>(fTrdHits->At(hitIndex)); break;

      case kTOFHIT: pixelHit = static_cast<const CbmPixelHit*>(fTofHits->At(hitIndex)); break;
      default: LOG(warn) << "Pixel type " << hitType << " not supported.";
    }

    if (0 != pixelHit) HandlePixelHit(eveTrack, n, pixelHit);
  }
}

void CbmTimesliceRecoTracks::HandleStsTrack(TEveTrack* eveTrack, Int_t& n, const CbmStsTrack* stsTrack)
{
  for (Int_t i = 0; i < stsTrack->GetNofMvdHits(); ++i) {
    const CbmPixelHit* hit = static_cast<const CbmPixelHit*>(fMvdHits->At(stsTrack->GetMvdHitIndex(i)));

    if (0 == n) {
      TVector3 mom3;
      stsTrack->GetParamFirst()->Momentum(mom3);
      TEveVector mom = TEveVector(mom3.X(), mom3.Y(), mom3.Z());
      HandlePixelHit(eveTrack, n, hit, &mom);
    }
    else
      HandlePixelHit(eveTrack, n, hit);
  }

  for (Int_t i = 0; i < stsTrack->GetNofStsHits(); ++i) {
    const CbmPixelHit* hit = static_cast<const CbmPixelHit*>(fStsHits->At(stsTrack->GetStsHitIndex(i)));

    if (0 == n) {
      TVector3 mom3;
      stsTrack->GetParamFirst()->Momentum(mom3);
      TEveVector mom = TEveVector(mom3.X(), mom3.Y(), mom3.Z());
      HandlePixelHit(eveTrack, n, hit, &mom);
    }
    else
      HandlePixelHit(eveTrack, n, hit);
  }
}

void CbmTimesliceRecoTracks::Exec(Option_t* /*option*/)
{
  if (0 < fCbmEvents->GetEntriesFast()) {
    /// When loading a new TS, load the first event if possible
    GotoEvent(0);
  }
}
void CbmTimesliceRecoTracks::GotoEvent(uint32_t uEventIdx)
{
  LOG(debug3) << "CbmTimesliceRecoTracks::GotoEvent " << uEventIdx;

  if (fCbmEvents->GetEntriesFast() <= static_cast<Int_t>(uEventIdx)) {
    LOG(fatal) << "CbmTimesliceRecoTracks::GotoEvent() => Failure, tried to load event " << uEventIdx << " while only "
               << fCbmEvents->GetEntriesFast() << " events available in this TS!!!";
  }

  fEventIdx = uEventIdx;

  if (CbmTimesliceManager::Instance()->GetClearHandler()) {  //
    Reset();
  }

  CbmEvent* event = dynamic_cast<CbmEvent*>(fCbmEvents->At(uEventIdx));

  Int_t nofGlobalTracks = event->GetNofData(ECbmDataType::kGlobalTrack);
  LOG(debug3) << "CbmTimesliceRecoTracks for nofGlobalTracks=" << nofGlobalTracks;

  for (Int_t iGloTrk = 0; iGloTrk < nofGlobalTracks; ++iGloTrk) {
    LOG(debug3) << "CbmTimesliceRecoTracks::GotoEvent " << iGloTrk;
    Int_t trkId                       = event->GetIndex(ECbmDataType::kGlobalTrack, iGloTrk);
    const CbmGlobalTrack* globalTrack = dynamic_cast<const CbmGlobalTrack*>(fGlobalTracks->At(trkId));
    Int_t stsId                       = globalTrack->GetStsTrackIndex();
    Int_t richId                      = globalTrack->GetRichRingIndex();
    Int_t muchId                      = globalTrack->GetMuchTrackIndex();
    Int_t trdId                       = globalTrack->GetTrdTrackIndex();
    Int_t tofId                       = globalTrack->GetTofTrackIndex();
    Int_t tofHitId                    = globalTrack->GetTofHitIndex();
    LOG(debug4) << "GloTrk " << iGloTrk << " trkId " << trkId << " stsId " << stsId << " trdId " << trdId << " tofId "
                << tofId << ", " << tofHitId;

    // Global Track fit display
    /*
    FIXME: Not working yet, just printout
    globalTrack->Print();
    const FairTrackParam* tParF = globalTrack->GetParamFirst();
    tParF->Print();
    */

    CbmStsTrack* stsTrack = nullptr;
    if (stsId > -1) {
      stsTrack = dynamic_cast<CbmStsTrack*>(fStsTracks->At(stsId));
    }

    Int_t n   = 0;
    Int_t pdg = 0;
    if (nullptr != stsTrack) {
      pdg = stsTrack->GetPidHypo();
    }
    TParticle P;
    P.SetPdgCode(pdg);
    fTrList             = GetTrGroup(&P);
    TEveTrack* eveTrack = new TEveTrack(&P, pdg, fTrPr);
    if (fbPdgColorTrack && 0 != pdg) {  //
      fEventManager->Color(pdg);
    }
    else {
      eveTrack->SetLineColor(kRed);
    }
    if (nullptr != stsTrack) {
      LOG(debug3) << "HandleStsTrack " << stsId;
      HandleStsTrack(eveTrack, n, stsTrack);
    }
    else {
      LOG(debug3) << "No Sts track";
    }

    if (-1 < richId) {
      const CbmRichRing* r  = dynamic_cast<const CbmRichRing*>(fRichRings->At(richId));
      const CbmPixelHit* rh = dynamic_cast<const CbmPixelHit*>(fRichHits->At(r->GetHit(0)));
      CbmPixelHit h(*rh);
      h.SetX(r->GetCenterX());
      h.SetY(r->GetCenterY());
      HandlePixelHit(eveTrack, n, &h);
    }
    else if (-1 < muchId) {
      HandleTrack(eveTrack, n, dynamic_cast<const CbmTrack*>(fMuchTracks->At(muchId)));
    }

    LOG(debug3) << "HandleTrdTrack " << trdId;
    if (-1 < trdId) {
      if (nullptr == fTrdTracks) {
        LOG(error) << GetName() << ": No TrdTrack array, return";
      }
      else {
        const CbmTrack* recoTrack = dynamic_cast<const CbmTrack*>(fTrdTracks->At(trdId));
        LOG(debug3) << "HandleTrdTrack " << trdId << ", " << recoTrack;
        if (nullptr != recoTrack) HandleTrack(eveTrack, n, recoTrack);
      }
    }

    if (-1 < tofId) {
      if (nullptr == fTofTracks) {
        LOG(error) << GetName() << ": No TofTrack array, return";
      }
      else {
        const CbmTrack* recoTrack = dynamic_cast<const CbmTrack*>(fTofTracks->At(tofId));
        LOG(debug3) << "HandleTofTrack " << tofId << ", " << recoTrack;
        if (nullptr != recoTrack) HandleTrack(eveTrack, n, recoTrack);
      }
    }
    else if (-1 < tofHitId)
      HandlePixelHit(eveTrack, n, dynamic_cast<const CbmPixelHit*>(fTofHits->At(tofHitId)));

    /// Draw Markers of all hits participating in the track
    eveTrack->SetRnrPoints(kTRUE);

    fTrList->AddElement(eveTrack);
    LOG(debug3) << "track added " << eveTrack->GetName();
  }

  gEve->Redraw3D(kFALSE);
}
// -------------------------------------------------------------------------
void CbmTimesliceRecoTracks::Reset()
{
  for (Int_t i = 0; i < fEveTrList->GetEntriesFast(); i++) {
    TEveTrackList* ele = (TEveTrackList*) fEveTrList->At(i);
    LOG(debug3) << GetName() << ": remove elements from " << ele->GetName();
    gEve->RemoveElement(ele, fEventManager);
  }
  fEveTrList->Clear();
}

TEveTrackList* CbmTimesliceRecoTracks::GetTrGroup(TParticle* P)
{
  char name_buf[128];
  snprintf(name_buf, 128, "reco_%s", P->GetName());
  fTrList = 0;
  for (Int_t i = 0; i < fEveTrList->GetEntriesFast(); i++) {
    TEveTrackList* TrListIn = (TEveTrackList*) fEveTrList->At(i);
    if (strcmp(TrListIn->GetName(), name_buf) == 0) {
      fTrList = TrListIn;
      break;
    }
  }
  if (fTrList == 0) {
    fTrPr   = new TEveTrackPropagator();
    fTrList = new TEveTrackList(name_buf, fTrPr);
    fTrList->SetMainColor(fEventManager->Color(P->GetPdgCode()));
    fEveTrList->Add(fTrList);
    gEve->AddElement(fTrList, fEventManager);
    fTrList->SetRnrLine(kTRUE);
  }
  return fTrList;
}

ClassImp(CbmTimesliceRecoTracks)
