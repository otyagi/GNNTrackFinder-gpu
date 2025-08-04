/* Copyright (C) 2019 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

Bool_t BmonCoincPlotSingleRun(UInt_t uRunId)
{
  /// Obtaining the plots
  TH1* tempH1 = NULL;
  TH1* phStsBmon;
  TH1* phMuchBmon;
  TH1* phTofBmon;
  TH1* phRichBmon;

  /// Open File
  TFile* pFile = TFile::Open(Form("HistosTimeCheck_%03u.root", uRunId));
  gROOT->cd();

  if (nullptr == pFile) return kFALSE;

  phStsBmon = new TH1D();
  tempH1    = (TH1*) (pFile->FindObjectAny("fBmonStsDiff"));
  if (NULL != tempH1) {
    tempH1->Copy(*(phStsBmon));  ///?
  }                              //  if( NULL != tempH1 )
  else
    return kFALSE;

  phMuchBmon = new TH1D();
  tempH1     = (TH1*) (pFile->FindObjectAny("fBmonMuchDiff"));
  if (NULL != tempH1) {
    tempH1->Copy(*(phMuchBmon));  ///?
  }                               //  if( NULL != tempH1 )
  else
    return kFALSE;

  phTofBmon = new TH1D();
  tempH1    = (TH1*) (pFile->FindObjectAny("fBmonTofDiff"));
  if (NULL != tempH1) {
    tempH1->Copy(*(phTofBmon));  ///?
  }                              //  if( NULL != tempH1 )
  else
    return kFALSE;

  phRichBmon = new TH1D();
  tempH1     = (TH1*) (pFile->FindObjectAny("fBmonRichDiff"));
  if (NULL != tempH1) {
    tempH1->Copy(*(phRichBmon));  ///?
  }                               //  if( NULL != tempH1 )
  else
    return kFALSE;

  /// Plotting: THStacks with all available detectors VS Bmon
  TCanvas* cBmonCoinc =
    new TCanvas(Form("cBmonCoinc_%03u", uRunId), Form("Time Coincidence with Bmon in run %3u", uRunId));

  THStack* pStacksBmonCoinc =
    new THStack(Form("stackBmonCoinc_%02u", uRunId),
                Form("Time Coincidence with Bmon in run %02u; tBmon - Tdet [ns]; Pairs []", uRunId));
  TLegend* legend = new TLegend(0.1, 0.7, 0.3, 0.9);

  phStsBmon->SetLineColor(kBlack);
  phMuchBmon->SetLineColor(kRed);
  phTofBmon->SetLineColor(kBlue);
  phRichBmon->SetLineColor(kViolet);

  phStsBmon->SetLineWidth(2);
  phMuchBmon->SetLineWidth(2);
  phTofBmon->SetLineWidth(2);
  phRichBmon->SetLineWidth(2);

  pStacksBmonCoinc->Add(phStsBmon);
  pStacksBmonCoinc->Add(phMuchBmon);
  pStacksBmonCoinc->Add(phTofBmon);
  pStacksBmonCoinc->Add(phRichBmon);

  legend->AddEntry(phStsBmon, "STS", "l");
  legend->AddEntry(phMuchBmon, "MUCH", "l");
  legend->AddEntry(phTofBmon, "TOF", "l");
  legend->AddEntry(phRichBmon, "RICH", "l");

  cBmonCoinc->cd();
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogy();
  pStacksBmonCoinc->Draw("nostack,hist");
  legend->Draw();
  pStacksBmonCoinc->GetXaxis()->SetRangeUser(-2000.0, 2000.0);

  /// Plotting: rescaled to baseline plots with all available detectors VS Bmon
  /// => TODO: fit baseline out of sync peak
  Double_t dBaselineSts;
  Double_t dBaselineMuch;
  Double_t dBaselineTof;
  Double_t dBaselineRich;

  /// TEMP solution: read bin at -5000 ns
  dBaselineSts  = phStsBmon->GetBinContent(phStsBmon->FindBin(-5000));
  dBaselineMuch = phMuchBmon->GetBinContent(phMuchBmon->FindBin(-5000));
  dBaselineTof  = phTofBmon->GetBinContent(phTofBmon->FindBin(-5000));
  dBaselineRich = phRichBmon->GetBinContent(phRichBmon->FindBin(-5000));

  TH1* phRescaledStsBmon  = static_cast<TH1*>(phStsBmon->Clone("hRescaledStsBmon"));
  TH1* phRescaledMuchBmon = static_cast<TH1*>(phMuchBmon->Clone("hRescaledMuchBmon"));
  TH1* phRescaledTofBmon  = static_cast<TH1*>(phTofBmon->Clone("hRescaledTofBmon"));
  TH1* phRescaledRichBmon = static_cast<TH1*>(phRichBmon->Clone("hRescaledRichBmon"));

  phRescaledStsBmon->Scale(1 / dBaselineSts);
  phRescaledMuchBmon->Scale(1 / dBaselineMuch);
  phRescaledTofBmon->Scale(1 / dBaselineTof);
  phRescaledRichBmon->Scale(1 / dBaselineRich);

  TCanvas* cBmonCoincRescale =
    new TCanvas(Form("cBmonCoincRescale_%03u", uRunId), Form("Time Coincidence with Bmon in run %3u", uRunId));

  THStack* pStacksBmonCoincRescale = new THStack(Form("stackBmonCoincRescale_%02u", uRunId),
                                                 Form("Time Coincidence with Bmon in run %02u; tBmon - Tdet [ns]; "
                                                      "Pairs nb rescaled [1/baseline]",
                                                      uRunId));
  TLegend* legendRescale           = new TLegend(0.1, 0.7, 0.3, 0.9);

  phRescaledStsBmon->SetLineColor(kBlack);
  phRescaledMuchBmon->SetLineColor(kRed);
  phRescaledTofBmon->SetLineColor(kBlue);
  phRescaledRichBmon->SetLineColor(kViolet);

  phRescaledStsBmon->SetLineWidth(2);
  phRescaledMuchBmon->SetLineWidth(2);
  phRescaledTofBmon->SetLineWidth(2);
  phRescaledRichBmon->SetLineWidth(2);

  pStacksBmonCoincRescale->Add(phRescaledStsBmon);
  pStacksBmonCoincRescale->Add(phRescaledMuchBmon);
  pStacksBmonCoincRescale->Add(phRescaledTofBmon);
  pStacksBmonCoincRescale->Add(phRescaledRichBmon);

  legendRescale->AddEntry(phRescaledStsBmon, "STS", "l");
  legendRescale->AddEntry(phRescaledMuchBmon, "MUCH", "l");
  legendRescale->AddEntry(phRescaledTofBmon, "TOF", "l");
  legendRescale->AddEntry(phRescaledRichBmon, "RICH", "l");

  cBmonCoincRescale->cd();
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogy();
  pStacksBmonCoincRescale->Draw("nostack,hist");
  legendRescale->Draw();
  pStacksBmonCoincRescale->GetXaxis()->SetRangeUser(-2000.0, 2000.0);

  return kTRUE;
}
