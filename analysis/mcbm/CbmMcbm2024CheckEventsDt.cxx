/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


#include "CbmMcbm2024CheckEventsDt.h"

#include "CbmDefs.h"
#include "CbmEvent.h"

#include <FairRootManager.h>
#include <Logger.h>

#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TROOT.h>
#include <TStopwatch.h>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <vector>


using namespace std;


// -----   Constructor   -----------------------------------------------------
CbmMcbm2024CheckEventsDt::CbmMcbm2024CheckEventsDt() : FairTask("MakeRecoEvents") {}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmMcbm2024CheckEventsDt::~CbmMcbm2024CheckEventsDt() {}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmMcbm2024CheckEventsDt::Exec(Option_t*)
{
  // --- First TS: find boundaries of plots
  /// TODO
  if (0 == fNumTs) {
    TDirectory* oldDir = gDirectory;
    TFile* oldFile     = gFile;
    gROOT->cd();

    for (std::string sDet : fvDets) {
      fHistDt[sDet]    = new TH1I(Form("histDt%s", sDet.data()),
                               Form("Time difference to event seed, %s; dt [ns]", sDet.data()), 1000, -500., 500.);
      fHistDtEvo[sDet] = new TH2I(Form("histDtEvo%s", sDet.data()),
                                  Form("Time difference to event seed vs TS, %s; TS []; dt [ns]", sDet.data()),  //
                                  100, 0., 1000., 1000, -500., 500.);

      fHistDtToBmon[sDet] =
        new TH1I(Form("histDtToBmon%s", sDet.data()),
                 Form("Time difference to first/single BMon, %s; dt [ns]", sDet.data()), 1000, -500., 500.);
      fHistDtToBmonEvo[sDet] =
        new TH2I(Form("histDtToBmonEvo%s", sDet.data()),
                 Form("Time difference to first/single BMon vs TS, %s; TS []; dt [ns]", sDet.data()),  //
                 100, 0., 1000., 1000, -500., 500.);

      fHistMul[sDet] =
        new TH1I(Form("histMul%s", sDet.data()), Form("Nb %s digis per event; N []", sDet.data()), 100, -0.5, 99.5);
      fHistDtMul[sDet] =
        new TH2I(Form("histDtMul%s", sDet.data()), Form("Dt Bmon vs Nb %s digis per event; dt [ns]; N []", sDet.data()),
                 1000, -500., 500., 100, -0.5, 99.5);
    }


    fCanvDt = new TCanvas("canvDt", "Time differences to event seed");
    fCanvDt->Divide(3, 3);

    uint32_t uPadIdx = 1;
    for (std::string sDet : fvDets) {
      fCanvDt->cd(uPadIdx);
      gPad->SetLogy();
      gPad->SetGridx();
      gPad->SetGridy();
      fHistDt[sDet]->Draw("hist");
      uPadIdx++;
    }

    fCanvDtEvo = new TCanvas("canvDtEvo", "Time differences to event seed vs TS");
    fCanvDtEvo->Divide(3, 3);

    uPadIdx = 1;
    for (std::string sDet : fvDets) {
      fCanvDtEvo->cd(uPadIdx);
      gPad->SetLogz();
      gPad->SetGridx();
      gPad->SetGridy();
      fHistDtEvo[sDet]->Draw("colz");
      uPadIdx++;
    }


    fCanvDtToBmon = new TCanvas("canvDtToBmon", "Time differences to event seed");
    fCanvDtToBmon->Divide(3, 3);

    uPadIdx = 1;
    for (std::string sDet : fvDets) {
      fCanvDtToBmon->cd(uPadIdx);
      gPad->SetLogy();
      gPad->SetGridx();
      gPad->SetGridy();
      fHistDtToBmon[sDet]->Draw("hist");
      uPadIdx++;
    }

    fCanvDtToBmonEvo = new TCanvas("canvDtToBmonEvo", "Time differences to event seed vs TS");
    fCanvDtToBmonEvo->Divide(3, 3);

    uPadIdx = 1;
    for (std::string sDet : fvDets) {
      fCanvDtToBmonEvo->cd(uPadIdx);
      gPad->SetLogz();
      gPad->SetGridx();
      gPad->SetGridy();
      fHistDtToBmonEvo[sDet]->Draw("colz");
      uPadIdx++;
    }

    for (std::string sDet : fvDets) {
      fCanvMul[sDet] = new TCanvas(Form("canvMul%s", sDet.data()), Form("Multiplicity %s", sDet.data()));
      fCanvMul[sDet]->Divide(2);

      fCanvMul[sDet]->cd(1);
      gPad->SetLogy();
      gPad->SetGridx();
      gPad->SetGridy();
      fHistMul[sDet]->Draw("hist");

      fCanvMul[sDet]->cd(2);
      gPad->SetLogz();
      gPad->SetGridx();
      gPad->SetGridy();
      fHistDtMul[sDet]->Draw("colz");
    }

    gFile      = oldFile;
    gDirectory = oldDir;
  }

  size_t numEventsInTs = 0;
  std::string sDet     = "Bmon";
  for (auto& event : *fEvents) {
    bool bSingleBmon         = false;
    double_t dSingleBmonTime = 0.;
    if (0 < event.fData.fBmon.Size()) {
      if (1 == event.fData.fBmon.Size()) {
        bSingleBmon = true;
      }
      dSingleBmonTime = event.fData.fBmon.fDigis[0].GetTime();
    }

    sDet = "Bmon";
    fHistMul[sDet]->Fill(event.fData.fBmon.Size());
    for (auto& digi : event.fData.fBmon.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fBmon.Size());

      // Special case: make internal Dt to first BMon instead of Dt to single Bmon
      dDt = digi.GetTime() - dSingleBmonTime;
      fHistDtToBmon[sDet]->Fill(dDt);
      fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
    }

    sDet = "Sts";
    fHistMul[sDet]->Fill(event.fData.fSts.Size());
    for (auto& digi : event.fData.fSts.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fSts.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }

    sDet = "Much";
    fHistMul[sDet]->Fill(event.fData.fMuch.Size());
    for (auto& digi : event.fData.fMuch.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fMuch.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }

    sDet = "Trd1d";
    fHistMul[sDet]->Fill(event.fData.fTrd.Size());
    for (auto& digi : event.fData.fTrd.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fTrd.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }

    sDet = "Trd2d";
    fHistMul[sDet]->Fill(event.fData.fTrd2d.Size());
    for (auto& digi : event.fData.fTrd2d.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fTrd2d.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }

    sDet = "Tof";
    fHistMul[sDet]->Fill(event.fData.fTof.Size());
    for (auto& digi : event.fData.fTof.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fTof.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }

    sDet = "Rich";
    fHistMul[sDet]->Fill(event.fData.fRich.Size());
    for (auto& digi : event.fData.fRich.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fRich.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }

    sDet = "Fsd";
    fHistMul[sDet]->Fill(event.fData.fFsd.Size());
    for (auto& digi : event.fData.fFsd.fDigis) {
      double_t dDt = digi.GetTime() - event.fTime;
      fHistDt[sDet]->Fill(dDt);
      fHistDtEvo[sDet]->Fill(fNumTs, dDt);
      fHistDtMul[sDet]->Fill(dDt, event.fData.fFsd.Size());
      if (bSingleBmon) {
        dDt = digi.GetTime() - dSingleBmonTime;
        fHistDtToBmon[sDet]->Fill(dDt);
        fHistDtToBmonEvo[sDet]->Fill(fNumTs, dDt);
      }
    }
    numEventsInTs++;
  }

  // --- Run statistics
  fNumTs++;
  fNumEvents += fEvents->size();
}
// ----------------------------------------------------------------------------


// -----   End-of-timeslice action   ------------------------------------------
void CbmMcbm2024CheckEventsDt::Finish()
{
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices : " << fNumTs;
  LOG(info) << "Events     : " << fNumEvents;
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
InitStatus CbmMcbm2024CheckEventsDt::Init()
{
  // --- Get FairRootManager instance
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  LOG(info) << "==================================================";
  LOG(info) << GetName() << ": Initialising...";

  // --- Input data
  fEvents = ioman->InitObjectAs<const std::vector<CbmDigiEvent>*>("DigiEvent");
  if (!fEvents) {
    LOG(error) << GetName() << ": No input branch DigiEvent!";
    return kFATAL;
  }
  LOG(info) << "--- Found branch DigiEvent at " << fEvents;

  LOG(info) << "==================================================";
  return kSUCCESS;
}
// ----------------------------------------------------------------------------

ClassImp(CbmMcbm2024CheckEventsDt)
