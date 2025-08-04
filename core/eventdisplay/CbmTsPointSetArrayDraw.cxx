/* Copyright (C) 2023 PI-UHd, Heidelberg
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer], Pierre-Alain Loizeau */

// -------------------------------------------------------------------------
// -----               CbmTsPointSetArrayDraw source file                -----
// -----                Created 18/06/22  by J. Brandt                 -----
// -----               Following class FairPointSetDraw                -----
// -------------------------------------------------------------------------
#include "CbmTsPointSetArrayDraw.h"

#include "CbmEvent.h"             // for CbmEvent
#include "CbmPixelHit.h"          // for CbmPixelHit
#include "CbmPointSetArray.h"     // for CbmPointSetArray
#include "CbmTofHit.h"            // for CbmTofHit
#include <CbmTimesliceManager.h>  // for CbmTimesliceManager

#include <FairRootManager.h>  // for FairRootManager
#include <FairTask.h>         // for FairTask, InitStatus, kSUCCESS
#include <Logger.h>           // for LOG, Logger

#include <Rtypes.h>             // for kRed, ClassImp
#include <TClonesArray.h>       // for TClonesArray
#include <TEveManager.h>        // for TEveManager, gEve
#include <TEveTreeTools.h>      // for TEvePointSelectorConsumer, TEvePointS...
#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TVector3.h>           // for TVector3

#include <iomanip>  // for operator<<, setprecision

// -----   Standard constructor   ------------------------------------------
CbmTsPointSetArrayDraw::CbmTsPointSetArrayDraw(const char* name, Int_t colorMode, Int_t markerMode, Int_t iVerbose,
                                               Bool_t render)
  : FairTask(name, iVerbose)
  , fVerbose(iVerbose)
  , fColorMode(colorMode)
  , fMarkerMode(markerMode)
  , fRender(render)
{
}

// -------------------------------------------------------------------------
InitStatus CbmTsPointSetArrayDraw::Init()
{
  LOG(debug) << "CbmTsPointSetArrayDraw::Init() for " << fTofHitArrayName;
  FairRootManager* fManager = FairRootManager::Instance();
  fCbmEvents                = dynamic_cast<TClonesArray*>(fManager->GetObject("CbmEvent"));
  fTsPointList              = static_cast<TClonesArray*>(fManager->GetObject(fTofHitArrayName.Data()));
  if (fTsPointList == nullptr) {
    LOG(warn) << "CbmTsPointSetArrayDraw::Init()  branch " << fTofHitArrayName
              << " not found! Task will be deactivated ";
    SetActive(kFALSE);
  }
  //LOG(info) << "CbmTsPointSetArrayDraw::Init() get track list" << fTsPointList->GetName();
  fPointList = new TClonesArray("CbmTofHit", 100);  // allocate memory
  fTsManager = CbmTimesliceManager::Instance();
  LOG(debug1) << "CbmTsPointSetArrayDraw::Init() get instance of CbmTimesliceManager ";
  fl = 0;

  return kSUCCESS;
}
// -------------------------------------------------------------------------
void CbmTsPointSetArrayDraw::Exec(Option_t* /*option*/)
{
  if (0 < fCbmEvents->GetEntriesFast()) {
    /// When loading a new TS, load the first event if possible
    GotoEvent(0);
  }
}

void CbmTsPointSetArrayDraw::GotoEvent(uint32_t uEventIdx)
{
  if (IsActive()) {

    if (CbmTimesliceManager::Instance()->GetClearHandler()) {  //
      Reset();
    }

    CbmEvent* event = dynamic_cast<CbmEvent*>(fCbmEvents->At(uEventIdx));
    if (nullptr == fTsPointList) {
      fTsPointList = static_cast<TClonesArray*>(FairRootManager::Instance()->GetObject(fTofHitArrayName.Data()));
      if (nullptr == fTsPointList) {
        LOG(warn) << " No " << fTofHitArrayName;
        return;
      }
    }
    if (nullptr != fPointList) fPointList->Clear();
    int nofPoints = fTsPointList->GetEntriesFast();
    int nPoints   = 0;
    LOG(debug3) << " Fill TofUhit from " << nofPoints << " hits in Ts.";
    for (size_t iP = 0; iP < event->GetNofData(ECbmDataType::kTofUHit); ++iP) {
      Int_t iPId      = event->GetIndex(ECbmDataType::kTofUHit, iP);  // make this generic!!
      CbmTofHit* tHit = dynamic_cast<CbmTofHit*>(fTsPointList->At(iPId));
      LOG(debug3) << "Add Hit " << iP << ", " << iPId << ", " << tHit << " at " << nPoints;
      if (nullptr != tHit && iPId > -1) new ((*fPointList)[nPoints++]) CbmTofHit(*tHit);
    }
    nPoints = fPointList->GetEntriesFast();

    //Reset();

    // initialize CbmPointSetArray to display set of hits
    CbmPointSetArray* l = new CbmPointSetArray(fTofHitArrayName, "");
    l->SetColorMode(fColorMode);
    l->SetMarkerMode(fMarkerMode);
    l->SetSourceCS(TEvePointSelectorConsumer::kTVT_XYZ);
    l->SetMarkerColor(kRed);
    l->SetMarkerStyle(22);
    l->SetMarkerSize(2.0);

    DetermineTimeOffset();
    l->InitBins("Hits", nPoints, 0.5, nPoints + 0.5);
    l->InitValues(nPoints);

    for (Int_t i = 1; i <= nPoints; i++) {  //loop over all hits in event
      TObject* p = static_cast<TObject*>(fPointList->At(i - 1));
      if (p != 0) {
        TVector3 vec(GetVector(p));
        l->Fill(vec.X(), vec.Y(), vec.Z(), i);  // fill 3D position
        l->FillValues(GetPointId(p), GetTime(p), GetTot(p), GetClusterSize(p),
                      i);  // fill physical information used for color and markersize
      }
    }

    //l->ApplyColorMode();   // apply colorMode and calculate color of each bin
    //l->ApplyMarkerMode();  // apply markerMode and calculate markersize of each bin
    //l->ApplyTitles();      // set BBox-title of each bin and computeBBox

    l->SetRnrChildren(fRender);
    gEve->AddElement(l);
    gEve->Redraw3D(kFALSE);
    fl = l;
  }
}
// --------------------------------------------------------------------------------
// returns 3D-vector with position data of hit
TVector3 CbmTsPointSetArrayDraw::GetVector(TObject* obj)
{
  CbmPixelHit* p = (CbmPixelHit*) obj;
  LOG(debug2) << "-I- CbmTsPointSetArrayDraw::GetVector(): " << p->GetX() << " " << p->GetY() << " " << p->GetZ()
              << " ";
  return TVector3(p->GetX(), p->GetY(), p->GetZ());
}
// --------------------------------------------------------------------------------
// returns hit-time against first hit
Double_t CbmTsPointSetArrayDraw::GetTime(TObject* obj)
{
  CbmPixelHit* p = (CbmPixelHit*) obj;
  LOG(debug2) << "-I- CbmTsPointSetArrayDraw::GetTime(): " << p->GetTime() - fTimeOffset;
  return p->GetTime() - fTimeOffset;
}
// --------------------------------------------------------------------------------
// returns ClusterSize of Hit
Int_t CbmTsPointSetArrayDraw::GetClusterSize(TObject* obj)
{
  //CluSize of TofHit is stored in Flag-Variable (set in Clusterizer)
  CbmTofHit* p     = (CbmTofHit*) obj;
  Double_t cluSize = p->GetFlag();
  //Flag= #digis = 2*cluSize  +100 if used for track
  cluSize = ((int) cluSize % 100) / 2;
  LOG(debug3) << "-I- CbmTsPointSetArrayDraw::GetClusterSize(): " << cluSize;
  return cluSize;
}
// --------------------------------------------------------------------------------
// returns ToT of hit
Double_t CbmTsPointSetArrayDraw::GetTot(TObject* obj)
{
  // ToT of TofHit is stored in Channel-Variable (set in Clusterizer)
  CbmTofHit* p = (CbmTofHit*) obj;
  Double_t tot = Double_t(p->GetCh()) / (20 * GetClusterSize(p));
  LOG(debug3) << "-I- CbmTsPointSetArrayDraw::GetTot(): " << tot;
  return tot;
}
// --------------------------------------------------------------------------------
// returns Id of hit
Int_t CbmTsPointSetArrayDraw::GetPointId(TObject* obj)
{
  CbmPixelHit* p = (CbmPixelHit*) obj;
  return p->GetRefId();
}
// ---------------------------------------------------------------------------------
// Determine time of first hit in event to use as offset
void CbmTsPointSetArrayDraw::DetermineTimeOffset()
{
  Int_t nPoints = fPointList->GetEntriesFast();
  fTimeOffset   = 115200000000000;  //32hours in ns as maximum of clock
  fTimeMax      = 0;
  Double_t currtime;
  CbmPixelHit* hit;
  for (Int_t i = 0; i < nPoints; i++) {  //loop over all hits in event
    hit      = static_cast<CbmPixelHit*>(fPointList->At(i));
    currtime = hit->GetTime();
    if (currtime < fTimeOffset) {
      fTimeOffset = currtime;
    }
    else if (currtime > fTimeMax) {
      fTimeMax = currtime;
    }
  }
  fTimeMax -= fTimeOffset;  //time of latest hit in event
  LOG(debug3) << std::setprecision(15) << "-I- CbmTsPointSetArrayDraw::DetermineTimeBins: fTimeOffset " << fTimeOffset;
}

// -----   Destructor   ----------------------------------------------------
CbmTsPointSetArrayDraw::~CbmTsPointSetArrayDraw() {}
// -------------------------------------------------------------------------
void CbmTsPointSetArrayDraw::SetParContainers() {}
// -------------------------------------------------------------------------
/** Action after each event**/
void CbmTsPointSetArrayDraw::Finish() {}
// -------------------------------------------------------------------------
void CbmTsPointSetArrayDraw::Reset()
{
  if (fl != 0) {
    LOG(debug3) << GetName() << ": Remove Element " << fl->GetName() << ", Man " << fTsManager->GetName();
    //fl->RemoveElementsLocal();
    gEve->RemoveElement(fl, fTsManager);
  }
}


ClassImp(CbmTsPointSetArrayDraw);
