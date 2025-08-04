/* Copyright (C) 2009-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmTimeslicePixelHitSetDraw.h"

#include "CbmEvent.h"             // for CbmEvent
#include "CbmPixelHit.h"          // for CbmPixelHit
#include "CbmTimesliceManager.h"  // for CbmTimesliceManager

#include "FairDataSourceI.h"  // for FairDataSourceI
#include "FairTCASource.h"    // for FairTCASource
#include <Logger.h>           // for LOG, Logger

#include <Rtypes.h>         // for ClassImp
#include <TClonesArray.h>   // for TClonesArray
#include <TEveManager.h>    // for TEveManager, gEve
#include <TEvePointSet.h>   // for TEvePointSet
#include <TEveTreeTools.h>  // for TEvePointSelectorConsumer, etc
#include <TVector3.h>       // for TVector3

CbmTimeslicePixelHitSetDraw::CbmTimeslicePixelHitSetDraw(const char* name, Color_t color, Style_t mstyle,
                                                         Int_t iVerbose)
  : FairPointSetDraw(name, color, mstyle, iVerbose)
{
  /// Trick to get access to the data source pointer even if private in the base class
  /// TODO: switch from legacy constructor to standard constructor of the base class?
  ///       or break inheritance and make this one standalone by copying the remaining parts of base class?
  fLocalDataSourcePtr = new FairTCASource(GetName());
  SetDataSource(fLocalDataSourcePtr);
}

InitStatus CbmTimeslicePixelHitSetDraw::Init()
{
  LOG(debug) << "CbmTimeslicePixelHitSetDraw::Init()";

  /// Default initialization on base class
  FairPointSetDraw::Init();

  /// CBM timeslice specific: get array of CbmEvents
  FairRootManager* fManager = FairRootManager::Instance();
  fCbmEvents                = dynamic_cast<TClonesArray*>(fManager->GetObject("CbmEvent"));
  if (nullptr == fCbmEvents) {
    LOG(fatal) << "CbmTimeslicePixelHitSetDraw::Init() => CbmEvents branch not found! Task will be deactivated";
    SetActive(kFALSE);
  }

  /// Find the data type enum based on the name provided to constructor
  /// FIXME: find alternative to hard-coded if block... maybe reverse logic with constructor getting name from enum?
  std::string sName = GetName();  // Needed for comparisoon with string literal
  if ("MvdHit" == sName) {        //
    fDataType = ECbmDataType::kMvdHit;
  }
  else if ("StsHit" == sName) {  //
    fDataType = ECbmDataType::kStsHit;
  }
  else if ("RichHit" == sName) {  //
    fDataType = ECbmDataType::kRichHit;
  }
  else if ("MuchHit" == sName) {  //
    fDataType = ECbmDataType::kMuchPixelHit;
  }
  else if ("TrdHit" == sName) {  //
    fDataType = ECbmDataType::kTrdHit;
  }
  else if ("TofHit" == sName) {  //
    fDataType = ECbmDataType::kTofHit;
  }
  else if ("PsdHit" == sName) {  //
    fDataType = ECbmDataType::kPsdHit;
  }
  else if ("BmonHit" == sName) {  //
    fDataType = ECbmDataType::kBmonHit;
  }
  else {
    fDataType = ECbmDataType::kUnknown;
  }

  if (IsActive()) { return kSUCCESS; }
  else {
    return kERROR;
  }
}

void CbmTimeslicePixelHitSetDraw::Exec(Option_t* /*option*/)
{
  fLocalDataSourcePtr->Reset();
  fLocalDataSourcePtr->RetrieveData(CbmTimesliceManager::Instance()->GetTimesliceTime());

  if (0 < fCbmEvents->GetEntriesFast()) {
    /// When loading a new TS, load the first event if possible
    GotoEvent(0);
  }
}

void CbmTimeslicePixelHitSetDraw::GotoEvent(uint32_t uEventIdx)
{
  LOG(debug) << "CbmTimeslicePixelHitSetDraw::GotoEvent " << uEventIdx << " target " << GetName();

  if (fCbmEvents->GetEntriesFast() <= static_cast<Int_t>(uEventIdx)) {
    LOG(fatal) << "CbmTimeslicePixelHitSetDraw::GotoEvent() => Failure, tried to load event " << uEventIdx
               << " while only " << fCbmEvents->GetEntriesFast() << " events available in this TS!!!";
  }

  fEventIdx = uEventIdx;

  CbmEvent* event = dynamic_cast<CbmEvent*>(fCbmEvents->At(uEventIdx));

  int32_t iNbHitsInTs    = fLocalDataSourcePtr->GetNData();
  int32_t iNbHitsInEvent = event->GetNofData(fDataType);

  if (iNbHitsInTs < iNbHitsInEvent) {
    LOG(fatal) << "CbmTimeslicePixelHitSetDraw::GotoEvent() => Failure, more " << GetName() << " in event " << uEventIdx
               << " than available in the TS: " << iNbHitsInEvent << " VS " << iNbHitsInTs;
  }

  if (CbmTimesliceManager::Instance()->GetClearHandler()) {  //
    Reset();
  }

  TEvePointSet* q = new TEvePointSet(GetName(), iNbHitsInEvent, TEvePointSelectorConsumer::kTVT_XYZ);
  q->SetOwnIds(kTRUE);
  q->SetMarkerColor(fColor);
  q->SetMarkerSize(1.5);
  q->SetMarkerStyle(fStyle);

  for (int32_t iHitIdxInEvt = 0; iHitIdxInEvt < iNbHitsInEvent; ++iHitIdxInEvt) {
    TVector3 vec(GetVector(fLocalDataSourcePtr->GetData(event->GetIndex(fDataType, iHitIdxInEvt))));
    q->SetNextPoint(vec.X(), vec.Y(), vec.Z());
    // q->SetPointId(GetValue(p, i));
  }
  gEve->AddElement(q);
  gEve->Redraw3D(kFALSE);
  fq = q;
}

void CbmTimeslicePixelHitSetDraw::Reset()
{
  if (fq != 0) {
    fq->Reset();
    gEve->RemoveElement(fq, CbmTimesliceManager::Instance());
  }
}


TVector3 CbmTimeslicePixelHitSetDraw::GetVector(TObject* obj)
{
  CbmPixelHit* p = dynamic_cast<CbmPixelHit*>(obj);
  if (nullptr == p) {
    LOG(fatal) << "CbmTimesliceRecoTracks::GetVector() => Failure, object not derived from CbmPixelHit";
  }
  LOG(debug) << "-I- CbmTimeslicePixelHitSetDraw::GetVector: " << p->GetX() << " " << p->GetY() << " " << p->GetZ();
  return TVector3(p->GetX(), p->GetY(), p->GetZ());
}


ClassImp(CbmTimeslicePixelHitSetDraw)
