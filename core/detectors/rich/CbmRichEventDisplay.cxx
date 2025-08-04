/* Copyright (C) 2006-2023 UGiessen, JINR-LIT
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Supriya Das, Semen Lebedev [committer], Martin Beyer, Florian Uhlig */

/**
* \file CbmRichEventDisplay.cxx
*
* \author Supriya Das
* \date 2006
**/
#include "CbmRichEventDisplay.h"

#include "CbmDigiManager.h"
#include "CbmDrawHist.h"
#include "CbmEvent.h"
#include "CbmHistManager.h"
#include "CbmRichGeoManager.h"
#include "CbmRichHit.h"
#include "CbmRichPoint.h"
#include "CbmRichRing.h"

#include <FairRootManager.h>
#include <FairTrackParam.h>

#include <TCanvas.h>
#include <TClonesArray.h>
#include <TEllipse.h>
#include <TH2.h>
#include <TMarker.h>

#include <string>

InitStatus CbmRichEventDisplay::Init()
{
  FairRootManager* ioman = FairRootManager::Instance();
  if (!ioman) LOG(fatal) << "CbmRichEventDisplay::Init: RootManager not instantiated!";

  fDigiManager = CbmDigiManager::Instance();
  fDigiManager->Init();
  if (!fDigiManager->IsPresent(ECbmModuleId::kRich)) LOG(fatal) << "CbmRichEventDisplay::Init: No RichDigi array!";

  fCbmEvents = static_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
  if (!fCbmEvents) LOG(fatal) << "CbmRichEventDisplay::Init: No CbmEvent array!";

  fRichPoints = static_cast<TClonesArray*>(ioman->GetObject("RichPoint"));
  if (!fRichPoints && fDrawPoints) LOG(fatal) << "CbmRichEventDisplay::Init: No RichPoint array!";

  fRichHits = static_cast<TClonesArray*>(ioman->GetObject("RichHit"));
  if (!fRichHits && fDrawHits) LOG(fatal) << "CbmRichEventDisplay::Init: No RichHit array!";

  fRichRings = static_cast<TClonesArray*>(ioman->GetObject("RichRing"));
  if (!fRichRings && fDrawRings) LOG(fatal) << "CbmRichEventDisplay::Init: No RichRing array!";

  fRichProjections = static_cast<TClonesArray*>(ioman->GetObject("RichProjection"));
  if (!fRichProjections && fDrawProjections) LOG(fatal) << "CbmRichEventDisplay::Init: No RichProjection array!";

  fHM = new CbmHistManager();

  return kSUCCESS;
}

void CbmRichEventDisplay::Exec(Option_t* /*opt*/)
{
  SetDefaultDrawStyle();
  for (Int_t i = 0; i < fCbmEvents->GetEntriesFast(); i++) {
    fEventNum++;
    CbmEvent* event = static_cast<CbmEvent*>(fCbmEvents->At(i));
    DrawOneEvent(event);
  }
}

void CbmRichEventDisplay::DrawOneEvent(CbmEvent* event)
{
  std::stringstream ss;
  ss << "rich_event_display_event_" << fEventNum;
  TCanvas* c = fHM->CreateCanvas(ss.str().c_str(), ss.str().c_str(), 800, 800);
  c->Divide(1, 2);
  c->cd(1);
  TH2D* padU = new TH2D("padU", ";x [cm];y [cm]", 1, -120., 120., 1, 120., 210);
  DrawH2(padU);
  padU->GetYaxis()->SetTitleOffset(0.75);
  gPad->SetLeftMargin(0.1);
  gPad->SetRightMargin(0.05);
  DrawOnePmtPlane("up", event);

  c->cd(2);
  TH2D* padD = new TH2D("padD", ";x [cm];y [cm]", 1, -120., 120., 1, -210., -120.);
  DrawH2(padD);
  padD->GetYaxis()->SetTitleOffset(0.75);
  gPad->SetLeftMargin(0.1);
  gPad->SetRightMargin(0.05);
  DrawOnePmtPlane("down", event);
}

void CbmRichEventDisplay::DrawOnePmtPlane(const std::string& plane, CbmEvent* event)
{
  // Draw Track projections
  if (fDrawProjections) {
    int nofProjections = event->GetNofData(ECbmDataType::kRichTrackProjection);
    for (int iP = 0; iP < nofProjections; iP++) {
      FairTrackParam* pr =
        (FairTrackParam*) fRichProjections->At(event->GetIndex(ECbmDataType::kRichTrackProjection, iP));
      if (!pr) continue;
      // FIXME: only draw successful projections
      if ((plane == "up" && pr->GetY() >= 0.) || (plane == "down" && pr->GetY() < 0.)) {
        TMarker* m = new TMarker(pr->GetX(), pr->GetY(), 3.);
        m->SetMarkerSize(0.7);
        m->SetMarkerColor(kGreen + 3);
        m->Draw();
      }
    }
  }

  // Draw hits
  if (fDrawHits) {
    int nofHits = event->GetNofData(ECbmDataType::kRichHit);
    for (int iH = 0; iH < nofHits; iH++) {
      CbmRichHit* hit = (CbmRichHit*) fRichHits->At(event->GetIndex(ECbmDataType::kRichHit, iH));
      if (!hit) continue;
      if ((plane == "up" && hit->GetY() >= 0.) || (plane == "down" && hit->GetY() < 0.)) {
        TEllipse* hitDr = new TEllipse(hit->GetX(), hit->GetY(), 0.6);
        hitDr->SetFillColor(kRed);
        hitDr->SetLineColor(kRed);
        hitDr->Draw();
      }
    }
  }

  // Draw rings
  if (fDrawRings) {
    int nofRings = event->GetNofData(ECbmDataType::kRichRing);
    for (int iR = 0; iR < nofRings; iR++) {
      CbmRichRing* ring = (CbmRichRing*) fRichRings->At(event->GetIndex(ECbmDataType::kRichRing, iR));
      if (!ring) continue;
      if ((plane == "up" && ring->GetCenterY() >= 0.) || (plane == "down" && ring->GetCenterY() < 0.)) {
        DrawEllipse(ring);
      }
    }
  }

  // Draw RICH MC Points
  // Slow for large ts
  if (fDrawPoints) {
    Double_t evStart = event->GetStartTime();
    Double_t evStop  = event->GetEndTime();
    int nofPoints    = fRichPoints->GetEntriesFast();
    for (int iP = 0; iP < nofPoints; iP++) {
      CbmRichPoint* point = (CbmRichPoint*) fRichPoints->At(iP);
      if (!point) continue;
      Double_t pointTime = point->GetTime();
      if (pointTime > evStart && pointTime < evStop) continue;
      if ((plane == "up" && point->GetY() >= 0.) || (plane == "down" && point->GetY() < 0.)) {
        TVector3 posPoint(point->GetX(), point->GetY(), point->GetZ());
        TVector3 detPoint;
        CbmRichGeoManager::GetInstance().RotatePoint(&posPoint, &detPoint, false);
        TEllipse* pointDr = new TEllipse(detPoint.X(), detPoint.Y(), 0.2);
        pointDr->Draw();
      }
    }
  }
}

void CbmRichEventDisplay::DrawEllipse(CbmRichRing* ring)
{
  TEllipse* circle = new TEllipse(ring->GetCenterX(), ring->GetCenterY(), ring->GetRadius());
  circle->SetFillStyle(0);
  circle->SetLineWidth(2);
  circle->SetLineColor(kBlue);
  circle->Draw();
  TMarker* center = new TMarker(ring->GetCenterX(), ring->GetCenterY(), 2);
  center->SetMarkerColor(kBlue);
  center->SetMarkerSize(0.4);
  center->Draw();
}

void CbmRichEventDisplay::Finish() { fHM->SaveCanvasToImage(fOutputDir, fOutputFormat); }

ClassImp(CbmRichEventDisplay)
