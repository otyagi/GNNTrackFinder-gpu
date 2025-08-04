/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


void bmon_Q_Factor_timescale_plots()
{
  std::vector<uint32_t> vRunId       = {2391, 2984, 3006, 3107, 3109};
  std::map<uint32_t, EColor> mColors = {{2391, EColor(kBlue)},
                                        {2984, EColor(kRed)},
                                        {3006, EColor(kGreen + 2)},
                                        {3107, EColor(kViolet)},
                                        {3109, EColor(kBlack)}};
  uint32_t uRefRun                   = 2391;

  std::map<uint32_t, TH1*> vMeanOld;
  std::map<uint32_t, TH1*> vMeanNew;
  std::map<uint32_t, TH1*> vQvalOld;
  std::map<uint32_t, TH1*> vQvalNew;
  std::map<uint32_t, TH1*> vMeanOldVsRef;
  std::map<uint32_t, TH1*> vQvalOldVsRef;

  std::map<uint32_t, TF1*> vMeanFitOld;
  std::map<uint32_t, TF1*> vMeanFitNew;

  for (auto runIdx : vRunId) {
    TFile* inFile = new TFile(Form("data/bmon_q_factor_timescale_%04u_Spill.root", runIdx), "READ");
    gROOT->cd();
    std::cout << runIdx << std::endl;

    TH1* pTemp = dynamic_cast<TH1*>(inFile->FindObjectAny(Form("MeanVsBinSzOld_%04u", runIdx)));
    if (nullptr != pTemp) {
      pTemp = dynamic_cast<TH1*>(pTemp->Clone(Form("MeanVsBinSzOld_%04d_", runIdx)));
    }
    vMeanOld[runIdx] = pTemp;

    pTemp = dynamic_cast<TH1*>(inFile->FindObjectAny(Form("MeanVsBinSzNew_%04u", runIdx)));
    if (nullptr != pTemp) {
      pTemp = dynamic_cast<TH1*>(pTemp->Clone(Form("MeanVsBinSzNew_%04d_", runIdx)));
    }
    vMeanNew[runIdx] = pTemp;

    pTemp = dynamic_cast<TH1*>(inFile->FindObjectAny(Form("hQfactorVsBinSzOld_%04u", runIdx)));
    if (nullptr != pTemp) {
      pTemp = dynamic_cast<TH1*>(pTemp->Clone(Form("QfactorVsBinSzOld_%04d", runIdx)));
    }
    vQvalOld[runIdx] = pTemp;

    pTemp = dynamic_cast<TH1*>(inFile->FindObjectAny(Form("hQfactorVsBinSzNew_%04u", runIdx)));
    if (nullptr != pTemp) {
      pTemp = dynamic_cast<TH1*>(pTemp->Clone(Form("QfactorVsBinSzOld_%04d", runIdx)));
    }
    vQvalNew[runIdx] = pTemp;

    inFile->Close();
  }

  if (vMeanOld[uRefRun] && vQvalOld[uRefRun]) {
    for (auto runIdx : vRunId) {
      if (nullptr != vMeanOld[runIdx]) {
        vMeanOldVsRef[runIdx] = dynamic_cast<TH1*>(vMeanOld[runIdx]->Clone(Form("MeanOldVsRef_%04d_", runIdx)));
        vMeanOldVsRef[runIdx]->Divide(vMeanOld[uRefRun]);

        vMeanFitOld[runIdx] = new TF1(Form("fMeanFitOld_%04u", runIdx), "pol1", 0, 1e9);
        TCanvas temp("tempcanv", "temp");
        temp.cd();
        vMeanOld[runIdx]->Draw();
        vMeanOld[runIdx]->Fit(vMeanFitOld[runIdx]);
      }
      if (nullptr != vQvalOld[runIdx]) {
        vQvalOldVsRef[runIdx] = dynamic_cast<TH1*>(vQvalOld[runIdx]->Clone(Form("QvalOldVsRef_%04d_", runIdx)));
        vQvalOldVsRef[runIdx]->Divide(vQvalOld[uRefRun]);
      }
      if (nullptr != vMeanNew[runIdx]) {
        vMeanFitNew[runIdx] = new TF1(Form("fMeanFitNew_%04u", runIdx), "pol1", 0, 1e9);
        TCanvas temp("tempcanv", "temp");
        temp.cd();
        vMeanNew[runIdx]->Draw();
        vMeanNew[runIdx]->Fit(vMeanFitNew[runIdx]);
      }
    }
  }

  /// Timescale -----------------------------------------------------------------------------------------------------///
  uint32_t uTimescaleHistSz  = 4;
  TCanvas* cTimescaleQfactor = new TCanvas("cTimescaleQfactor", "Mean & Q-factors vs timescale: 2.56 ms range");
  cTimescaleQfactor->Divide(2, 2);

  THStack* stackTimescaleMeanOld =
    new THStack("stackBinSz_Mean_Old", "Mean bin content, Old BMon; Bin size [ns]; Mean bin content [digis]");
  THStack* stackTimescaleMeanNew =
    new THStack("stackBinSz_Mean_New", "Mean bin content, New BMon; Bin size [ns]; Mean bin content [digis]");
  THStack* stackTimescaleQfactorOld =
    new THStack("stackBinSz_Qfact_Old", "Q-Factor, Old BMon; Bin size [ns]; Q-Factor []");
  THStack* stackTimescaleQfactorNew =
    new THStack("stackBinSz_Qfact_New", "Q-Factor, New BMon; Bin size [ns]; Q-Factor []");

  for (auto runIdx : vRunId) {
    if (nullptr != vMeanOld[runIdx]) {
      vMeanOld[runIdx]->SetLineColor(mColors[runIdx]);
      vMeanOld[runIdx]->SetLineWidth(2);
      stackTimescaleMeanOld->Add(vMeanOld[runIdx]);
    }

    if (nullptr != vMeanNew[runIdx]) {
      vMeanNew[runIdx]->SetLineColor(mColors[runIdx]);
      vMeanNew[runIdx]->SetLineWidth(2);
      stackTimescaleMeanNew->Add(vMeanNew[runIdx]);
    }

    if (nullptr != vQvalOld[runIdx]) {
      vQvalOld[runIdx]->SetLineColor(mColors[runIdx]);
      vQvalOld[runIdx]->SetLineWidth(2);
      stackTimescaleQfactorOld->Add(vQvalOld[runIdx]);
    }

    if (nullptr != vQvalNew[runIdx]) {
      vQvalNew[runIdx]->SetLineColor(mColors[runIdx]);
      vQvalNew[runIdx]->SetLineWidth(2);
      stackTimescaleQfactorNew->Add(vQvalNew[runIdx]);
    }
  }

  cTimescaleQfactor->cd(1);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  stackTimescaleMeanOld->Draw("hist nostack");
  gPad->BuildLegend(0.15, 0.70, 0.45, 0.90, "");

  cTimescaleQfactor->cd(2);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  stackTimescaleMeanNew->Draw("hist nostack");
  gPad->BuildLegend(0.15, 0.70, 0.45, 0.90, "");

  cTimescaleQfactor->cd(3);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  stackTimescaleQfactorOld->Draw("hist nostack");
  gPad->BuildLegend(0.69, 0.70, 0.99, 0.90, "");

  cTimescaleQfactor->cd(4);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  stackTimescaleQfactorNew->Draw("hist nostack");
  gPad->BuildLegend(0.69, 0.70, 0.99, 0.90, "");
  ///----------------------------------------------------------------------------------------------------------------///

  /// Reference -----------------------------------------------------------------------------------------------------///
  TCanvas* cTimescaleRef = new TCanvas("cTimescaleRef", "Mean & Q-factors vs timescale: 2.56 ms range");
  cTimescaleRef->Divide(2);

  THStack* stackTimescaleRefMeanOld = new THStack(
    "stackBinSzRef_Mean_Old",
    Form("Mean bin content as fraction of run %04u, Old BMon; Bin size [ns]; Mean / Mean_%04u []", uRefRun, uRefRun));
  THStack* stackTimescaleRefQfactorOld = new THStack(
    "stackBinSzRef_Qfact_Old",
    Form("Q-Factor as fraction of run %04u, Old BMon; Bin size [ns]; Q-Factor / Q-Factor)%04u []", uRefRun, uRefRun));

  for (auto runIdx : vRunId) {
    if (nullptr != vMeanOldVsRef[runIdx]) {
      vMeanOldVsRef[runIdx]->SetLineColor(mColors[runIdx]);
      vMeanOldVsRef[runIdx]->SetLineWidth(2);
      stackTimescaleRefMeanOld->Add(vMeanOldVsRef[runIdx]);
    }

    if (nullptr != vQvalOldVsRef[runIdx]) {
      vQvalOldVsRef[runIdx]->SetLineColor(mColors[runIdx]);
      vQvalOldVsRef[runIdx]->SetLineWidth(2);
      stackTimescaleRefQfactorOld->Add(vQvalOldVsRef[runIdx]);
    }
  }

  cTimescaleRef->cd(1);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  stackTimescaleRefMeanOld->Draw("hist nostack");
  gPad->BuildLegend(0.25, 0.20, 0.75, 0.40, "");

  cTimescaleRef->cd(2);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  stackTimescaleRefQfactorOld->Draw("hist nostack");
  gPad->BuildLegend(0.49, 0.60, 0.99, 0.90, "");
  ///----------------------------------------------------------------------------------------------------------------///

  /// Check of beam particles estimated counts ----------------------------------------------------------------------///
  std::cout << "Fitted average flux per seconds: " << std::endl;
  for (auto runIdx : vRunId) {
    std::cout << Form("%04d => %9e Digi/s (Old) %9e Digi/s (New)", runIdx, vMeanFitOld[runIdx]->Eval(1e9),
                      vMeanFitNew[runIdx]->Eval(1e9))
              << std::endl;
  }
  std::cout << "Fitted average count per spill: " << std::endl;
  for (auto runIdx : vRunId) {
    double_t dSpillDurationNs = 9.6e9;  // ns
    if (2391 == runIdx) {
      dSpillDurationNs = 7.8e9;  // ns
    }
    std::cout << Form("%04d => %9e Digis (Old) %9e Digis (New)", runIdx, vMeanFitOld[runIdx]->Eval(dSpillDurationNs),
                      vMeanFitNew[runIdx]->Eval(dSpillDurationNs))
              << std::endl;
  }
  /*
  ===============================================================
  Run 2391 [38, 98] (61 TS)
  Beam particles in spill: 35459706 (Old) VS 0 (New)
  Avg Beam particles/s: 4.54146e+06 (Old) VS 0 (New)
  ===============================================================
  Run 2984 [66, 140] (75 TS, 141 is mostly spill ramp down)
  Beam particles in spill: 27967704 (Old) VS 21882062 (New)
  Avg Beam particles/s: 2.9133e+06 (Old) VS 2.27938e+06 (New)
  ===============================================================
  Run 3006 [76, 148] (73 TS, 149 is half empty, maybe sharp spill edge)
  Beam particles in spill: 18387484 (Old) VS 29189274 (New)
  Avg Beam particles/s: 1.96784e+06 (Old) VS 3.12385e+06 (New)
  ===============================================================
  Run 3107 [53, 128] (76 TS)
  Beam particles in spill: 44813487 (Old) VS 50719284 (New)
  Avg Beam particles/s: 4.60665e+06 (Old) VS 5.21374e+06 (New)
  ===============================================================
  Run 3109 [14, 88] (75 TS)
  Beam particles in spill: 20582262 (Old) VS 24777405 (New)
  Avg Beam particles/s: 2.14399e+06 (Old) VS 2.58098e+06 (New)
  ===============================================================
  Fitted average flux per seconds:
  2391 => 2.102520e+06 Digi/s (Old) 0.000000e+00 Digi/s (New)
  2984 => 1.212869e+06 Digi/s (Old) 1.066250e+06 Digi/s (New)
  3006 => 1.188376e+06 Digi/s (Old) 2.202738e+06 Digi/s (New)
  3107 => 1.761680e+06 Digi/s (Old) 2.133611e+06 Digi/s (New)
  3109 => 1.113414e+06 Digi/s (Old) 1.419566e+06 Digi/s (New)
  Fitted average count per spill:
  2391 => 1.639965e+07 Digis (Old) 0.000000e+00 Digis (New)
  2984 => 1.164355e+07 Digis (Old) 1.023600e+07 Digis (New)
  3006 => 1.140841e+07 Digis (Old) 2.114629e+07 Digis (New)
  3107 => 1.691213e+07 Digis (Old) 2.048267e+07 Digis (New)
  3109 => 1.068877e+07 Digis (Old) 1.362784e+07 Digis (New)
   */

  ///----------------------------------------------------------------------------------------------------------------///
}
