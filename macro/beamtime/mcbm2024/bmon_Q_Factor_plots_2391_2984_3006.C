/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


void bmon_Q_Factor_plots_2391_2984_3006()
{
  std::vector<uint32_t> vRunId = {2391, 2894, 3006};
  // std::vector<uint32_t> vRunId = {3006, 3107, 3109};

  /// Hint: keep fractions of TS size and under 100 us
  std::vector<double_t> vdBinSizesNs = {10, 100, 1.28e3, 25.6e3};  // 4 values, biggest ~ HADES style
  /// Hint: keep fractions of TS size + multiples of bin size and above 10 us
  std::vector<double_t> vdIntegrationNs = {102.4e3, 512e3, 1.28e6, 32e6, 64e6};  // 4 values, prev to last ~ HADES style
  /// Dimension: same as BinSizes vector!!
  std::vector<double_t> vdQfactorHistMax = {2000., 400., 40., 20.};

  std::map<uint32_t, std::vector<std::vector<TH1*>>> vQvalOld;
  std::map<uint32_t, std::vector<std::vector<TH1*>>> vQvalNew;
  std::map<uint32_t, std::vector<std::vector<TH1*>>> vMeanOld;
  std::map<uint32_t, std::vector<std::vector<TH1*>>> vMeanNew;

  for (uint32_t uRun = 0; uRun < vRunId.size(); ++uRun) {
    TFile* inFile = new TFile(Form("data/bmon_q_factor_%04u_080Ts.root", vRunId[uRun]), "READ");
    gROOT->cd();
    std::cout << vRunId[uRun] << std::endl;

    vQvalOld[uRun].resize(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
    vQvalNew[uRun].resize(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
    vMeanOld[uRun].resize(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
    vMeanNew[uRun].resize(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));

    for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
      for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
        TH1* pTemp = dynamic_cast<TH1*>(
          inFile->FindObjectAny(Form("evoQFactor_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        if (nullptr != pTemp) {
          pTemp = dynamic_cast<TH1*>(pTemp->Clone(
            Form("EvoQFactor_Old_%04d_%9.0f_%5.0f", vRunId[uRun], vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        }
        vQvalOld[uRun][uHistSz][uBinSz] = pTemp;

        pTemp = dynamic_cast<TH1*>(
          inFile->FindObjectAny(Form("evoQFactor_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        if (nullptr != pTemp) {
          pTemp = dynamic_cast<TH1*>(pTemp->Clone(
            Form("EvoQFactor_New_%04d_%9.0f_%5.0f", vRunId[uRun], vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        }
        vQvalNew[uRun][uHistSz][uBinSz] = pTemp;

        pTemp = dynamic_cast<TH1*>(
          inFile->FindObjectAny(Form("evoMean_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        if (nullptr != pTemp) {
          pTemp = dynamic_cast<TH1*>(pTemp->Clone(
            Form("EvoMean_Old_%04d_%9.0f_%5.0f", vRunId[uRun], vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        }
        vMeanOld[uRun][uHistSz][uBinSz] = pTemp;

        pTemp = dynamic_cast<TH1*>(
          inFile->FindObjectAny(Form("evoMean_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        if (nullptr != pTemp) {
          pTemp = dynamic_cast<TH1*>(pTemp->Clone(
            Form("EvoMean_New_%04d_%9.0f_%5.0f", vRunId[uRun], vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz])));
        }
        vMeanNew[uRun][uHistSz][uBinSz] = pTemp;
      }
    }
    inFile->Close();
  }

  /// HADES ---------------------------------------------------------------------------------------------------------///
  uint32_t uHadesHistSz  = 3;
  uint32_t uHadesBinSz   = 3;
  TCanvas* cHadesQfactor = new TCanvas("cHadesQfactor", Form("HADES-like Q factors: %5.0f ns bin, %9.0f ns range",
                                                             vdBinSizesNs[uHadesBinSz], vdIntegrationNs[uHadesHistSz]));
  cHadesQfactor->Divide(vRunId.size(), 2);

  std::vector<THStack*> vStacksHadesQfactor(2 * vRunId.size(), nullptr);
  for (uint32_t uRun = 0; uRun < vRunId.size(); ++uRun) {

    vStacksHadesQfactor[uRun] =
      new THStack(Form("stackHades_Qfact_%04d", vRunId[uRun]), Form("Q-Factor, run %4d", vRunId[uRun]));
    if (nullptr != vQvalOld[uRun][uHadesHistSz][uHadesBinSz]) {
      vQvalOld[uRun][uHadesHistSz][uHadesBinSz]->SetLineColor(kBlue);
      vQvalOld[uRun][uHadesHistSz][uHadesBinSz]->SetLineWidth(2);
      vQvalOld[uRun][uHadesHistSz][uHadesBinSz]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[uHadesBinSz]);
      vStacksHadesQfactor[uRun]->Add(vQvalOld[uRun][uHadesHistSz][uHadesBinSz]);
    }

    if (nullptr != vQvalNew[uRun][uHadesHistSz][uHadesBinSz]) {
      vQvalNew[uRun][uHadesHistSz][uHadesBinSz]->SetLineColor(kRed);
      vQvalNew[uRun][uHadesHistSz][uHadesBinSz]->SetLineWidth(2);
      vQvalNew[uRun][uHadesHistSz][uHadesBinSz]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[uHadesBinSz]);
      vStacksHadesQfactor[uRun]->Add(vQvalNew[uRun][uHadesHistSz][uHadesBinSz]);
    }

    cHadesQfactor->cd(1 + uRun);
    gPad->SetGridx();
    gPad->SetGridy();
    vStacksHadesQfactor[uRun]->Draw("hist nostack");
    gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

    vStacksHadesQfactor[vRunId.size() + uRun] =
      new THStack(Form("stackHades_Mean_%04d", vRunId[uRun]), Form("Mean bin content, run %4d", vRunId[uRun]));
    if (nullptr != vMeanOld[uRun][uHadesHistSz][uHadesBinSz]) {
      vMeanOld[uRun][uHadesHistSz][uHadesBinSz]->SetLineColor(kBlue);
      vMeanOld[uRun][uHadesHistSz][uHadesBinSz]->SetLineWidth(2);
      vStacksHadesQfactor[vRunId.size() + uRun]->Add(vMeanOld[uRun][uHadesHistSz][uHadesBinSz]);
    }

    if (nullptr != vMeanNew[uRun][uHadesHistSz][uHadesBinSz]) {
      vMeanNew[uRun][uHadesHistSz][uHadesBinSz]->SetLineColor(kRed);
      vMeanNew[uRun][uHadesHistSz][uHadesBinSz]->SetLineWidth(2);
      vStacksHadesQfactor[vRunId.size() + uRun]->Add(vMeanNew[uRun][uHadesHistSz][uHadesBinSz]);
    }

    cHadesQfactor->cd(1 + vRunId.size() + uRun);
    gPad->SetGridx();
    gPad->SetGridy();
    vStacksHadesQfactor[vRunId.size() + uRun]->Draw("hist nostack");
    gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");
  }
  ///----------------------------------------------------------------------------------------------------------------///

  /// Timescale -----------------------------------------------------------------------------------------------------///
  uint32_t uTimescaleHistSz = 4;
  TCanvas* cTimescaleQfactor =
    new TCanvas("cTimescaleQfactor", Form("Q factors vs timescale: %9.0f ns range", vdIntegrationNs[uTimescaleHistSz]));
  cTimescaleQfactor->Divide(vdBinSizesNs.size(), vRunId.size());
  TCanvas* cTimescaleMean = new TCanvas(
    "cTimescaleMean", Form("Mean bin content vs timescale: %9.0f ns range", vdIntegrationNs[uTimescaleHistSz]));
  cTimescaleMean->Divide(vdBinSizesNs.size(), vRunId.size());

  std::vector<std::vector<THStack*>> vStacksTimescaleQfactor(vRunId.size(),
                                                             std::vector<THStack*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<THStack*>> vStacksTimescaleMean(vRunId.size(),
                                                          std::vector<THStack*>(vdBinSizesNs.size(), nullptr));
  for (uint32_t uRun = 0; uRun < vRunId.size(); ++uRun) {
    for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
      vStacksTimescaleQfactor[uRun][uBinSz] =
        new THStack(Form("stackBinSz_Qfact_%04d_%02d", vRunId[uRun], uBinSz),
                    Form("Q-Factor, run %4d, %5.0f ns bin", vRunId[uRun], vdBinSizesNs[uBinSz]));
      if (nullptr != vQvalOld[uRun][uTimescaleHistSz][uBinSz]) {
        vQvalOld[uRun][uTimescaleHistSz][uBinSz]->SetLineColor(kBlue);
        vQvalOld[uRun][uTimescaleHistSz][uBinSz]->SetLineWidth(2);
        vQvalOld[uRun][uTimescaleHistSz][uBinSz]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[uBinSz]);
        vStacksTimescaleQfactor[uRun][uBinSz]->Add(vQvalOld[uRun][uTimescaleHistSz][uBinSz]);
      }

      if (nullptr != vQvalNew[uRun][uTimescaleHistSz][uBinSz]) {
        vQvalNew[uRun][uTimescaleHistSz][uBinSz]->SetLineColor(kRed);
        vQvalNew[uRun][uTimescaleHistSz][uBinSz]->SetLineWidth(2);
        vQvalNew[uRun][uTimescaleHistSz][uBinSz]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[uBinSz]);
        vStacksTimescaleQfactor[uRun][uBinSz]->Add(vQvalNew[uRun][uTimescaleHistSz][uBinSz]);
      }

      cTimescaleQfactor->cd(1 + uRun * vdBinSizesNs.size() + uBinSz);
      gPad->SetGridx();
      gPad->SetGridy();
      vStacksTimescaleQfactor[uRun][uBinSz]->Draw("hist nostack");
      gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

      vStacksTimescaleMean[uRun][uBinSz] =
        new THStack(Form("stackBinSz_Mean_%04d_%02d", vRunId[uRun], uBinSz),
                    Form("Mean bin content, run %4d, %5.0f ns bin", vRunId[uRun], vdBinSizesNs[uBinSz]));
      if (nullptr != vMeanOld[uRun][uTimescaleHistSz][uBinSz]) {
        vMeanOld[uRun][uTimescaleHistSz][uBinSz]->SetLineColor(kBlue);
        vMeanOld[uRun][uTimescaleHistSz][uBinSz]->SetLineWidth(2);
        vStacksTimescaleMean[uRun][uBinSz]->Add(vMeanOld[uRun][uTimescaleHistSz][uBinSz]);
      }

      if (nullptr != vMeanNew[uRun][uTimescaleHistSz][uBinSz]) {
        vMeanNew[uRun][uTimescaleHistSz][uBinSz]->SetLineColor(kRed);
        vMeanNew[uRun][uTimescaleHistSz][uBinSz]->SetLineWidth(2);
        vStacksTimescaleMean[uRun][uBinSz]->Add(vMeanNew[uRun][uTimescaleHistSz][uBinSz]);
      }

      cTimescaleMean->cd(1 + uRun * vdBinSizesNs.size() + uBinSz);
      gPad->SetGridx();
      gPad->SetGridy();
      vStacksTimescaleMean[uRun][uBinSz]->Draw("hist nostack");
      gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");
    }
  }
  ///----------------------------------------------------------------------------------------------------------------///

  /// Integration ---------------------------------------------------------------------------------------------------///
  uint32_t uIntegrationHistSzSmall = 1;
  uint32_t uIntegrationHistSzBig   = 3;
  TCanvas* cIntegrationQfactor =
    new TCanvas("cIntegrationQfactor", "Q factors vs Integration length, min/max timescales");
  cIntegrationQfactor->Divide(4, vRunId.size());

  std::vector<std::vector<THStack*>> vStacksIntegrationQfactor(vRunId.size(), std::vector<THStack*>(4, nullptr));
  for (uint32_t uRun = 0; uRun < vRunId.size(); ++uRun) {
    /// Small scale, small integration
    vStacksIntegrationQfactor[uRun][0] =
      new THStack(Form("stackIntegr_Qfact_%04d_%02d", vRunId[uRun], 0),
                  Form("Q-Factor, run %4d, %5.0f ns bin, %9.0f ns range", vRunId[uRun], vdBinSizesNs[0],
                       vdIntegrationNs[uIntegrationHistSzSmall]));
    if (nullptr != vQvalOld[uRun][uIntegrationHistSzSmall][0]) {
      vQvalOld[uRun][uIntegrationHistSzSmall][0]->SetLineColor(kBlue);
      vQvalOld[uRun][uIntegrationHistSzSmall][0]->SetLineWidth(2);
      vQvalOld[uRun][uIntegrationHistSzSmall][0]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[0]);
      vStacksIntegrationQfactor[uRun][0]->Add(vQvalOld[uRun][uIntegrationHistSzSmall][0]);
    }

    if (nullptr != vQvalNew[uRun][uIntegrationHistSzSmall][0]) {
      vQvalNew[uRun][uIntegrationHistSzSmall][0]->SetLineColor(kRed);
      vQvalNew[uRun][uIntegrationHistSzSmall][0]->SetLineWidth(2);
      vQvalNew[uRun][uIntegrationHistSzSmall][0]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[0]);
      vStacksIntegrationQfactor[uRun][0]->Add(vQvalNew[uRun][uIntegrationHistSzSmall][0]);
    }

    cIntegrationQfactor->cd(1 + uRun * 4);
    gPad->SetGridx();
    gPad->SetGridy();
    vStacksIntegrationQfactor[uRun][0]->Draw("hist nostack");
    gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

    /// Large scale, small integration
    vStacksIntegrationQfactor[uRun][1] =
      new THStack(Form("stackIntegr_Qfact_%04d_%02d", vRunId[uRun], 1),
                  Form("Q-Factor, run %4d, %5.0f ns bin, %9.0f ns range", vRunId[uRun],
                       vdBinSizesNs[vdBinSizesNs.size() - 1], vdIntegrationNs[uIntegrationHistSzSmall]));
    if (nullptr != vQvalOld[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]) {
      vQvalOld[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]->SetLineColor(kBlue);
      vQvalOld[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]->SetLineWidth(2);
      vQvalOld[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]->GetYaxis()->SetRangeUser(
        0., vdQfactorHistMax[vdBinSizesNs.size() - 1]);
      vStacksIntegrationQfactor[uRun][1]->Add(vQvalOld[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]);
    }

    if (nullptr != vQvalNew[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]) {
      vQvalNew[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]->SetLineColor(kRed);
      vQvalNew[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]->SetLineWidth(2);
      vQvalNew[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]->GetYaxis()->SetRangeUser(
        0., vdQfactorHistMax[vdBinSizesNs.size() - 1]);
      vStacksIntegrationQfactor[uRun][1]->Add(vQvalNew[uRun][uIntegrationHistSzSmall][vdBinSizesNs.size() - 1]);
    }

    cIntegrationQfactor->cd(2 + uRun * 4);
    gPad->SetGridx();
    gPad->SetGridy();
    vStacksIntegrationQfactor[uRun][1]->Draw("hist nostack");
    gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

    /// Small scale, large integration
    vStacksIntegrationQfactor[uRun][2] =
      new THStack(Form("stackIntegr_Qfact_%04d_%02d", vRunId[uRun], 2),
                  Form("Q-Factor, run %4d, %5.0f ns bin, %9.0f ns range", vRunId[uRun], vdBinSizesNs[0],
                       vdIntegrationNs[uIntegrationHistSzBig]));
    if (nullptr != vQvalOld[uRun][uIntegrationHistSzBig][0]) {
      vQvalOld[uRun][uIntegrationHistSzBig][0]->SetLineColor(kBlue);
      vQvalOld[uRun][uIntegrationHistSzBig][0]->SetLineWidth(2);
      vQvalOld[uRun][uIntegrationHistSzBig][0]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[0]);
      vStacksIntegrationQfactor[uRun][2]->Add(vQvalOld[uRun][uIntegrationHistSzBig][0]);
    }

    if (nullptr != vQvalNew[uRun][uIntegrationHistSzBig][0]) {
      vQvalNew[uRun][uIntegrationHistSzBig][0]->SetLineColor(kRed);
      vQvalNew[uRun][uIntegrationHistSzBig][0]->SetLineWidth(2);
      vQvalNew[uRun][uIntegrationHistSzBig][0]->GetYaxis()->SetRangeUser(0., vdQfactorHistMax[0]);
      vStacksIntegrationQfactor[uRun][2]->Add(vQvalNew[uRun][uIntegrationHistSzBig][0]);
    }

    cIntegrationQfactor->cd(3 + uRun * 4);
    gPad->SetGridx();
    gPad->SetGridy();
    vStacksIntegrationQfactor[uRun][2]->Draw("hist nostack");
    gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");

    /// Large scale, large integration
    vStacksIntegrationQfactor[uRun][3] =
      new THStack(Form("stackIntegr_Qfact_%04d_%02d", vRunId[uRun], 3),
                  Form("Q-Factor, run %4d, %5.0f ns bin, %9.0f ns range", vRunId[uRun],
                       vdBinSizesNs[vdBinSizesNs.size() - 1], vdIntegrationNs[uIntegrationHistSzBig]));
    if (nullptr != vQvalOld[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]) {
      vQvalOld[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]->SetLineColor(kBlue);
      vQvalOld[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]->SetLineWidth(2);
      vQvalOld[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]->GetYaxis()->SetRangeUser(
        0., vdQfactorHistMax[vdBinSizesNs.size() - 1]);
      vStacksIntegrationQfactor[uRun][3]->Add(vQvalOld[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]);
    }

    if (nullptr != vQvalNew[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]) {
      vQvalNew[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]->SetLineColor(kRed);
      vQvalNew[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]->SetLineWidth(2);
      vQvalNew[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]->GetYaxis()->SetRangeUser(
        0., vdQfactorHistMax[vdBinSizesNs.size() - 1]);
      vStacksIntegrationQfactor[uRun][3]->Add(vQvalNew[uRun][uIntegrationHistSzBig][vdBinSizesNs.size() - 1]);
    }

    cIntegrationQfactor->cd(4 + uRun * 4);
    gPad->SetGridx();
    gPad->SetGridy();
    vStacksIntegrationQfactor[uRun][3]->Draw("hist nostack");
    gPad->BuildLegend(0.79, 0.79, 0.99, 0.94, "");
  }
  ///----------------------------------------------------------------------------------------------------------------///
}
