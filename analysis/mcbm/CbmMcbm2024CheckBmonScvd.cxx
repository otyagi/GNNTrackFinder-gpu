/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


#include "CbmMcbm2024CheckBmonScvd.h"

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
CbmMcbm2024CheckBmonScvd::CbmMcbm2024CheckBmonScvd() : FairTask("MakeRecoEvents") {}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmMcbm2024CheckBmonScvd::~CbmMcbm2024CheckBmonScvd() {}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmMcbm2024CheckBmonScvd::Exec(Option_t*)
{
  // --- First TS: find boundaries of plots
  /// TODO
  if (0 == fNumTs) {
    TDirectory* oldDir = gDirectory;
    TFile* oldFile     = gFile;
    gROOT->cd();

    fHistMapBmonOld     = new TH1I("histMapBmonOld", "Channel map, old BMON; Strip []", 16, 0., 16.);
    fHistMapBmonScvd    = new TH2I("histMapBmonScvd", "Pad map, sCVD BMON; Pad X []; Pad Y []",  //
                                2, 0., 2., 2, 0., 2.);
    fHistMapEvoBmonOld  = new TH2I("histMapEvoBmonOld", "Pad map, old BMON; TS []; Strip []",  //
                                  100, 0., 1000., 16, 0., 16.);
    fHistMapEvoBmonScvd = new TH2I("histMapEvoBmonScvd", "Pad map, sCVD BMON; TS []; Channel []",  //
                                   100, 0., 1000., 4, 0., 4.);

    fHistDtBmon    = new TH1I("histDtBmon", "Time difference old vs sCVD BMON; dt [ns]", 1000, -500., 500.);
    fHistDtEvoBmon = new TH2I("histDtEvoBmon", "Evolution Time difference old vs sCVD BMON ; TS []; dt [ns]",  //
                              100, 0., 1000., 1000, -500., 500.);
    fHistDtDxBmon  = new TH2I("histDtDxBmon", "X correlation vs Time diff, old vs sCVD BMON; dt [ns]; dX []",  //
                             1000, -500., 500., 33, -16.5, 16.5);
    fHistDxCorBmon = new TH2I("histDxCorrBmon", "Pad map, old vs sCVD BMON; Strip []; Column []",  //
                              16, 0., 16., 2, 0., 2.);

    fCanvMap = new TCanvas("canvMap", "Channel counts mapping for old and sCVD BMON");
    fCanvMap->Divide(2, 2);

    fCanvMap->cd(1);
    gPad->SetLogy();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistMapBmonOld->Draw("hist");

    fCanvMap->cd(2);
    gPad->SetLogz();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistMapBmonScvd->Draw("ColzText");

    fCanvMap->cd(3);
    gPad->SetLogz();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistMapEvoBmonOld->Draw("colz");

    fCanvMap->cd(4);
    gPad->SetLogz();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistMapEvoBmonScvd->Draw("colz");

    fCanvCorr = new TCanvas("canvCorr", "Correlations (T, X) between old and sCVD BMON");
    fCanvCorr->Divide(2, 2);

    fCanvCorr->cd(1);
    gPad->SetLogy();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistDtBmon->Draw("hist");

    fCanvCorr->cd(2);
    gPad->SetLogz();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistDtEvoBmon->Draw("colz");

    fCanvCorr->cd(3);
    gPad->SetLogz();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistDtDxBmon->Draw("colz");

    fCanvCorr->cd(4);
    gPad->SetLogz();
    gPad->SetGridx();
    gPad->SetGridy();
    fHistDxCorBmon->Draw("colz");

    gFile      = oldFile;
    gDirectory = oldDir;
  }

  size_t numEventsInTs = 0;
  uint8_t ucScvdX[4]   = {1, 1, 0, 0};
  uint8_t ucScvdY[4]   = {1, 0, 0, 1};
  for (auto& event : *fEvents) {

    std::vector<CbmBmonDigi> vDigisOld;
    std::vector<CbmBmonDigi> vDigisScvd;
    vDigisOld.reserve(event.fData.fBmon.Size());
    vDigisScvd.reserve(event.fData.fBmon.Size());
    for (auto& digi : event.fData.fBmon.fDigis) {
      if (1 == CbmTofAddress::GetChannelSide(digi.GetAddress())) {
        if (CbmTofAddress::GetChannelId(digi.GetAddress()) < 4) {
          fHistMapBmonScvd->Fill(ucScvdX[CbmTofAddress::GetChannelId(digi.GetAddress())],
                                 ucScvdY[CbmTofAddress::GetChannelId(digi.GetAddress())]);
          fHistMapEvoBmonScvd->Fill(fNumTs, CbmTofAddress::GetChannelId(digi.GetAddress()));
          vDigisScvd.push_back(digi);
        }
        else {
          LOG(fatal) << "Bad sCVD channel: " << CbmTofAddress::GetChannelId(digi.GetAddress());
        }
      }
      else {
        fHistMapBmonOld->Fill(CbmTofAddress::GetChannelId(digi.GetAddress()));
        fHistMapEvoBmonOld->Fill(fNumTs, CbmTofAddress::GetChannelId(digi.GetAddress()));
        vDigisOld.push_back(digi);
      }
    }

    for (auto& digiOld : vDigisOld) {
      for (auto& digiScvd : vDigisScvd) {
        double_t dDt = digiScvd.GetTime() - digiOld.GetTime();
        fHistDtBmon->Fill(dDt);
        fHistDtEvoBmon->Fill(fNumTs, dDt);
        fHistDtDxBmon->Fill(dDt, 16 * ucScvdX[CbmTofAddress::GetChannelId(digiScvd.GetAddress())]
                                   - CbmTofAddress::GetChannelId(digiOld.GetAddress()));
        fHistDxCorBmon->Fill(CbmTofAddress::GetChannelId(digiOld.GetAddress()),
                             ucScvdX[CbmTofAddress::GetChannelId(digiScvd.GetAddress())]);
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
void CbmMcbm2024CheckBmonScvd::Finish()
{
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices : " << fNumTs;
  LOG(info) << "Events     : " << fNumEvents;
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
InitStatus CbmMcbm2024CheckBmonScvd::Init()
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

ClassImp(CbmMcbm2024CheckBmonScvd)
