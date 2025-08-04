/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

/** @brief Macro for check of bmon digis time spread depending on bin size in direct vector from legacy or online
 *         unpacking
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 ** @param dTsSizeNs      Size of one Ts in ns = Total length of one time disribution histogram
 **/
void first_spill_bmon_bins(TString inputFileName, uint32_t uRunId, size_t numTimeslices = 0,
                           double_t dTsSizeNs = 128000000.0)
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


  std::vector<double_t> vdBinSizes = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0,
                                      200,  400,  600,  800,  1e3,  2e3,  4e3,  6e3,  8e3,  1e4};
  std::vector<int32_t> viNbBinsRange(vdBinSizes.size(), 1);

  std::vector<std::vector<TH1*>> vDigiTimeDistBmonOld;
  std::vector<std::vector<TH1*>> vDigiTimeDistBmonNew;
  vDigiTimeDistBmonOld.resize(vdBinSizes.size());
  vDigiTimeDistBmonNew.resize(vdBinSizes.size());

  for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
    vDigiTimeDistBmonOld[uBinSz].resize(nentries);
    vDigiTimeDistBmonNew[uBinSz].resize(nentries);
    viNbBinsRange[uBinSz] = std::round(dTsSizeNs / vdBinSizes[uBinSz]);
  }
  TH2* vDigiTimeDistBmonOld_tenUs =
    new TH2I("vDigiTimeDistBmonOld_tenUs", "Time distribution of Old Bmon digis; TS; time in TS [ns];  Nb Digis []",
             nentries, 0, nentries, 12800, 0.0, dTsSizeNs);
  TH2* vDigiTimeDistBmonNew_tenUs =
    new TH2I("vDigiTimeDistBmonNew_tenUs", "Time distribution of Old Bmon digis; TS; time in TS [ns];  Nb Digis []",
             nentries, 0, nentries, 12800, 0.0, dTsSizeNs);


  /// Loop on timeslices
  for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
    //if (iEntry % 10 == 0 ) std::cout << "Entry " << iEntry << " / " << nentries << std::endl;

    for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
      vDigiTimeDistBmonOld[uBinSz][iEntry] = new TH1I(
        Form("hDistBmonOld_%03d_%03d", uBinSz, iEntry),
        Form("Time distribution of Old Bmon digis in TS %3d, bin %.0f ns; Time in TS [ns]; Nb Digis []", iEntry,
             vdBinSizes[uBinSz]),  //
        viNbBinsRange[uBinSz], 0.0, dTsSizeNs);
      vDigiTimeDistBmonNew[uBinSz][iEntry] = new TH1I(
        Form("hDistBmonNew_%03d_%03d", uBinSz, iEntry),
        Form("Time distribution of New Bmon digis in TS %3d, bin %.0f ns; Time in TS [ns]; Nb Digis []", iEntry,
             vdBinSizes[uBinSz]),  //
        viNbBinsRange[uBinSz], 0.0, dTsSizeNs);
    }

    tree->GetEntry(iEntry);
    uint32_t nDigisBmon = vDigisBmon->size();

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
              << vDigisNew.size() << " sCVD" << std::endl;

    for (auto itOld = vDigisOld.begin(); itOld != vDigisOld.end(); ++itOld) {
      double_t dTime = (*itOld).GetTime();
      for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
        vDigiTimeDistBmonOld[uBinSz][iEntry]->Fill(dTime);
      }
      vDigiTimeDistBmonOld_tenUs->Fill(iEntry, dTime);
    }
    for (auto itNew = vDigisNew.begin(); itNew != vDigisNew.end(); ++itNew) {
      double_t dTime = (*itNew).GetTime();
      for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
        vDigiTimeDistBmonNew[uBinSz][iEntry]->Fill(dTime);
      }
      vDigiTimeDistBmonNew_tenUs->Fill(iEntry, dTime);
    }
  }

  /// Loop on bins in each timeslice
  std::vector<TH1*> vhDigiMulOld(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhDigiMulNew(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhGapDurationNsOld(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhGapDurationNsNew(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhGapDurationUsOld(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhGapDurationUsNew(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhGapDurationMsOld(vdBinSizes.size(), nullptr);
  std::vector<TH1*> vhGapDurationMsNew(vdBinSizes.size(), nullptr);

  constexpr double epsilon = std::numeric_limits<double>::epsilon();
  int32_t iNbBinsDurationBin0 =
    std::floor(50000.0 / vdBinSizes[0]) + (epsilon < std::fabs(std::fmod(50000.0, vdBinSizes[0])) ? 1 : 0);

  TH1* vhGapDurationNsOld_bin0 =
    new TH1I("hGapDurationNsOld_bin0",
             Form("Gaps duration, N bins * %3.0f ns bin, Old Bmon, %04d; Gap duration [ns]", vdBinSizes[0], uRunId),  //
             iNbBinsDurationBin0, 0, 50000.0 + (std::fmod(50000.0, vdBinSizes[0])));
  TH1* vhGapDurationNsNew_bin0 =
    new TH1I("hGapDurationNsNew_bin0",
             Form("Gaps duration, N bins * %3.0f ns bin, New Bmon, %04d; Gap duration [ns]", vdBinSizes[0], uRunId),  //
             iNbBinsDurationBin0, 0, 50000.0 + (std::fmod(50000.0, vdBinSizes[0])));

  TH2* vhGapDurationVsPrevNsOld_bin0 =
    new TH2I("hGapDurationVsPrevNsOld_bin0",
             Form("Gaps duration, N bins * %3.0f ns bin, Old Bmon, %04d; Prev Gap duration [ns]; Gap duration [ns]",
                  vdBinSizes[0], uRunId),                                                //
             iNbBinsDurationBin0 / 5, 0, 10000.0 + (std::fmod(10000.0, vdBinSizes[0])),  //
             iNbBinsDurationBin0 / 5, 0, 10000.0 + (std::fmod(10000.0, vdBinSizes[0])));
  TH2* vhGapDurationVsPrevNsNew_bin0 =
    new TH2I("hGapDurationVsPrevNsNew_bin0",
             Form("Gaps duration, N bins * %3.0f ns bin, New Bmon, %04d; Prev Gap duration [ns];; Gap duration [ns]",
                  vdBinSizes[0], uRunId),                                                //
             iNbBinsDurationBin0 / 5, 0, 10000.0 + (std::fmod(10000.0, vdBinSizes[0])),  //
             iNbBinsDurationBin0 / 5, 0, 10000.0 + (std::fmod(10000.0, vdBinSizes[0])));

  for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
    vhDigiMulOld[uBinSz] =
      new TH1I(Form("hDigiMulOld_%03d", uBinSz),
               Form("Digi Multiplicity per %3.0f ns bin, Old Bmon, %04d; Old [Digis]", vdBinSizes[uBinSz], uRunId),  //
               30, -0.5, 29.5);
    vhDigiMulNew[uBinSz] =
      new TH1I(Form("hDigiMulNew_%03d", uBinSz),
               Form("Digi Multiplicity per %3.0f ns bin, New Bmon, %04d; New [Digis]", vdBinSizes[uBinSz], uRunId),  //
               30, -0.5, 29.5);

    if (vdBinSizes[uBinSz] < 1000.0) {
      int32_t iNbBinsDuration =
        std::floor(1000.0 / vdBinSizes[uBinSz]) + (epsilon < std::fabs(std::fmod(1000.0, vdBinSizes[uBinSz])) ? 1 : 0);

      vhGapDurationNsOld[uBinSz] = new TH1I(
        Form("hGapDurationNsOld_%03d", uBinSz),
        Form("Gaps duration, N bins * %3.0f ns bin, Old Bmon, %04d; Gap duration [ns]", vdBinSizes[uBinSz], uRunId),  //
        iNbBinsDuration, 0, 1000.0 + (std::fmod(1000.0, vdBinSizes[uBinSz])));
      vhGapDurationNsNew[uBinSz] = new TH1I(
        Form("hGapDurationNsNew_%03d", uBinSz),
        Form("Gaps duration, N bins * %3.0f ns bin, New Bmon, %04d; Gap duration [ns]", vdBinSizes[uBinSz], uRunId),  //
        iNbBinsDuration, 0, 1000.0 + (std::fmod(1000.0, vdBinSizes[uBinSz])));
    }
    int32_t iNbBinsDuration = 2000;
    double_t dDurationMax   = 1e6;
    if (1e3 <= vdBinSizes[uBinSz] && vdBinSizes[uBinSz] < 1e6) {
      iNbBinsDuration =
        std::floor(1e6 / vdBinSizes[uBinSz]) + (epsilon < std::fabs(std::fmod(1e6, vdBinSizes[uBinSz])) ? 1 : 0);
      dDurationMax = 1e6 + (std::fmod(1e6, vdBinSizes[uBinSz]));
    }

    vhGapDurationUsOld[uBinSz] = new TH1I(
      Form("hGapDurationUsOld_%03d", uBinSz),
      Form("Gaps duration, N bins * %3.0f ns bin, Old Bmon, %04d; Gap duration [ns]", vdBinSizes[uBinSz], uRunId),  //
      iNbBinsDuration, 0, dDurationMax);
    vhGapDurationUsNew[uBinSz] = new TH1I(
      Form("hGapDurationUsNew_%03d", uBinSz),
      Form("Gaps duration, N bins * %3.0f ns bin, New Bmon, %04d; Gap duration [ns]", vdBinSizes[uBinSz], uRunId),  //
      iNbBinsDuration, 0, dDurationMax);

    vhGapDurationMsOld[uBinSz] = new TH1I(
      Form("hGapDurationMsOld_%03d", uBinSz),
      Form("Gaps duration, N bins * %3.0f ns bin, Old Bmon, %04d; Gap duration [ms]", vdBinSizes[uBinSz], uRunId),  //
      200, 0, 200);
    vhGapDurationMsNew[uBinSz] = new TH1I(
      Form("hGapDuratioMsNew_%03d", uBinSz),
      Form("Gaps duration, N bins * %3.0f ns bin, New Bmon, %04d; Gap duration [ms]", vdBinSizes[uBinSz], uRunId),  //
      200, 0, 200);
  }

  for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
    for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
      bool bLastEmptyOld           = false;
      bool bLastEmptyNew           = false;
      int32_t iNbEmptiesOld        = 0;
      int32_t iNbEmptiesNew        = 0;
      double_t dLastGapDurationOld = -1.0;
      double_t dLastGapDurationNew = -1.0;
      for (Int_t iBin = 0; iBin < viNbBinsRange[uBinSz]; iBin++) {
        int32_t iCountsOld = vDigiTimeDistBmonOld[uBinSz][iEntry]->GetBinContent(iBin);
        int32_t iCountsNew = vDigiTimeDistBmonNew[uBinSz][iEntry]->GetBinContent(iBin);

        if (0 == iCountsOld) {
          bLastEmptyOld = true;
          iNbEmptiesOld++;
        }
        else {
          vhDigiMulOld[uBinSz]->Fill(iCountsOld);
          if (bLastEmptyOld) {

            double_t dGapDuration = iNbEmptiesOld * vdBinSizes[uBinSz];
            if (dGapDuration < 1e3 && vdBinSizes[uBinSz] < 1000.0) {
              vhGapDurationNsOld[uBinSz]->Fill(dGapDuration);
            }
            if (dGapDuration < 1e6) {
              vhGapDurationUsOld[uBinSz]->Fill(dGapDuration);
            }
            vhGapDurationMsOld[uBinSz]->Fill(dGapDuration / 1e6);

            if (0 == uBinSz) {
              vhGapDurationNsOld_bin0->Fill(dGapDuration);
              if (0.0 < dLastGapDurationOld) {
                vhGapDurationVsPrevNsOld_bin0->Fill(dLastGapDurationOld, dGapDuration);
              }
              dLastGapDurationOld = dGapDuration;
            }

            bLastEmptyOld = false;
            iNbEmptiesOld = 0;
          }
        }

        if (0 == iCountsNew) {
          bLastEmptyNew = true;
          iNbEmptiesNew++;
        }
        else {
          vhDigiMulNew[uBinSz]->Fill(iCountsNew);
          if (bLastEmptyNew) {
            double_t dGapDuration = iNbEmptiesNew * vdBinSizes[uBinSz];
            if (dGapDuration < 1e3) {
              vhGapDurationNsNew[uBinSz]->Fill(dGapDuration);
            }
            if (dGapDuration < 1e6) {
              vhGapDurationUsNew[uBinSz]->Fill(dGapDuration);
            }
            vhGapDurationMsNew[uBinSz]->Fill(dGapDuration / 1e6);

            if (0 == uBinSz) {
              vhGapDurationNsNew_bin0->Fill(dGapDuration);
              if (0.0 < dLastGapDurationNew) {
                vhGapDurationVsPrevNsNew_bin0->Fill(dLastGapDurationNew, dGapDuration);
              }
              dLastGapDurationNew = dGapDuration;
            }

            bLastEmptyNew = false;
            iNbEmptiesNew = 0;
          }
        }
      }
    }
  }
  TCanvas* fCanvBinSizes =
    new TCanvas("canvBinSizes", "Digi Multiplicity per bin, as function of bin size, Old and New Bmon");
  fCanvBinSizes->Divide(2);

  const TArrayI& palette = TColor::GetPalette();
  int32_t iNbColors      = palette.GetSize();

  THStack* pStackOld =
    new THStack("pStackOld", Form("Digi Multiplicity per bin, as function of bin size, Old Bmon, run %4d", uRunId));
  THStack* pStackNew =
    new THStack("pStackNew", Form("Digi Multiplicity per bin, as function of bin size, New Bmon, run %4d", uRunId));
  for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
    int32_t iColor = iNbColors * (static_cast<double_t>(uBinSz) / vdBinSizes.size());
    vhDigiMulOld[uBinSz]->SetLineColor(palette[iColor]);
    vhDigiMulOld[uBinSz]->SetLineWidth(2);
    pStackOld->Add(vhDigiMulOld[uBinSz]);

    vhDigiMulNew[uBinSz]->SetLineColor(palette[iColor]);
    vhDigiMulNew[uBinSz]->SetLineWidth(2);
    pStackNew->Add(vhDigiMulNew[uBinSz]);

    std::cout << Form("Bin %5.0f => Mean Old %5.2f New %5.2f", vdBinSizes[uBinSz], vhDigiMulOld[uBinSz]->GetMean(),
                      vhDigiMulNew[uBinSz]->GetMean())
              << std::endl;
  }

  fCanvBinSizes->cd(1);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackOld->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  fCanvBinSizes->cd(2);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackNew->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  TCanvas* fCanvGapsDuration =
    new TCanvas("canvGapsDuration", "Gaps durations between digis, as function of bin size, Old and New Bmon");
  fCanvGapsDuration->Divide(3, 2);

  THStack* pStackGapsNsOld =
    new THStack("pStackGapsNsOld",
                Form("Gaps durations between digis, as function of bin size, ns scale, Old Bmon, run %4d", uRunId));
  THStack* pStackGapsNsNew =
    new THStack("pStackGapsNsNew",
                Form("Gaps durations between digis, as function of bin size, ns scale, New Bmon, run %4d", uRunId));

  THStack* pStackGapsUsOld =
    new THStack("pStackGapsUsOld",
                Form("Gaps durations between digis, as function of bin size, us scale, Old Bmon, run %4d", uRunId));
  THStack* pStackGapsUsNew =
    new THStack("pStackGapsUsNew",
                Form("Gaps durations between digis, as function of bin size, us scale, New Bmon, run %4d", uRunId));

  THStack* pStackGapsMsOld =
    new THStack("pStackGapMsOld",
                Form("Gaps durations between digis, as function of bin size, ms scale, Old Bmon, run %4d", uRunId));
  THStack* pStackGapsMsNew =
    new THStack("pStackGapsMsNew",
                Form("Gaps durations between digis, as function of bin size, ms scale, New Bmon, run %4d", uRunId));
  for (uint32_t uBinSz = 0; uBinSz < vdBinSizes.size(); ++uBinSz) {
    int32_t iColor = iNbColors * (static_cast<double_t>(uBinSz) / vdBinSizes.size());

    if (vdBinSizes[uBinSz] < 1000.0) {
      vhGapDurationNsOld[uBinSz]->SetLineColor(palette[iColor]);
      vhGapDurationNsOld[uBinSz]->SetLineWidth(2);
      pStackGapsNsOld->Add(vhGapDurationNsOld[uBinSz]);

      vhGapDurationNsNew[uBinSz]->SetLineColor(palette[iColor]);
      vhGapDurationNsNew[uBinSz]->SetLineWidth(2);
      pStackGapsNsNew->Add(vhGapDurationNsNew[uBinSz]);
    }

    vhGapDurationUsOld[uBinSz]->SetLineColor(palette[iColor]);
    vhGapDurationUsOld[uBinSz]->SetLineWidth(2);
    pStackGapsUsOld->Add(vhGapDurationUsOld[uBinSz]);

    vhGapDurationUsNew[uBinSz]->SetLineColor(palette[iColor]);
    vhGapDurationUsNew[uBinSz]->SetLineWidth(2);
    pStackGapsUsNew->Add(vhGapDurationUsNew[uBinSz]);

    vhGapDurationMsOld[uBinSz]->SetLineColor(palette[iColor]);
    vhGapDurationMsOld[uBinSz]->SetLineWidth(2);
    pStackGapsMsOld->Add(vhGapDurationMsOld[uBinSz]);

    vhGapDurationMsNew[uBinSz]->SetLineColor(palette[iColor]);
    vhGapDurationMsNew[uBinSz]->SetLineWidth(2);
    pStackGapsMsNew->Add(vhGapDurationMsNew[uBinSz]);
  }

  fCanvGapsDuration->cd(1);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackGapsNsOld->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  fCanvGapsDuration->cd(4);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackGapsNsNew->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  fCanvGapsDuration->cd(2);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackGapsUsOld->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  fCanvGapsDuration->cd(5);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackGapsUsNew->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  fCanvGapsDuration->cd(3);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackGapsMsOld->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

  fCanvGapsDuration->cd(6);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  pStackGapsMsNew->Draw("hist nostack");
  gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");


  TCanvas* fCanvGapsDurationBin0 =
    new TCanvas("canvGapsDurationBin0", "Gaps durations between digis, Old and New Bmon");
  fCanvGapsDurationBin0->cd(1);
  gPad->SetLogy();
  gPad->SetGridx();
  gPad->SetGridy();
  vhGapDurationNsOld_bin0->SetLineColor(kBlue);
  vhGapDurationNsNew_bin0->SetLineColor(kRed);
  vhGapDurationNsOld_bin0->Draw("hist");
  vhGapDurationNsNew_bin0->Draw("hist same");

  TCanvas* fCanvGapsDurationVsPrevBin0 =
    new TCanvas("canvGapsDurationVsPrevBin0", "Gaps durations between digis, Old and New Bmon");
  fCanvGapsDurationVsPrevBin0->Divide(2, 2);

  fCanvGapsDurationVsPrevBin0->cd(1);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  vhGapDurationVsPrevNsOld_bin0->Draw("colz");

  fCanvGapsDurationVsPrevBin0->cd(2);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  vhGapDurationVsPrevNsNew_bin0->Draw("colz");

  fCanvGapsDurationVsPrevBin0->cd(3);
  gPad->SetLogx();
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  vhGapDurationVsPrevNsOld_bin0->Draw("colz");

  fCanvGapsDurationVsPrevBin0->cd(4);
  gPad->SetLogx();
  gPad->SetLogy();
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  vhGapDurationVsPrevNsNew_bin0->Draw("colz");

  TCanvas* fCanvDigiTimeDist_tenUs =
    new TCanvas("fCanvDigiTimeDist_tenUs", "Digi time dist, 10 us bining, per TS, Old and New Bmon");
  fCanvDigiTimeDist_tenUs->Divide(2);

  fCanvDigiTimeDist_tenUs->cd(1);
  gPad->SetGridx();
  gPad->SetGridy();
  vDigiTimeDistBmonOld_tenUs->Draw("colz");

  fCanvDigiTimeDist_tenUs->cd(2);
  gPad->SetGridx();
  gPad->SetGridy();
  vDigiTimeDistBmonNew_tenUs->Draw("colz");

  TFile* outFile = new TFile(Form("data/first_spill_bmon_bins_%04u_%03luTs.root", uRunId, numTimeslices), "RECREATE");
  outFile->cd();
  fCanvBinSizes->Write();
  fCanvGapsDuration->Write();
  fCanvGapsDurationBin0->Write();
  fCanvGapsDurationVsPrevBin0->Write();

  gROOT->cd();

  outFile->Close();
}
