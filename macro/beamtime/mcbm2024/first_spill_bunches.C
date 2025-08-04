/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

/** @brief Macro for check of digis time spread in direct vector from legacy or online unpacking
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 ** @param dBinSizeNs     Size of one bin in the time distribution in ns
 ** @param dTsSizeNs      Size of one Ts in ns = Total length of one time disribution histogram
 **/
/// TODO: Add plots for individual STS and TOF "stations"
void first_spill_bunches(TString inputFileName, uint32_t uRunId, size_t numTimeslices = 0, double_t dBinSizeNs = 100.0,
                         double_t dZoomOffset = 60000000.0, double_t dZoomLength = 10000.0,
                         double_t dTsSizeNs = 128000000.0)
{
  gROOT->cd();
  TFile* file = new TFile(inputFileName, "READ");
  TTree* tree = (TTree*) (file->Get("cbmsim"));

  std::vector<CbmBmonDigi>* vDigisBmon = new std::vector<CbmBmonDigi>();
  tree->SetBranchAddress("BmonDigi", &vDigisBmon);

  std::vector<CbmStsDigi>* vDigisSts = new std::vector<CbmStsDigi>();
  tree->SetBranchAddress("StsDigi", &vDigisSts);

  std::vector<CbmTofDigi>* vDigisTof = new std::vector<CbmTofDigi>();
  tree->SetBranchAddress("TofDigi", &vDigisTof);

  std::vector<CbmTrdDigi>* vDigisTrd = new std::vector<CbmTrdDigi>();
  tree->SetBranchAddress("TrdDigi", &vDigisTrd);

  std::vector<CbmRichDigi>* vDigisRich = new std::vector<CbmRichDigi>();
  tree->SetBranchAddress("RichDigi", &vDigisRich);

  uint32_t nentries = tree->GetEntries();
  cout << "Entries: " << nentries << endl;
  nentries = (numTimeslices && numTimeslices < nentries ? numTimeslices : nentries);

  gROOT->cd();

  int32_t iNbBinsRange = std::round(dTsSizeNs / dBinSizeNs);
  int32_t iNbBinsZoom  = std::round(dZoomLength / dBinSizeNs);

  TH1* hDigiTimeDistZoomBmonOld = new TH1I(
    Form("hDistZoomBmonOld_%04d", uRunId),
    Form("Time distribution of Old Bmon digis in TS 0 of %4d, offset %.0f us bin %.0f ns; Time in TS [ns]; Nb Digis []",
         uRunId, dZoomOffset / 1e3, dBinSizeNs),  //
    iNbBinsZoom, 0.0, dZoomLength);
  TH1* hDigiTimeDistZoomBmonNew = new TH1I(
    Form("hDistZoomBmonNew_%04d", uRunId),
    Form("Time distribution of New Bmon digis in TS 0 of %4d, offset %.0f us bin %.0f ns; Time in TS [ns]; Nb Digis []",
         uRunId, dZoomOffset / 1e3, dBinSizeNs),  //
    iNbBinsZoom, 0.0, dZoomLength);
  TH1* hDigiTimeDistZoomStsSta0 = new TH1I(
    Form("hDistZoomStsSta0_%04d", uRunId),
    Form("Time distribution of Sts St 0 digis in TS 0 of %4d, offset %.0f us bin %.0f ns; Time in TS [ns]; Nb Digis []",
         uRunId, dZoomOffset / 1e3, dBinSizeNs),  //
    iNbBinsZoom, 0.0, dZoomLength);
  TH1* hDigiTimeDistZoomStsSta1 = new TH1I(
    Form("hDistZoomStsSta1_%04d", uRunId),
    Form("Time distribution of Sts St 1 digis in TS 0 of %4d, offset %.0f us bin %.0f ns; Time in TS [ns]; Nb Digis []",
         uRunId, dZoomOffset / 1e3, dBinSizeNs),  //
    iNbBinsZoom, 0.0, dZoomLength);
  TH1* hDigiTimeDistZoomStsSta2 = new TH1I(
    Form("hDistZoomStsSta2_%04d", uRunId),
    Form("Time distribution of Sts St 2 digis in TS 0 of %4d, offset %.0f us bin %.0f ns; Time in TS [ns]; Nb Digis []",
         uRunId, dZoomOffset / 1e3, dBinSizeNs),  //
    iNbBinsZoom, 0.0, dZoomLength);
  TH1* hDigiTimeDistZoomTofSta0 = new TH1I(
    Form("hDistZoomTofSta0_%04d", uRunId),
    Form("Time distribution of Tof St 0 digis in TS 0 of %4d, offset %.0f us bin %.0f ns; Time in TS [ns]; Nb Digis []",
         uRunId, dZoomOffset / 1e3, dBinSizeNs),  //
    iNbBinsZoom, 0.0, dZoomLength);

  std::vector<TH1*> vDigiTimeDistBmonOld(nentries, nullptr);
  std::vector<TH1*> vDigiTimeDistBmonNew(nentries, nullptr);
  std::vector<TH1*> vDigiTimeDistSts(nentries, nullptr);
  std::vector<TH1*> vDigiTimeDistTof(nentries, nullptr);

  std::vector<TH1*> vDigiTimeDistStsSta0(nentries, nullptr);
  std::vector<TH1*> vDigiTimeDistStsSta1(nentries, nullptr);
  std::vector<TH1*> vDigiTimeDistStsSta2(nentries, nullptr);

  std::vector<TH1*> vDigiTimeDistTrd(nentries, nullptr);
  std::vector<TH1*> vDigiTimeDistRich(nentries, nullptr);

  /// Loop on timeslices
  for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
    //if (iEntry % 10 == 0 ) std::cout << "Entry " << iEntry << " / " << nentries << std::endl;

    vDigiTimeDistBmonOld[iEntry] =
      new TH1I(Form("hDistBmonOld_%03d", iEntry),
               Form("Time distribution of Old Bmon digis in TS %3d; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);
    vDigiTimeDistBmonNew[iEntry] =
      new TH1I(Form("hDistBmonNew_%03d", iEntry),
               Form("Time distribution of New Bmon digis in TS %3d; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);
    vDigiTimeDistSts[iEntry] =
      new TH1I(Form("hDistSts_%03d", iEntry),
               Form("Time distribution of Sts digis in TS %3d; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);
    vDigiTimeDistTof[iEntry] =
      new TH1I(Form("hDistTof_%03d", iEntry),
               Form("Time distribution of Tof digis in TS %3d; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);

    vDigiTimeDistStsSta0[iEntry] =
      new TH1I(Form("hDistStsSta0_%03d", iEntry),
               Form("Time distribution of Sts digis in TS %3d, Station 0; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);

    vDigiTimeDistStsSta1[iEntry] =
      new TH1I(Form("hDistStsSta1_%03d", iEntry),
               Form("Time distribution of Sts digis in TS %3d, Station 1; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);

    vDigiTimeDistStsSta2[iEntry] =
      new TH1I(Form("hDistStsSta2_%03d", iEntry),
               Form("Time distribution of Sts digis in TS %3d, Station 2; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);

    vDigiTimeDistTrd[iEntry] =
      new TH1I(Form("hDistTrd_%03d", iEntry),
               Form("Time distribution of Trd digis in TS %3d; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);

    vDigiTimeDistRich[iEntry] =
      new TH1I(Form("hDistRich_%03d", iEntry),
               Form("Time distribution of Rich digis in TS %3d; Time in TS [ns]; Nb Digis []", iEntry),  //
               iNbBinsRange, 0.0, dTsSizeNs);


    tree->GetEntry(iEntry);
    uint32_t nDigisBmon = vDigisBmon->size();
    uint32_t nDigisSts  = vDigisSts->size();
    uint32_t nDigisTof  = vDigisTof->size();

    std::vector<CbmBmonDigi> vDigisOld;
    std::vector<CbmBmonDigi> vDigisNew;
    vDigisOld.reserve(nDigisBmon);
    vDigisNew.reserve(nDigisBmon);
    for (auto& digi : *vDigisBmon) {
      if (1 == CbmTofAddress::GetChannelSide(digi.GetAddress())) {
        if (CbmTofAddress::GetChannelId(digi.GetAddress()) < 4) {
          vDigisNew.push_back(digi);
        }
        else {
          LOG(fatal) << "Bad sCVD channel: " << CbmTofAddress::GetChannelId(digi.GetAddress());
        }
      }
      else {
        vDigisOld.push_back(digi);
      }
    }
    std::cout << "TS " << iEntry << ", BMON Digis: " << nDigisBmon << " => " << vDigisOld.size() << " Old + "
              << vDigisNew.size() << " sCVD, STS Digis: " << nDigisSts << ", TOF Digis: " << nDigisTof << std::endl;

    for (auto itOld = vDigisOld.begin(); itOld != vDigisOld.end(); ++itOld) {
      vDigiTimeDistBmonOld[iEntry]->Fill((*itOld).GetTime());
    }
    for (auto itNew = vDigisNew.begin(); itNew != vDigisNew.end(); ++itNew) {
      vDigiTimeDistBmonNew[iEntry]->Fill((*itNew).GetTime());
    }
    for (auto itSts = vDigisSts->begin(); itSts != vDigisSts->end(); ++itSts) {
      auto timeInTs = (*itSts).GetTime();
      vDigiTimeDistSts[iEntry]->Fill(timeInTs);

      auto addr    = (*itSts).GetAddress();
      auto station = CbmStsAddress::GetElementId(addr, EStsElementLevel::kStsUnit);

      if (0 == station) {
        vDigiTimeDistStsSta0[iEntry]->Fill(timeInTs);
      }
      else if (1 == station) {
        vDigiTimeDistStsSta1[iEntry]->Fill(timeInTs);
      }
      else if (2 == station) {
        vDigiTimeDistStsSta2[iEntry]->Fill(timeInTs);
      }
    }
    for (auto itTof = vDigisTof->begin(); itTof != vDigisTof->end(); ++itTof) {
      vDigiTimeDistTof[iEntry]->Fill((*itTof).GetTime());
    }

    uint32_t nDigisTrd  = vDigisTrd->size();
    uint32_t nDigisRich = vDigisRich->size();
    for (auto itTrd = vDigisTrd->begin(); itTrd != vDigisTrd->end(); ++itTrd) {
      vDigiTimeDistTrd[iEntry]->Fill((*itTrd).GetTime());
    }
    for (auto itRich = vDigisRich->begin(); itRich != vDigisRich->end(); ++itRich) {
      vDigiTimeDistRich[iEntry]->Fill((*itRich).GetTime());
    }

    if (0 == iEntry) {
      for (auto itOld = vDigisOld.begin(); itOld != vDigisOld.end(); ++itOld) {
        auto timeInTs = (*itOld).GetTime();

        if (timeInTs < dZoomOffset) continue;
        if (dZoomOffset + dZoomLength < timeInTs) break;

        hDigiTimeDistZoomBmonOld->Fill(timeInTs - dZoomOffset);
      }
      for (auto itNew = vDigisNew.begin(); itNew != vDigisNew.end(); ++itNew) {
        auto timeInTs = (*itNew).GetTime();

        if (timeInTs < dZoomOffset) continue;
        if (dZoomOffset + dZoomLength < timeInTs) break;

        hDigiTimeDistZoomBmonNew->Fill(timeInTs - dZoomOffset);
      }
      for (auto itSts = vDigisSts->begin(); itSts != vDigisSts->end(); ++itSts) {
        auto timeInTs = (*itSts).GetTime();

        if (timeInTs < dZoomOffset) continue;
        if (dZoomOffset + dZoomLength < timeInTs) break;

        auto addr    = (*itSts).GetAddress();
        auto station = CbmStsAddress::GetElementId(addr, EStsElementLevel::kStsUnit);

        if (0 == station) {
          hDigiTimeDistZoomStsSta0->Fill(timeInTs - dZoomOffset);
        }
        else if (1 == station) {
          hDigiTimeDistZoomStsSta1->Fill(timeInTs - dZoomOffset);
        }
        else if (2 == station) {
          hDigiTimeDistZoomStsSta2->Fill(timeInTs - dZoomOffset);
        }
      }
      for (auto itTof = vDigisTof->begin(); itTof != vDigisTof->end(); ++itTof) {
        auto timeInTs = (*itTof).GetTime();

        if (timeInTs < dZoomOffset) continue;
        if (dZoomOffset + dZoomLength < timeInTs) break;

        hDigiTimeDistZoomTofSta0->Fill(timeInTs - dZoomOffset);
      }
    }
  }

  TCanvas* fCanvFirstTs = new TCanvas("canvFirstTs", "Digis time distribution first TS, Bmon Old/New + STS + TOF");
  /*
  THStack* pStackTimeDist   = new THStack(
      "pStackTimeDist",
      Form("Digis time distribution first TS, zoomed %1.0f us at %1.0f us, run %4d", dZoomLength/1e3, dZoomOffset/1e3,
          uRunId));
  hDigiTimeDistZoomBmonOld->SetLineColor(kRed);
  hDigiTimeDistZoomBmonNew->SetLineColor(kOrange + 1);
  hDigiTimeDistZoomStsSta0->SetLineColor(kBlue);
  hDigiTimeDistZoomStsSta1->SetLineColor(kMagenta);
  hDigiTimeDistZoomStsSta2->SetLineColor(kCyan);
  hDigiTimeDistZoomTofSta0->SetLineColor(kBlack);

  hDigiTimeDistZoomBmonOld->SetLineWidth(2);
  hDigiTimeDistZoomBmonNew->SetLineWidth(2);
  hDigiTimeDistZoomStsSta0->SetLineWidth(2);
  hDigiTimeDistZoomStsSta1->SetLineWidth(2);
  hDigiTimeDistZoomStsSta2->SetLineWidth(2);
  hDigiTimeDistZoomTofSta0->SetLineWidth(2);

  pStackTimeDist->Add(hDigiTimeDistZoomStsSta0);
  pStackTimeDist->Add(hDigiTimeDistZoomStsSta1);
  pStackTimeDist->Add(hDigiTimeDistZoomStsSta2);
  pStackTimeDist->Add(hDigiTimeDistZoomTofSta0);
  pStackTimeDist->Add(hDigiTimeDistZoomBmonOld);
  pStackTimeDist->Add(hDigiTimeDistZoomBmonNew);

  fCanvFirstTs->cd(1);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackTimeDist->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");
*/

  fCanvFirstTs->Divide(3, 2);

  fCanvFirstTs->cd(1);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiTimeDistZoomBmonOld->Draw("hist");

  fCanvFirstTs->cd(2);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiTimeDistZoomBmonNew->Draw("hist");

  fCanvFirstTs->cd(3);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiTimeDistZoomStsSta0->Draw("hist");

  fCanvFirstTs->cd(4);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiTimeDistZoomStsSta1->Draw("hist");

  fCanvFirstTs->cd(5);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiTimeDistZoomStsSta2->Draw("hist");

  fCanvFirstTs->cd(6);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiTimeDistZoomTofSta0->Draw("hist");

  /// Loop on bins in each timeslice
  TH2* hDigiMulNewOld = new TH2I(
    "hDigiMulNewOld",
    Form("Digi Multiplicity per %3.0f ns bin, Old VS New Bmon, %04d; New [Digis]; Old [Digis]", dBinSizeNs, uRunId),  //
    30, -0.5, 29.5, 30, -0.5, 29.5);
  TH2* hDigiMulOldSts = new TH2I(
    "hDigiMulOldSts",
    Form("Digi Multiplicity per %3.0f ns bin, STS VS Old Bmon, %04d; Old [Digis]; STS [Digis]", dBinSizeNs, uRunId),  //
    30, -0.5, 29.5, 1000, -0.5, 999.5);
  TH2* hDigiMulOldTof = new TH2I(
    "hDigiMulOldTof",
    Form("Digi Multiplicity per %3.0f ns bin, TOF VS Old Bmon, %04d; Old [Digis]; TOF [Digis]", dBinSizeNs, uRunId),  //
    30, -0.5, 29.5, 500, -0.5, 499.5);
  TH2* hDigiMulNewSts = new TH2I(
    "hDigiMulNewSts",
    Form("Digi Multiplicity per %3.0f ns bin, STS VS New Bmon, %04d; New [Digis]; STS [Digis]", dBinSizeNs, uRunId),  //
    30, -0.5, 29.5, 1000, -0.5, 999.5);
  TH2* hDigiMulNewTof = new TH2I(
    "hDigiMulNewTof",
    Form("Digi Multiplicity per %3.0f ns bin, TOF VS New Bmon, %04d; New [Digis]; TOF [Digis]", dBinSizeNs, uRunId),  //
    30, -0.5, 29.5, 500, -0.5, 499.5);
  TH2* hDigiMulStsTof = new TH2I(
    "hDigiMulStsTof",
    Form("Digi Multiplicity per %3.0f ns bin, TOF VS STS, %04d; Sts [Digis]; TOF [Digis]", dBinSizeNs, uRunId),  //
    100, -0.5, 999.5, 50, -0.5, 499.5);
  TH2* hDigiMulTofTrd = new TH2I(
    "hDigiMulTofTrd",
    Form("Digi Multiplicity per %3.0f ns bin, TRD VS TOF, %04d; TOF [Digis]; TRD [Digis]", dBinSizeNs, uRunId),  //
    500, -0.5, 499.5, 500, -0.5, 499.5);
  TH2* hDigiMulTofRich = new TH2I(
    "hDigiMulTofRich",
    Form("Digi Multiplicity per %3.0f ns bin, RICH VS TOF, %04d; TOF [Digis]; RICH [Digis]", dBinSizeNs, uRunId),  //
    500, -0.5, 499.5, 300, -0.5, 299.5);

  TH2* hDigiMulEvoBmonOld =
    new TH2I(Form("hDigiMulEvoBmonOld_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin of Old Bmon digis in %4d; TS [ns]; Nb Digis per bin []",
                  dBinSizeNs, uRunId),  //
             nentries * 10, 0.0, nentries, 30, -0.5, 29.5);
  TH2* hDigiMulEvoBmonNew =
    new TH2I(Form("hDigiMulEvoBmonNew_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of New Bmon digis in %4d; TS [ns]; Nb Digis per bin []",
                  dBinSizeNs, uRunId),  //
             nentries * 10, 0.0, nentries, 30, -0.5, 29.5);
  TH2* hDigiMulEvoStsSta0 =
    new TH2I(Form("hDigiMulEvoStsSta0_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of Sts St 0 digis in %4d; TS [ns]; Nb Digis per bin []",
                  dBinSizeNs, uRunId),  //
             nentries * 10, 0.0, nentries, 1000, -0.5, 999.5);
  TH2* hDigiMulEvoStsSta1 =
    new TH2I(Form("hDigiMulEvoStsSta1_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of Sts St 1 digis in %4d; TS [ns]; Nb Digis per bin []",
                  dBinSizeNs, uRunId),  //
             nentries * 10, 0.0, nentries, 1000, -0.5, 999.5);
  TH2* hDigiMulEvoStsSta2 =
    new TH2I(Form("hDigiMulEvoStsSta2_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of Sts St 2 digis in %4d; TS [ns]; Nb Digis per bin []",
                  dBinSizeNs, uRunId),  //
             nentries * 10, 0.0, nentries, 1000, -0.5, 999.5);
  TH2* hDigiMulEvoTofSta0 =
    new TH2I(Form("hDigiMulEvoTofSta0_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of Tof St 0 digis in %4d; TS [ns]; Nb Digis per bin []",
                  dBinSizeNs, uRunId),  //
             nentries * 10, 0.0, nentries, 500, -0.5, 499.5);
  TH2* hDigiMulEvoTrd =
    new TH2I(Form("hDigiMulEvoTrd_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of Trd digis in %4d; TS [ns]; Nb Digis per bin []", dBinSizeNs,
                  uRunId),  //
             nentries * 10, 0.0, nentries, 500, -0.5, 499.5);
  TH2* hDigiMulEvoRich =
    new TH2I(Form("hDigiMulEvoRich_%04d", uRunId),
             Form("Digi Multiplicity per %.0f ns bin  of Rich digis in %4d; TS [ns]; Nb Digis per bin []", dBinSizeNs,
                  uRunId),  //
             nentries * 10, 0.0, nentries, 300, -0.5, 299.5);

  for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
    for (Int_t iBin = 0; iBin < iNbBinsRange; iBin++) {
      Int_t iNbDigisNew  = vDigiTimeDistBmonNew[iEntry]->GetBinContent(iBin);
      Int_t iNbDigisOld  = vDigiTimeDistBmonOld[iEntry]->GetBinContent(iBin);
      Int_t iNbDigisSts  = vDigiTimeDistSts[iEntry]->GetBinContent(iBin);
      Int_t iNbDigisTof  = vDigiTimeDistTof[iEntry]->GetBinContent(iBin);
      Int_t iNbDigisTrd  = vDigiTimeDistTrd[iEntry]->GetBinContent(iBin);
      Int_t iNbDigisRich = vDigiTimeDistRich[iEntry]->GetBinContent(iBin);
      if (0 < iNbDigisNew || 0 < iNbDigisOld) {
        hDigiMulNewOld->Fill(iNbDigisNew, iNbDigisOld);
      }
      if (0 < iNbDigisOld && 0 < iNbDigisSts) {
        hDigiMulOldSts->Fill(iNbDigisOld, iNbDigisSts);
      }
      if (0 < iNbDigisOld && 0 < iNbDigisTof) {
        hDigiMulOldTof->Fill(iNbDigisOld, iNbDigisTof);
      }
      if (0 < iNbDigisNew && 0 < iNbDigisSts) {
        hDigiMulNewSts->Fill(iNbDigisNew, iNbDigisSts);
      }
      if (0 < iNbDigisNew && 0 < iNbDigisTof) {
        hDigiMulNewTof->Fill(iNbDigisNew, iNbDigisTof);
      }
      if (0 < iNbDigisSts && 0 < iNbDigisTof) {
        hDigiMulStsTof->Fill(iNbDigisSts, iNbDigisTof);
      }
      if (0 < iNbDigisTrd && 0 < iNbDigisTof) {
        hDigiMulTofTrd->Fill(iNbDigisTof, iNbDigisTrd);
      }
      if (0 < iNbDigisRich && 0 < iNbDigisTof) {
        hDigiMulTofRich->Fill(iNbDigisTof, iNbDigisRich);
      }

      double_t dTsFractional = (dBinSizeNs * iBin) / dTsSizeNs + iEntry;
      hDigiMulEvoBmonOld->Fill(dTsFractional, iNbDigisOld);
      hDigiMulEvoBmonNew->Fill(dTsFractional, iNbDigisNew);
      hDigiMulEvoStsSta0->Fill(dTsFractional, vDigiTimeDistStsSta0[iEntry]->GetBinContent(iBin));
      hDigiMulEvoStsSta1->Fill(dTsFractional, vDigiTimeDistStsSta1[iEntry]->GetBinContent(iBin));
      hDigiMulEvoStsSta2->Fill(dTsFractional, vDigiTimeDistStsSta2[iEntry]->GetBinContent(iBin));
      hDigiMulEvoTofSta0->Fill(dTsFractional, iNbDigisTof);
      hDigiMulEvoTrd->Fill(dTsFractional, iNbDigisTrd);
      hDigiMulEvoRich->Fill(dTsFractional, iNbDigisRich);
    }
  }
  TCanvas* fCanvCorrels =
    new TCanvas("canvCorrels", Form("Digi Multiplicity per %3.0f ns bin, New Bmon vs Old Bmon/STS/TOF", dBinSizeNs));
  fCanvCorrels->Divide(4, 2);

  fCanvCorrels->cd(1);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulNewOld->Draw("colz");
  TProfile* hDigiMulNewOldCorr = hDigiMulNewOld->ProfileX("hDigiMulNewOldCorr");
  hDigiMulNewOldCorr->Draw("same");

  fCanvCorrels->cd(2);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulOldSts->Draw("colz");
  TProfile* hDigiMulOldStsCorr = hDigiMulOldSts->ProfileX("hDigiMulOldStsCorr");
  hDigiMulOldStsCorr->Draw("same");

  fCanvCorrels->cd(3);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulOldTof->Draw("colz");
  TProfile* hDigiMulOldTofCorr = hDigiMulOldTof->ProfileX("hDigiMulOldTofCorr");
  hDigiMulOldTofCorr->Draw("same");

  fCanvCorrels->cd(5);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulStsTof->Draw("colz");
  TProfile* hDigiMulStsTofCorr = hDigiMulStsTof->ProfileX("hDigiMulStsTofCorr");
  hDigiMulStsTofCorr->Draw("same");

  fCanvCorrels->cd(6);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulNewSts->Draw("colz");
  TProfile* hDigiMulNewStsCorr = hDigiMulNewSts->ProfileX("hDigiMulNewStsCorr");
  hDigiMulNewStsCorr->Draw("same");

  fCanvCorrels->cd(7);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulNewTof->Draw("colz");
  TProfile* hDigiMulNewTofCorr = hDigiMulNewTof->ProfileX("hDigiMulNewTofCorr");
  hDigiMulNewTofCorr->Draw("same");

  fCanvCorrels->cd(4);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulTofTrd->Draw("colz");
  TProfile* hDigiMulTofTrdCorr = hDigiMulTofTrd->ProfileX("hDigiMulTofTrdCorr");
  hDigiMulTofTrdCorr->Draw("same");

  fCanvCorrels->cd(8);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulTofRich->Draw("colz");
  TProfile* hDigiMulTofRichCorr = hDigiMulTofRich->ProfileX("hDigiMulTofRichCorr");
  hDigiMulTofRichCorr->Draw("same");

  TCanvas* fCanvEvo = new TCanvas("canvEvo", Form("Digi Multiplicity per %3.0f ns bin vs timeslice", dBinSizeNs));
  fCanvEvo->Divide(4, 2);

  fCanvEvo->cd(1);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoBmonOld->Draw("colz");
  TProfile* hDigiMulEvoBmonOldMean = hDigiMulEvoBmonOld->ProfileX("hDigiMulEvoBmonOldMean");
  hDigiMulEvoBmonOldMean->Draw("same");

  fCanvEvo->cd(2);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoBmonNew->Draw("colz");
  TProfile* hDigiMulEvoBmonNewMean = hDigiMulEvoBmonNew->ProfileX("hDigiMulEvoBmonNewMean");
  hDigiMulEvoBmonNewMean->Draw("same");

  fCanvEvo->cd(3);
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoStsSta0->Draw("colz");
  TProfile* hDigiMulEvoStsSta0Mean = hDigiMulEvoStsSta0->ProfileX("hDigiMulEvoStsSta0Mean");
  hDigiMulEvoStsSta0Mean->Draw("same");

  fCanvEvo->cd(4);
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoStsSta1->Draw("colz");
  TProfile* hDigiMulEvoStsSta1Mean = hDigiMulEvoStsSta1->ProfileX("hDigiMulEvoStsSta1Mean");
  hDigiMulEvoStsSta1Mean->Draw("same");

  fCanvEvo->cd(5);
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoStsSta2->Draw("colz");
  TProfile* hDigiMulEvoStsSta2Mean = hDigiMulEvoStsSta2->ProfileX("hDigiMulEvoStsSta2Mean");
  hDigiMulEvoStsSta2Mean->Draw("same");

  fCanvEvo->cd(6);
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoTofSta0->Draw("colz");
  TProfile* hDigiMulEvoTofSta0Mean = hDigiMulEvoTofSta0->ProfileX("hDigiMulEvoTofSta0Mean");
  hDigiMulEvoTofSta0Mean->Draw("same");

  fCanvEvo->cd(7);
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoTrd->Draw("colz");
  TProfile* hDigiMulEvoTrdMean = hDigiMulEvoTrd->ProfileX("hDigiMulEvoTrdMean");
  hDigiMulEvoTrdMean->Draw("same");

  fCanvEvo->cd(8);
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  hDigiMulEvoRich->Draw("colz");
  TProfile* hDigiMulEvoRichMean = hDigiMulEvoRich->ProfileX("hDigiMulEvoRichMean");
  hDigiMulEvoRichMean->Draw("same");

  TFile* outFile =
    new TFile(Form("data/first_spill_bunches_%04u_%03luTs_%.0fNs.root", uRunId, numTimeslices, dBinSizeNs), "RECREATE");
  outFile->cd();
  fCanvFirstTs->Write();
  fCanvCorrels->Write();
  fCanvEvo->Write();

  gROOT->cd();

  outFile->Close();
}
