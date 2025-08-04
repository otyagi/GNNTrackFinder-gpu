/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

/** @brief Macro for check of sCVD BMON digis in direct vector from legacy unpacking
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 **/
void check_bmon_legacy_digis_scvd(TString inputFileName, size_t numTimeslices = 0, double_t dPileUpThrNs = 20.0)
{
  gROOT->cd();
  TFile* file = new TFile(inputFileName, "READ");
  TTree* tree = (TTree*) (file->Get("cbmsim"));

  std::vector<CbmBmonDigi>* vDigisBmon = new std::vector<CbmBmonDigi>();
  tree->SetBranchAddress("BmonDigi", &vDigisBmon);

  uint32_t nentries = tree->GetEntries();
  cout << "Entries: " << nentries << endl;
  nentries = (numTimeslices && numTimeslices < nentries ? numTimeslices : nentries);

  gROOT->cd();
  TH1* fHistMapBmonOld     = new TH1I("histMapBmonOld", "Channel map, old BMON; Strip []", 16, 0., 16.);
  TH2* fHistMapBmonScvd    = new TH2I("histMapBmonScvd", "Pad map, sCVD BMON; Pad X []; Pad Y []",  //
                                   2, 0., 2., 2, 0., 2.);
  TH2* fHistMapEvoBmonOld  = new TH2I("histMapEvoBmonOld", "Pad map, old BMON; TS []; Strip []",  //
                                     100, 0., 1000., 16, 0., 16.);
  TH2* fHistMapEvoBmonScvd = new TH2I("histMapEvoBmonScvd", "Pad map, sCVD BMON; TS []; Channel []",  //
                                      100, 0., 1000., 4, 0., 4.);

  double_t dDtMin   = -500.;
  double_t dDtMax   = 500.;
  int32_t iDtNbBins = std::round(dDtMax - dDtMin);
  TH1* fHistDtBmon =
    new TH1I("histDtBmon", "Time difference old vs sCVD BMON; dt [ns]", 10 * iDtNbBins, dDtMin, dDtMax);
  TH2* fHistDtEvoBmon = new TH2I("histDtEvoBmon", "Evolution Time difference old vs sCVD BMON ; TS []; dt [ns]",  //
                                 100, 0., 1000., iDtNbBins, dDtMin, dDtMax);
  TH2* fHistDtDxBmon  = new TH2I("histDtDxBmon", "X correlation vs Time diff, old vs sCVD BMON; dt [ns]; dX []",  //
                                1000, -500., 500., 33, -16.5, 16.5);
  TH2* fHistDxCorBmon = new TH2I("histDxCorrBmon", "Pad map, old vs sCVD BMON; Strip []; Pad []",  //
                                 16, 0., 16., 4, 0., 4.);

  TH2* fHistDtChanOld  = new TH2I("histDtChanOld", "Time difference old vs sCVD BMON VS Chan in old ; Ch []; dt [ns]",
                                 16, 0., 16., iDtNbBins, dDtMin, dDtMax);
  TH2* fHistDtChanScvd = new TH2I("histDtChanScvd", "Time difference old vs sCVD BMON VS Chan in Scvd ; Ch []; dt [ns]",
                                  4, 0., 4., iDtNbBins, dDtMin, dDtMax);

  double_t dSelfDtMin   = 0.;
  double_t dSelfDtMax   = 10000.;
  int32_t iSelfDtNbBins = std::round(dSelfDtMax - dSelfDtMin);
  TH1* fHistSelfDtOld =
    new TH1I("histSelfDtOld", "Time difference old BMON digis; self dt [ns]", iSelfDtNbBins, dSelfDtMin, dSelfDtMax);
  TH1* fHistSelfDtNew =
    new TH1I("histSelfDtNew", "Time difference new BMON digis; self dt [ns]", iSelfDtNbBins, dSelfDtMin, dSelfDtMax);
  TH1* fHistOldInPileup =
    new TH1I("histOldInPileup", Form("Fractions of old BMON digis in pileup (dt < %.1fns); Pileup?", dPileUpThrNs), 2,
             -0.5, 1.5);
  TH1* fHistNewInPileup =
    new TH1I("histNewInPileup", Form("Fractions of new BMON digis in pileup (dt < %.1fns); Pileup?", dPileUpThrNs), 2,
             -0.5, 1.5);

  uint8_t ucScvdX[4] = {1, 1, 0, 0};
  uint8_t ucScvdY[4] = {1, 0, 0, 1};

  for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
    //if (iEntry % 10 == 0 ) std::cout << "Entry " << iEntry << " / " << nentries << std::endl;

    tree->GetEntry(iEntry);
    uint32_t nDigisBmon = vDigisBmon->size();

    std::vector<CbmBmonDigi> vDigisOld;
    std::vector<CbmBmonDigi> vDigisScvd;
    vDigisOld.reserve(nDigisBmon);
    vDigisScvd.reserve(nDigisBmon);
    for (auto& digi : *vDigisBmon) {
      if (1 == CbmTofAddress::GetChannelSide(digi.GetAddress())) {
        if (CbmTofAddress::GetChannelId(digi.GetAddress()) < 4) {
          fHistMapBmonScvd->Fill(ucScvdX[CbmTofAddress::GetChannelId(digi.GetAddress())],
                                 ucScvdY[CbmTofAddress::GetChannelId(digi.GetAddress())]);
          fHistMapEvoBmonScvd->Fill(iEntry, CbmTofAddress::GetChannelId(digi.GetAddress()));
          vDigisScvd.push_back(digi);
        }
        else {
          LOG(fatal) << "Bad sCVD channel: " << CbmTofAddress::GetChannelId(digi.GetAddress());
        }
      }
      else {
        fHistMapBmonOld->Fill(CbmTofAddress::GetChannelId(digi.GetAddress()));
        fHistMapEvoBmonOld->Fill(iEntry, CbmTofAddress::GetChannelId(digi.GetAddress()));
        vDigisOld.push_back(digi);
      }
    }
    std::cout << "TS " << iEntry << ", BMON  Digis: " << nDigisBmon << " => " << vDigisOld.size() << " Old + "
              << vDigisScvd.size() << " sCVD" << std::endl;

    auto itScvdStart = vDigisScvd.begin();
    for (auto& digiOld : vDigisOld) {
      for (auto itScvd = itScvdStart; itScvd != vDigisScvd.end(); ++itScvd) {
        double_t dDt = (*itScvd).GetTime() - digiOld.GetTime();
        if (dDt < dDtMin) {
          itScvdStart = itScvd;
          continue;
        }
        if (dDtMax < dDt) {
          break;
        }
        fHistDtBmon->Fill(dDt);
        fHistDtEvoBmon->Fill(iEntry, dDt);
        fHistDtDxBmon->Fill(dDt, 16 * ucScvdX[CbmTofAddress::GetChannelId((*itScvd).GetAddress())]
                                   - CbmTofAddress::GetChannelId(digiOld.GetAddress()));
        if (-20 < dDt && dDt < 20) {
          fHistDxCorBmon->Fill(CbmTofAddress::GetChannelId(digiOld.GetAddress()),
                               CbmTofAddress::GetChannelId((*itScvd).GetAddress()));
          //                     ucScvdX[CbmTofAddress::GetChannelId((*itScvd).GetAddress())]);
        }
        fHistDtChanOld->Fill(CbmTofAddress::GetChannelId(digiOld.GetAddress()), dDt);
        fHistDtChanScvd->Fill(CbmTofAddress::GetChannelId((*itScvd).GetAddress()), dDt);
      }
    }
    auto prevDigiOld = vDigisOld.begin();
    for (auto itOld = prevDigiOld; itOld != vDigisOld.end(); ++itOld) {
      if (itOld != prevDigiOld) {
        double_t dDt = (*itOld).GetTime() - (*prevDigiOld).GetTime();
        fHistSelfDtOld->Fill(dDt);
        if (dDt < dPileUpThrNs) {
          fHistOldInPileup->Fill(1);
        }
        else {
          fHistOldInPileup->Fill(0);
        }
      }
      prevDigiOld = itOld;
    }
    auto prevDigiScvd = vDigisScvd.begin();
    for (auto itScvd = prevDigiScvd; itScvd != vDigisScvd.end(); ++itScvd) {
      if (itScvd != prevDigiScvd) {
        double_t dDt = (*itScvd).GetTime() - (*prevDigiScvd).GetTime();
        fHistSelfDtNew->Fill(dDt);
        if (dDt < dPileUpThrNs) {
          fHistNewInPileup->Fill(1);
        }
        else {
          fHistNewInPileup->Fill(0);
        }
      }
      prevDigiScvd = itScvd;
    }
  }

  TCanvas* fCanvMap = new TCanvas("canvMap", "Channel counts mapping for old and sCVD BMON");
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

  TCanvas* fCanvCorr = new TCanvas("canvCorr", "Correlations (T, X) between old and sCVD BMON");
  fCanvCorr->Divide(3, 2);

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
  fHistDtChanOld->Draw("colz");

  fCanvCorr->cd(4);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistDtDxBmon->Draw("colz");

  fCanvCorr->cd(5);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistDxCorBmon->Draw("colz");

  fCanvCorr->cd(6);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistDtChanScvd->Draw("colz");

  TCanvas* fCanvSelfCorr = new TCanvas("canvSelfCorr", "Self-Correlations in old and sCVD BMON");
  fCanvSelfCorr->Divide(2, 2);

  fCanvSelfCorr->cd(1);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistSelfDtOld->Draw("hist");

  fCanvSelfCorr->cd(2);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistSelfDtNew->Draw("hist");

  fCanvSelfCorr->cd(3);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistOldInPileup->Draw("hist");
  gPad->Update();
  dynamic_cast<TPaveStats*>(fHistOldInPileup->FindObject("stats"))->SetOptStat(110);

  fCanvSelfCorr->cd(4);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistNewInPileup->Draw("hist");
  gPad->Update();
  dynamic_cast<TPaveStats*>(fHistNewInPileup->FindObject("stats"))->SetOptStat(110);
}
