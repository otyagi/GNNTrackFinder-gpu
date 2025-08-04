/* Copyright (C) 2024 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer [committer] */

#include "CbmRichMCbmDenoiseQa.h"

#include "CbmEvent.h"
#include "CbmHistManager.h"
#include "CbmRichHit.h"
#include "CbmRichRing.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TCanvas.h>
#include <TClonesArray.h>
#include <TDirectory.h>
#include <TEllipse.h>
#include <TFile.h>
#include <TH2D.h>

InitStatus CbmRichMCbmDenoiseQa::Init()
{
  FairRootManager* manager = FairRootManager::Instance();
  if (!manager) LOG(fatal) << GetName() << "::Init: No FairRootManager";

  fCbmEvents = static_cast<TClonesArray*>(manager->GetObject("CbmEvent"));
  LOG(info) << GetName() << "::Init: CbmEvent array " << (fCbmEvents ? "found" : "not found");

  fRichHits = static_cast<TClonesArray*>(manager->GetObject("RichHit"));
  if (!fRichHits) LOG(fatal) << GetName() << "::Init: No RichHit array!";

  fRichRings = static_cast<TClonesArray*>(manager->GetObject("RichRing"));
  if (!fRichRings) LOG(fatal) << GetName() << "::Init: No RichRing array!";

  InitHistograms();

  return kSUCCESS;
}

void CbmRichMCbmDenoiseQa::InitHistograms()
{
  fHM    = std::make_unique<CbmHistManager>();
  fHMSed = std::make_unique<CbmHistManager>();

  // 1D Histograms
  fHM->Create1<TH1D>("fhRingsPerEvent", "number of ring per event;# rings per event;Entries", 6, -0.5, 5.5);

  fHM->Create1<TH1D>("fhRingRadius", "ring radius;ring radius [cm];Entries", 100, 0., 7.);

  fHM->Create1<TH1D>("fhRingRadiusUp", "ring radius upper half;ring radius [cm]; count", 100, 0., 7.);

  fHM->Create1<TH1D>("fhRingRadiusDown", "ring radius lower half;ring radius [cm]; count", 100, 0., 7.);

  fHM->Create1<TH1D>("fhRingHits", "number of ring hits;# ring hits;Entries", 50, -0.5, 49.5);

  fHM->Create1<TH1D>("fhRingHitsUp", "number of ring hits upper half;# ring hits;Entries", 50, -0.5, 49.5);

  fHM->Create1<TH1D>("fhRingHitsDown", "number of ring hits lower half;# ring hits;Entries", 50, -0.5, 49.5);

  fHM->Create1<TH1D>("fhRingHitsTimeDiff", "ringhit to ring time difference;hittime-ringtime [ns]; count", 100, -10.,
                     10.);

  // 2D Histograms
  fHM->Create2<TH2D>("fhRichHitsXY", "position of rich hits;X [cm];Y [cm];Entries", 37, -11.2, 11.2, 82, -24.115,
                     23.885);

  fHM->Create2<TH2D>("fhRingHitsXY", "position of ring hits;X [cm];Y [cm];Entries", 37, -11.2, 11.2, 82, -24.115,
                     23.885);

  fHM->Create2<TH2D>("fhRingCenterXY", "position of ring centers;X [cm];Y [cm];Entries", 37, -11.2, 11.2, 82, -24.115,
                     23.885);
}

void CbmRichMCbmDenoiseQa::Exec(Option_t* /*option*/)
{
  LOG(debug) << GetName() << " TS " << fTsNum;
  fTsNum++;

  if (fCbmEvents) {
    Int_t nEvents = fCbmEvents->GetEntriesFast();
    for (Int_t iEvent = 0; iEvent < nEvents; iEvent++) {
      CbmEvent* event = static_cast<CbmEvent*>(fCbmEvents->At(iEvent));
      if (!event) continue;
      if (event->GetNofData(ECbmDataType::kRichRing) > 0) {
        fHM->H1("fhRingsPerEvent")->Fill(static_cast<double>(event->GetNofData(ECbmDataType::kRichRing)));
        if (fnSEDs < fMaxSEDs) {
          DrawSED(event);
          fnSEDs++;
        }
      }
      Process(event);
      fEventNum++;
    }
  }
  else {
    Process(nullptr);
  }
}

void CbmRichMCbmDenoiseQa::Process(CbmEvent* event)
{
  // RICH Hits
  Int_t nRichHits = event ? static_cast<Int_t>(event->GetNofData(ECbmDataType::kRichHit)) : fRichHits->GetEntriesFast();
  for (int i = 0; i < nRichHits; i++) {
    Int_t hitIndex  = event ? static_cast<Int_t>(event->GetIndex(ECbmDataType::kRichHit, static_cast<uint32_t>(i))) : i;
    CbmRichHit* hit = static_cast<CbmRichHit*>(fRichHits->At(hitIndex));
    if (!hit) continue;
    fHM->H2("fhRichHitsXY")->Fill(hit->GetX(), hit->GetY());
  }

  // RICH Rings
  Int_t nRichRings =
    event ? static_cast<Int_t>(event->GetNofData(ECbmDataType::kRichRing)) : fRichRings->GetEntriesFast();
  for (int i = 0; i < nRichRings; i++) {
    Int_t ringIndex =
      event ? static_cast<Int_t>(event->GetIndex(ECbmDataType::kRichRing, static_cast<uint32_t>(i))) : i;
    CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(ringIndex));
    if (!ring) continue;
    fHM->H1("fhRingRadius")->Fill(ring->GetRadius());
    fHM->H1(std::string("fhRingRadius") + std::string((ring->GetCenterY() > 0) ? "Up" : "Down"))
      ->Fill(ring->GetRadius());
    fHM->H1("fhRingHits")->Fill(ring->GetNofHits());
    fHM->H1(std::string("fhRingHits") + std::string((ring->GetCenterY() > 0) ? "Up" : "Down"))
      ->Fill(ring->GetNofHits());
    fHM->H2("fhRingCenterXY")->Fill(ring->GetCenterX(), ring->GetCenterY());
    // Ring hits
    Int_t nRingHits = ring->GetNofHits();
    for (int j = 0; j < nRingHits; j++) {
      CbmRichHit* hit = static_cast<CbmRichHit*>(fRichHits->At(static_cast<Int_t>(ring->GetHit(j))));
      if (!hit) continue;
      fHM->H1("fhRingHitsTimeDiff")->Fill(hit->GetTime() - ring->GetTime());
      fHM->H2("fhRingHitsXY")->Fill(hit->GetX(), hit->GetY());
    }
  }
}

void CbmRichMCbmDenoiseQa::DrawSED(CbmEvent* event)
{
  std::string hName = "fhSED" + std::to_string(fEventNum);
  auto c            = fHMSed->CreateCanvas(hName, hName, 800, 1505);
  c->SetMargin(0.15, 0.15, 0.1, 0.1);
  c->cd();
  TH2D* h = new TH2D(hName.c_str(), hName.c_str(), 37, -11.2, 11.2, 82, -24.115, 23.885);
  h->SetXTitle("X [cm]");
  h->SetYTitle("Y [cm]");
  h->SetZTitle("NN classification 0: noise, 1: signal");
  h->GetZaxis()->SetTitleOffset(1.3);
  fHMSed->Add(hName, h);

  Int_t nHits = static_cast<Int_t>(event->GetNofData(ECbmDataType::kRichHit));
  for (int i = 0; i < nHits; i++) {
    Int_t hitIndex  = static_cast<Int_t>(event->GetIndex(ECbmDataType::kRichHit, static_cast<uint32_t>(i)));
    CbmRichHit* hit = static_cast<CbmRichHit*>(fRichHits->At(hitIndex));
    if (!hit) continue;
    if (h->GetBinContent(h->FindBin(hit->GetX(), hit->GetY())) > 0) {
      LOG(info) << GetName() << "::DrawSED channel already filled for this event. Skipping.";
      continue;
    }
    if (!hit->GetIsNoiseNN()) {
      h->Fill(hit->GetX(), hit->GetY(), 1.0);
    }
    else {
      h->Fill(hit->GetX(), hit->GetY(), 0.001);
    }
  }
  h->Draw("colz");

  Int_t nRings = static_cast<Int_t>(event->GetNofData(ECbmDataType::kRichRing));
  for (int i = 0; i < nRings; i++) {
    Int_t ringIndex   = static_cast<Int_t>(event->GetIndex(ECbmDataType::kRichRing, static_cast<uint32_t>(i)));
    CbmRichRing* ring = static_cast<CbmRichRing*>(fRichRings->At(ringIndex));
    if (!ring) continue;
    TEllipse* circle = new TEllipse(ring->GetCenterX(), ring->GetCenterY(), ring->GetAaxis(), ring->GetBaxis(), 0.,
                                    360., ring->GetPhi() * 180. / TMath::Pi());
    circle->SetFillStyle(0);
    circle->SetLineWidth(2);
    circle->Draw("same");
  }
}

void CbmRichMCbmDenoiseQa::Finish()
{
  TDirectory* oldir = gDirectory;
  TFile* outFile    = FairRootManager::Instance()->GetOutFile();
  if (outFile) {
    outFile->mkdir(GetName());
    outFile->cd(GetName());
    fHM->WriteToFile();
    outFile->cd();
    outFile->mkdir((std::string(GetName()) + std::string("SEDs")).c_str());
    outFile->cd((std::string(GetName()) + std::string("SEDs")).c_str());
    fHMSed->WriteCanvasToFile();
    fHM->Clear();
    fHMSed->Clear();
  }
  gDirectory->cd(oldir->GetPath());
}
