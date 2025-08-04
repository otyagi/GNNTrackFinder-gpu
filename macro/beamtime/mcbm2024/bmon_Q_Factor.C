/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


double_t ExtractQFactor(TH1* pHistoIn)
{
  // Q-Factor = Max Bin Content / Mean Content of all bin in range
  // => Tend toward 1 if bins are more identical
  if (pHistoIn->Integral()) {
    return (pHistoIn->GetBinContent(pHistoIn->GetMaximumBin())) / (pHistoIn->Integral() / pHistoIn->GetNbinsX());
  }
  else {
    return 0.0;
  }
}
double_t ExtractMean(TH1* pHistoIn)
{
  // Q-Factor = Max Bin Content / Mean Content of all bin in range
  // => Tend toward 1 if bins are more identical
  if (pHistoIn->Integral()) {
    return (pHistoIn->Integral() / pHistoIn->GetNbinsX());
  }
  else {
    return 0.0;
  }
}

/** @brief Macro for check of Hades-like Q-Factor based on bmon digis, against variations of bin size and integration
 *         length
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 ** @param dTsSizeNs      Size of one Ts in ns = Total length of one time disribution histogram
 **/
void bmon_Q_Factor(TString inputFileName, uint32_t uRunId, size_t numTimeslices = 0, double_t dTsSizeNs = 128000000.0)
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

  uint32_t uMinNbBins = 10;
  uint32_t uMaxNbBins = 100000;
  /*
  /// Hint: keep fractions of TS size and under 100 us
  std::vector<double_t> vdBinSizesNs    = {10, 20, 40, 80, 100, 200, 400, 800, 1.28e3, 2.56e3, 5.12e3, 12.8e3, 25.6e3,
                                           51.2e3};  // 14 values
  /// Hint: keep fractions of TS size + multiples of bin size and above 10 us
  std::vector<double_t> vdIntegrationNs = {12.8e3, 25.6e3, 51.2e3, 102.4e3, 204.8e3, 512e3, 1.28e6, 2.56e6, 5.12e6,
                                           12.8e6, 25.6e6, 32e6, 64e6}; // 13 values
  */
  /// Hint: keep fractions of TS size and under 100 us
  std::vector<double_t> vdBinSizesNs = {10, 100, 1.28e3, 25.6e3};  // 4 values, biggest ~ HADES style
  /// Hint: keep fractions of TS size + multiples of bin size and above 10 us
  std::vector<double_t> vdIntegrationNs = {102.4e3, 512e3, 1.28e6, 32e6, 64e6};  // 4 values, prev to last ~ HADES style
  /// Dimension: same as BinSizes vector!!
  std::vector<double_t> vdQfactorHistMax = {2000., 400., 40., 20.};

  std::vector<std::vector<uint32_t>> vuNbBinsHisto(vdIntegrationNs.size(),
                                                   std::vector<uint32_t>(vdBinSizesNs.size(), 0));
  std::vector<uint32_t> vuNbHistoCyclesPerTS(vdIntegrationNs.size(), 0);
  std::vector<std::vector<TH1*>> vHistoOld(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vHistoNew(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vQvalOld(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vQvalNew(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vMeanOld(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vMeanNew(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));

  uint16_t uNbPlots = 0;
  for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
    /// Pre-check values before in spreadsheet to make sure integer !!!!
    vuNbHistoCyclesPerTS[uHistSz] = dTsSizeNs / vdIntegrationNs[uHistSz];

    for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
      /// Pre-check values before in spreadsheet to make sure integer !!!!
      vuNbBinsHisto[uHistSz][uBinSz] = vdIntegrationNs[uHistSz] / vdBinSizesNs[uBinSz];
      if (uMinNbBins <= vuNbBinsHisto[uHistSz][uBinSz] /*&& vuNbBinsHisto[uHistSz][uBinSz] <= uMaxNbBins*/) {
        vHistoOld[uHistSz][uBinSz] =
          new TH1D(Form("binHist_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Counts per %5.0f ns bin in cycle of range %9.0f ns, Old Bmon; Time in Cycle [ns]; Digis []",
                        vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbBinsHisto[uHistSz][uBinSz], 0.0, vdIntegrationNs[uHistSz]);
        vHistoNew[uHistSz][uBinSz] =
          new TH1D(Form("binHist_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Counts per %5.0f ns bin in cycle of range %9.0f ns, New Bmon; Time in Cycle [ns]; Digis []",
                        vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbBinsHisto[uHistSz][uBinSz], 0.0, vdIntegrationNs[uHistSz]);

        double_t dBinOffset = 1.0 / (2.0 * vuNbHistoCyclesPerTS[uHistSz]);
        vQvalOld[uHistSz][uBinSz] =
          new TH1D(Form("evoQFactor_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, Old Bmon; Time in Run [TS]; Q Factor []",
                        uRunId, vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);
        vQvalNew[uHistSz][uBinSz] =
          new TH1D(Form("evoQFactor_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, New Bmon; Time in Run [TS]; Q Factor []",
                        uRunId, vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);

        vMeanOld[uHistSz][uBinSz] =
          new TH1D(Form("evoMean_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, Old Bmon; Time in Run [TS]; Q Factor []",
                        uRunId, vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);
        vMeanNew[uHistSz][uBinSz] =
          new TH1D(Form("evoMean_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, New Bmon; Time in Run [TS]; Q Factor []",
                        uRunId, vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);

        uNbPlots++;
      }
    }
  }

  std::vector<uint32_t> vuIdxHistoCycleinTS(vdIntegrationNs.size(), 0);

  /// Loop on timeslices
  for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
    tree->GetEntry(iEntry);
    uint32_t nDigisBmon = vDigisBmon->size();

    if (nDigisBmon < 300) {
      std::cout << "TS " << iEntry << ", BMON Digis: " << nDigisBmon << " => spill break, skipping this TS"
                << std::endl;
      continue;
    }

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

    vuIdxHistoCycleinTS.assign(vdIntegrationNs.size(), 0);
    for (auto itOld = vDigisOld.begin(); itOld != vDigisOld.end(); ++itOld) {
      double_t dTime = (*itOld).GetTime();
      for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
        uint32_t uCurrentCycle = std::floor(dTime / vdIntegrationNs[uHistSz]);
        if (vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle) {
          for (; vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle; ++vuIdxHistoCycleinTS[uHistSz]) {
            double_t dTsFractional = (vdIntegrationNs[uHistSz] * vuIdxHistoCycleinTS[uHistSz]) / dTsSizeNs + iEntry;
            for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
              if (nullptr != vQvalOld[uHistSz][uBinSz]) {
                double_t dQFactor = ExtractQFactor(vHistoOld[uHistSz][uBinSz]);
                vQvalOld[uHistSz][uBinSz]->Fill(dTsFractional, dQFactor);
                vMeanOld[uHistSz][uBinSz]->Fill(dTsFractional, ExtractMean(vHistoOld[uHistSz][uBinSz]));
                /*
                if (1280 == vdBinSizesNs[uBinSz]) {
                  std::cout << Form("%9f %9.0f %9.0f %9d %9.0f",
                                dTsFractional, vdIntegrationNs[uHistSz], vHistoOld[uHistSz][uBinSz]->Integral(),
                                vHistoOld[uHistSz][uBinSz]->GetNbinsX(),
                                vHistoOld[uHistSz][uBinSz]->GetBinContent(vHistoOld[uHistSz][uBinSz]->GetMaximumBin()))
                    << std::endl;
                }
                */
                if (0.0 < dQFactor) {
                  vHistoOld[uHistSz][uBinSz]->Reset();
                }
              }
            }
          }
        }

        double_t dTimeInCycle = std::fmod(dTime, vdIntegrationNs[uHistSz]);
        for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
          if (nullptr != vQvalOld[uHistSz][uBinSz]) {
            vHistoOld[uHistSz][uBinSz]->Fill(dTimeInCycle);
          }
        }
      }
    }

    vuIdxHistoCycleinTS.assign(vdIntegrationNs.size(), 0);
    uint32_t uDigiIdx = 0;
    for (auto itNew = vDigisNew.begin(); itNew != vDigisNew.end(); ++itNew) {
      double_t dTime = (*itNew).GetTime();
      if (dTime < 0) {
        std::cout << Form("TS %5d Digi %6u %8.0f!!!!!", iEntry, uDigiIdx, dTime) << std::endl;
        continue;
      }
      for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
        uint32_t uCurrentCycle = std::floor(dTime / vdIntegrationNs[uHistSz]);
        if (vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle) {
          for (; vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle; ++vuIdxHistoCycleinTS[uHistSz]) {
            double_t dTsFractional = (vdIntegrationNs[uHistSz] * vuIdxHistoCycleinTS[uHistSz]) / dTsSizeNs + iEntry;
            for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
              if (nullptr != vQvalNew[uHistSz][uBinSz]) {
                double_t dQFactor = ExtractQFactor(vHistoNew[uHistSz][uBinSz]);
                vQvalNew[uHistSz][uBinSz]->Fill(dTsFractional, dQFactor);
                vMeanNew[uHistSz][uBinSz]->Fill(dTsFractional, ExtractMean(vHistoNew[uHistSz][uBinSz]));
                if (0.0 < dQFactor) {
                  vHistoNew[uHistSz][uBinSz]->Reset();
                }
              }
            }
          }
        }

        double_t dTimeInCycle = std::fmod(dTime, vdIntegrationNs[uHistSz]);
        for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
          if (nullptr != vQvalNew[uHistSz][uBinSz]) {
            vHistoNew[uHistSz][uBinSz]->Fill(dTimeInCycle);
          }
        }
      }
      uDigiIdx++;
    }
  }

  uint16_t nPadX = std::ceil(std::sqrt(uNbPlots));
  uint16_t nPadY = std::ceil(1.0 * uNbPlots / nPadX);
  std::cout << Form("Nb plots %3d Nb pads X %2d Y %2d", uNbPlots, nPadX, nPadY) << std::endl;

  TCanvas* test = new TCanvas("test", "test");
  test->Divide(nPadX, nPadY);

  TCanvas* testMean = new TCanvas("testMean", "testMean");
  testMean->Divide(nPadX, nPadY);

  uint32_t uPadIdx = 1;
  for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
    for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
      if (nullptr != vQvalOld[uHistSz][uBinSz]) {
        THStack* pStack = new THStack(Form("pStack_%2d_%2d", uHistSz, uBinSz),
                                      Form("Q-Factor, Old and New Bmon, run %4d, %5.0f ns bin, %9.0f ns range", uRunId,
                                           vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]));
        vQvalOld[uHistSz][uBinSz]->SetLineColor(kBlue);
        vQvalOld[uHistSz][uBinSz]->SetLineWidth(2);
        vQvalOld[uHistSz][uBinSz]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[uBinSz]);
        pStack->Add(vQvalOld[uHistSz][uBinSz]);

        vQvalNew[uHistSz][uBinSz]->SetLineColor(kRed);
        vQvalNew[uHistSz][uBinSz]->SetLineWidth(2);
        vQvalNew[uHistSz][uBinSz]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[uBinSz]);
        pStack->Add(vQvalNew[uHistSz][uBinSz]);

        test->cd(uPadIdx);
        gPad->SetGridx();
        gPad->SetGridy();
        pStack->Draw("hist nostack");
        gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

        THStack* pStackMean =
          new THStack(Form("pStackMean_%2d_%2d", uHistSz, uBinSz),
                      Form("Mean bin content, Old and New Bmon, run %4d, %5.0f ns bin, %9.0f ns range", uRunId,
                           vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]));
        vMeanOld[uHistSz][uBinSz]->SetLineColor(kBlue);
        vMeanOld[uHistSz][uBinSz]->SetLineWidth(2);
        pStackMean->Add(vMeanOld[uHistSz][uBinSz]);

        vMeanNew[uHistSz][uBinSz]->SetLineColor(kRed);
        vMeanNew[uHistSz][uBinSz]->SetLineWidth(2);
        pStackMean->Add(vMeanNew[uHistSz][uBinSz]);

        testMean->cd(uPadIdx);
        gPad->SetGridx();
        gPad->SetGridy();
        pStackMean->Draw("hist nostack");
        gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

        uPadIdx++;
      }
    }
  }

  TFile* outFile = new TFile(Form("data/bmon_q_factor_%04u_%03luTs.root", uRunId, numTimeslices), "RECREATE");
  outFile->cd();
  for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
    for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
      if (nullptr != vQvalOld[uHistSz][uBinSz]) {
        vQvalOld[uHistSz][uBinSz]->Write();
        vQvalNew[uHistSz][uBinSz]->Write();
        vMeanOld[uHistSz][uBinSz]->Write();
        vMeanNew[uHistSz][uBinSz]->Write();
      }
    }
  }
  test->Write();
  testMean->Write();

  gROOT->cd();

  outFile->Close();
}
