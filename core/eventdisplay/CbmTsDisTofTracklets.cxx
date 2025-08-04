/* Copyright (C) 2023 PI-UHd, Heidelberg
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer], Pierre-Alain Loizeau */

#define TOFDisplay 1  // =1 means active, other: without Label and not relying on TEvePointSet
#define TOFTtErr 1    // =1 means active, other: not relying on VelocityError of CbmTofTracklet

#include "CbmTsDisTofTracklets.h"

#include "CbmEvent.h"             // For CbmEvent
#include "CbmTimesliceManager.h"  // for CbmTimesliceManager
#include "CbmTofHit.h"            // for CbmTofHit
#include "CbmTofTracklet.h"       // for CbmTofTracklet

#include <FairEventManager.h>  // for FairEventManager
#include <FairRootManager.h>   // for FairRootManager
#include <FairTask.h>          // for FairTask, InitStatus, kERROR, kSUCCESS
#include <Logger.h>            // for LOG, Logger

#include <Rtypes.h>               // for ClassImp
#include <TClonesArray.h>         // for TClonesArray
#include <TEveElement.h>          // for TEveElementList
#include <TEveManager.h>          // for TEveManager, gEve
#include <TEvePathMark.h>         // for TEvePathMark
#include <TEvePointSet.h>         // for TEvePointSetArray, TEvePointSet
#include <TEveTrack.h>            // for TEveTrack, TEveTrackList
#include <TEveTrackPropagator.h>  // for TEveTrackPropagator
#include <TEveVSDStructs.h>       // for TEveRecTrack
#include <TEveVector.h>           // for TEveVector, TEveVectorT
#include <TGLAnnotation.h>
#include <TGLViewer.h>
#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TObjArray.h>          // for TObjArray
#include <TParticle.h>          // for TParticle
#include <TString.h>            // for Form, TString

#include <string.h>  // for strcmp

ClassImp(CbmTsDisTofTracklets);
CbmTsDisTofTracklets* CbmTsDisTofTracklets::fInstance = nullptr;

//static TGLAnnotation* anne;
static TGLAnnotation* annt;
static CbmTimesliceManager* fTsManager = nullptr;
static FairEventManager* fEventManager = nullptr;

// -----   Default constructor   -------------------------------------------
CbmTsDisTofTracklets::CbmTsDisTofTracklets() : FairTask("CbmTsDisTofTracklets", 0)
{
  if (!fInstance) fInstance = this;
}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmTsDisTofTracklets::CbmTsDisTofTracklets(const char* name, Int_t iVerbose, Bool_t renderP, Bool_t renderT)
  : FairTask(name, iVerbose)
  , fEveTrList(new TObjArray(16))
  , fEvePSList(new TObjArray(8))
  , fRenderP(renderP)
  , fRenderT(renderT)
{
  if (!fInstance) fInstance = this;
}
// -------------------------------------------------------------------------
InitStatus CbmTsDisTofTracklets::Init()
{
  LOG(info) << "CbmTsDisTofTracklets::Init()";
  FairRootManager* fManager = FairRootManager::Instance();
  fCbmEvents                = dynamic_cast<TClonesArray*>(fManager->GetObject("CbmEvent"));
  fTrackList                = dynamic_cast<TClonesArray*>(fManager->GetObject("TofTracklets"));
  if (fTrackList == 0) {
    LOG(warn) << "CbmTsDisTofTracklets::Init() branch " << GetName() << " Not found! Task will be deactivated ";
    SetActive(kFALSE);
  }
  LOG(debug1) << "CbmTsDisTofTracklets::Init() get track list" << fTrackList;
  /*
  LOG(debug1) << "CbmTsDisTofTracklets::Init() create propagator";
  fEventManager = FairEventManager::Instance();
  fEventManager = CbmTimesliceManager::Instance();
  LOG(debug1) << "CbmTsDisTofTracklets::Init() get instance of FairEventManager ";
  fEvent         = "Current Event";
  MinEnergyLimit = fEventManager->GetEvtMinEnergy();
  MaxEnergyLimit = fEventManager->GetEvtMaxEnergy();
  PEnergy        = 0;
  */
  fTsManager    = CbmTimesliceManager::Instance();
  fEventManager = FairEventManager::Instance();

  if (nullptr == fEventManager) {
    LOG(warn) << GetName() << ": no FairEventManager found, use TsManager ";
  }
  else
    LOG(info) << "CbmTsDisTofTracklets::Init() got instance of FairEventManager ";

  if (IsActive()) {
    return kSUCCESS;
  }
  else {
    return kERROR;
  }
}
// -------------------------------------------------------------------------
void CbmTsDisTofTracklets::Exec(Option_t* option)
{
  LOG(debug2) << " CbmTsDisTofTracklets::Exec starting with verbosity " << fVerbose << " and option " << option;
  if (0 < fCbmEvents->GetEntriesFast()) {
    /// When loading a new TS, load the first event if possible
    GotoEvent(0);
  }
}
void CbmTsDisTofTracklets::GotoEvent(uint32_t uEventIdx)
{
  if (IsActive()) {

    fEventIdx = uEventIdx;

    if (CbmTimesliceManager::Instance()->GetClearHandler()) {  //
      Reset();
    }

    CbmEvent* event = dynamic_cast<CbmEvent*>(fCbmEvents->At(uEventIdx));

    Int_t nofTofTracks = event->GetNofData(ECbmDataType::kTofTracklet);
    LOG(debug3) << GetName() << " : nofTofTracks " << nofTofTracks << " in event " << uEventIdx;

    Reset();

    CbmTofTracklet* tr;
    const Double_t* point;
    CbmTofHit* hit;
    Int_t TMul[10] = {10 * 0};  //FIXME - don't use constants in code

    for (Int_t iOpt = 0; iOpt < 2; iOpt++)
      for (Int_t iTrk = 0; iTrk < nofTofTracks; ++iTrk) {
        Int_t trkId = event->GetIndex(ECbmDataType::kTofTracklet, iTrk);
        tr          = dynamic_cast<CbmTofTracklet*>(fTrackList->At(trkId));
        int i       = iTrk;
        //      for (Int_t i = 0; i < fTrackList->GetEntriesFast(); i++) {
        //        LOG(debug4) << "CbmTsDisTofTracklets::Exec " << i;
        //        tr = (CbmTofTracklet*) fTrackList->At(i);
        if (nullptr == tr) continue;
        Int_t Np = tr->GetNofHits();

#if TOFDisplay == 1  //List for TEvePointSets
        if (iOpt == 0) TMul[Np]++;
        fPSList = GetPSGroup(Np, iOpt);
#endif

        fTrList          = GetTrGroup(tr->GetNofHits(), iOpt);
        TParticle* P     = new TParticle();
        TEveTrack* track = new TEveTrack(P, tr->GetPidHypo(), fTrPr);
        Int_t iCol       = Np;
        if (iCol > 4) iCol++;
        track->SetAttLineAttMarker(fTrList);  //set defaults
        track->SetLineColor(iCol);
        track->SetMarkerColor(iCol);
        track->SetMarkerSize(2.);
        //track->SetMarkerDraw(kTRUE);

        track->SetPoint(0, tr->GetFitX(0.), tr->GetFitY(0.), 0.);  //insert starting point
        TEveVector pos0     = TEveVector(tr->GetFitX(0.), tr->GetFitY(0.), 0.);
        TEvePathMark* path0 = new TEvePathMark();
        path0->fV           = pos0;
        track->AddPathMark(*path0);

        Double_t pbuf[3], vbuf[3];
        TEveRecTrack rt;
        rt.fIndex = i;
        pbuf[0]   = 0.;
        pbuf[1]   = 0.;
        pbuf[2]   = 1. / tr->GetTt();  // velocity
        rt.fP.Set(pbuf);
        vbuf[0] = tr->GetFitX(0.);
        vbuf[1] = tr->GetFitY(0.);
        vbuf[2] = 0.;
        rt.fV.Set(vbuf);
        track->SetName(Form("TEveTrack %d", rt.fIndex));

        //track->SetStdTitle();
        //      Double_t beta, beta_err, res_x, res_y, res_z, res_t;
        Double_t beta, beta_err, res_x, res_y, res_t;
        switch (iOpt) {
          case 0: track->SetStdTitle(); break;
          case 1:
#if TOFDisplay == 1  //setting content of label depending on available information
            beta = (1 / tr->GetTt()) / 29.98;
#if TOFTtErr == 1
            beta_err = beta * (tr->GetTtErr() / tr->GetTt());
            track->SetTitle(
              Form("%s\nChiSqDoF = %2.2f\nbeta = %1.3f +/- %1.3f", track->GetName(), tr->GetChiSq(), beta, beta_err));
#else
            track->SetTitle(Form("%s\nChiSqDoF = %2.2f\nbeta = %1.3f", track->GetName(), tr->GetChiSq(), beta));
#endif
#else
            track->SetStdTitle();
#endif
            break;
        }

#if TOFDisplay == 1
        // initialize TEvePointSet to show Datapoints belonging to track
        TEvePointSetArray* psa = new TEvePointSetArray(Form("TEveTrack Points %d", i), "");
        psa->SetMarkerColor(iCol);
        psa->SetMarkerSize(1.6);
        if (iOpt == 0)
          psa->SetMarkerStyle(4);
        else
          psa->SetMarkerStyle(5);
        psa->InitBins("Hits", Np, 0.5, Np + 0.5);
#endif

        for (Int_t n = 0; n < Np; n++) {
          switch (iOpt) {
            case 0: point = tr->GetPoint(n);  //pointer to member variable so GetFitPoint() would also change GetPoint()
#if TOFDisplay == 1
              // following belongs to filling and labeling of PointSetArray
              psa->Fill(point[0], point[1], point[2], n + 1);
              hit   = tr->GetTofHitPointer(n);
              res_x = (point[0] - tr->GetFitX(point[2])) / hit->GetDx();
              res_y = (point[1] - tr->GetFitY(point[2])) / hit->GetDy();
              res_t = (point[3] - tr->GetFitT(point[2])) / hit->GetTimeError();
              //          res_z=0;
              psa->GetBin(n + 1)->SetTitle(Form("%s\nPointId = %d\nResiduals:\nX = %1.3f\nY = %1.3f\nT = %1.3f",
                                                track->GetName(), tr->GetHitIndex(n), res_x, res_y, res_t));
#endif
              break;
            case 1:  // represent fit
              point = tr->GetFitPoint(n);
              psa->Fill(point[0], point[1], point[2], n + 1);
              break;
          }
          track->SetPoint(n + 1, point[0], point[1], point[2]);
          /*
          LOG(info) << Form("   CbmTsDisTofTracklets::SetPoint Opt %d, n %d, %6.2f, %6.2f, %6.2f, %6.2f ", iOpt, n, point[0], point[1],
                              point[2], point[3]);
          */
          TEveVector pos     = TEveVector(point[0], point[1], point[2]);
          TEvePathMark* path = new TEvePathMark();
          path->fV           = pos;
          path->fTime        = point[3];
          if (n == 0) {
            TEveVector Mom = TEveVector(P->Px(), P->Py(), P->Pz());
            path->fP       = Mom;
          }

          track->AddPathMark(*path);
          if (iOpt == 1 && n == Np - 1) {
            //LOG(info) << "Add extrapolation by 20%, TBD ";
            Double_t pointL[3];
            pointL[2] = point[2] * 1.2;
            pointL[0] = tr->GetFitX(pointL[2]);
            pointL[1] = tr->GetFitY(pointL[2]);
            track->SetPoint(n + 2, pointL[0], pointL[1], pointL[2]);
            TEveVector posL     = TEveVector(pointL[0], pointL[1], pointL[2]);
            TEvePathMark* pathL = new TEvePathMark();
            pathL->fV           = posL;
            track->AddPathMark(*pathL);
          }

          LOG(debug4) << "Path marker added " << path;
        }
#if TOFDisplay == 1
        if (iOpt >= 0) {
          fPSList->AddElement(psa);
        }
#endif
        track->SortPathMarksByTime();
        fTrList->AddElement(track);
        LOG(debug3) << i << ". track added: " << track->GetName();
      }
    for (Int_t i = 0; i < fEveTrList->GetEntriesFast(); i++) {
      // TEveTrackList *TrListIn=( TEveTrackList *) fEveTrList->At(i);
      //TrListIn->FindMomentumLimits(TrListIn, kFALSE);
    }

    //fEventManager->SetEvtMaxEnergy(MaxEnergyLimit);
    //fEventManager->SetEvtMinEnergy(MinEnergyLimit);
    TString cEventInfo;
    if (nullptr == fTsManager)
      cEventInfo = Form("ev# %d ", fEventManager->GetCurrentEvent());
    else
      cEventInfo = Form("ev# %d ", fTsManager->GetCurrentEvent());

    TString cTrackInfo = "TrklMul: ";  // to be completed while building the display
    for (Int_t i = 9; i > 0; i--)
      if (TMul[i] > 0) cTrackInfo += Form("M%d %d/", i, TMul[i]);

    TGLViewer* v = gEve->GetDefaultGLViewer();
    /*
    if (nullptr != anne) anne->SetText(cEventInfo);
    else
      anne = new TGLAnnotation(v, cEventInfo, 0.01, 0.95);
    */
    if (nullptr != annt)
      annt->SetText(cTrackInfo);
    else
      annt = new TGLAnnotation(v, cTrackInfo, 0.01, 0.78);
    //anne->SetTextSize(0.03);  // % of window diagonal
    //anne->SetTextColor(4);
    annt->SetTextSize(0.03);  // % of window diagonal
    annt->SetTextColor(4);

    gEve->Redraw3D(kFALSE);
    //gEve->DoRedraw3D();
    //gEve->Redraw3D(kTRUE);
  }
}
// -----   Destructor   ----------------------------------------------------
CbmTsDisTofTracklets::~CbmTsDisTofTracklets() {}
// -------------------------------------------------------------------------
void CbmTsDisTofTracklets::SetParContainers() {}

// -------------------------------------------------------------------------
void CbmTsDisTofTracklets::Finish() {}
// -------------------------------------------------------------------------
void CbmTsDisTofTracklets::Reset()
{
  for (Int_t i = 0; i < fEveTrList->GetEntriesFast(); i++) {
    TEveTrackList* ele = (TEveTrackList*) fEveTrList->At(i);
    //gEve->RemoveElement(ele, fEventManager);
    LOG(debug3) << GetName() << ": remove Tr elements from " << ele->GetName();
    ele->Clear();
    gEve->RemoveElement(ele, fTsManager);
  }
  fEveTrList->Clear();
#if TOFDisplay == 1
  for (Int_t i = 0; i < fEvePSList->GetEntriesFast(); i++) {
    TEveElementList* ele = (TEveElementList*) fEvePSList->At(i);
    //gEve->RemoveElement(ele, fEventManager);
    LOG(debug3) << GetName() << ": remove PS elements from " << ele->GetName();
    ele->Clear();
    gEve->RemoveElement(ele, fTsManager);
  }
  fEvePSList->Clear();
#endif
}

Char_t* gs;
TEveTrackList* CbmTsDisTofTracklets::GetTrGroup(Int_t ihmul, Int_t iOpt)
{
  switch (iOpt) {
    case 0: gs = Form("Trkl_hmul%d", ihmul); break;
    case 1: gs = Form("FTrkl_hmul%d", ihmul); break;
  }
  fTrList = 0;
  for (Int_t i = 0; i < fEveTrList->GetEntriesFast(); i++) {
    TEveTrackList* TrListIn = (TEveTrackList*) fEveTrList->At(i);
    if (strcmp(TrListIn->GetName(), gs) == 0) {
      fTrList = TrListIn;
      break;
    }
  }
  if (fTrList == 0) {
    fTrPr      = new TEveTrackPropagator();
    fTrList    = new TEveTrackList(gs, fTrPr);
    Int_t iCol = ihmul;
    if (iCol > 4) iCol++;
    fTrList->SetMainColor(iCol);
    fEveTrList->Add(fTrList);
#if TOFDisplay == 1
    if (iOpt == 1) {  // delete if-condition to return to old code
      //gEve->AddElement(fTrList, fEventManager);
      gEve->AddElement(fTrList, fTsManager);
    }
#else
    //gEve->AddElement(fTrList, fEventManager);
    gEve->AddElement(fTrList, fTsManager);
#endif
    fTrList->SetRecurse(kTRUE);
    switch (iOpt) {
      case 0:  //  display points
        fTrList->SetRnrPoints(kTRUE);
        fTrList->SetRnrLine(kFALSE);
        fTrList->SetMarkerSize(2.);
        fTrList->SetRnrChildren(fRenderP);  // default not shown
        break;
      case 1:  //display fit line
        fTrList->SetRnrLine(kTRUE);
        fTrList->SetLineWidth(2.);
        fTrList->SetRnrChildren(fRenderT);  // default not shown
        break;
      default:;
    }
  }
  return fTrList;
}
#if TOFDisplay == 1
TEveElementList* CbmTsDisTofTracklets::GetPSGroup(Int_t ihmul, Int_t iOpt)
{
  if (iOpt == 0)
    gs = Form("PTrkl_hmul%d", ihmul);
  else
    gs = Form("FTrkl_hmul%d", ihmul);
  fPSList = 0;
  for (Int_t i = 0; i < fEvePSList->GetEntriesFast(); i++) {
    TEveElementList* l = (TEveElementList*) fEvePSList->At(i);
    if (strcmp(l->GetName(), gs) == 0) {
      fPSList = l;
      break;
    }
  }
  if (fPSList == 0) {
    fPSList    = new TEveElementList(gs);
    Int_t iCol = ihmul;
    if (iCol > 4) iCol++;
    fPSList->SetMainColor(iCol);
    fEvePSList->Add(fPSList);
    //gEve->AddElement(fPSList, fEventManager);
    gEve->AddElement(fPSList, fTsManager);
    fPSList->SetRnrChildren(fRenderP);
  }
  return fPSList;
}
#endif

ClassImp(CbmTsDisTofTracklets)
