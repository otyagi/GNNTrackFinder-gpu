/* Copyright (C) 2013-2016 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

/**
 * \file CbmLitTofQaReport.cxx
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2013
 */
#include "CbmLitTofQaReport.h"

#include "CbmDrawHist.h"
#include "CbmHistManager.h"
#include "CbmReportElement.h"
#include "CbmUtils.h"
#include "TCanvas.h"
#include "TDirectory.h"
#include "TF1.h"
#include "TH1.h"
#include "TLatex.h"
#include "TMarker.h"
#include "TPad.h"

#include <boost/assign/list_of.hpp>

#include <iostream>
#include <map>

using boost::assign::list_of;
using Cbm::NumberToString;
using Cbm::Split;
using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;

CbmLitTofQaReport::CbmLitTofQaReport() : CbmSimulationReport() { SetReportName("tof_qa"); }

CbmLitTofQaReport::~CbmLitTofQaReport() {}

void CbmLitTofQaReport::Create()
{
  Out().precision(3);
  Out() << R()->DocumentBegin();
  Out() << R()->Title(0, GetTitle());

  PrintCanvases();

  Out() << R()->DocumentEnd();
}

void CbmLitTofQaReport::Draw()
{
  HM()->NormalizeToIntegralByPattern("hmp_TofTrack_All_.+");

  DrawH2ByPattern("hmp_Tof_Reco_.+_m2p", kLinear, kLinear, kLog, "colz");
  DrawH2ByPattern("hmp_Tof_RecoMCID_.+_m2p", kLinear, kLinear, kLog, "colz");
  DrawH2ByPattern("hmp_Tof_RecoAccTof_.+_m2p", kLinear, kLinear, kLog, "colz");
  DrawH2ByPattern("hmp_Tof_RecoMCIDAccTof_.+_m2p", kLinear, kLinear, kLog, "colz");
  DrawH1ByPattern("hmp_Tof_TimeZero_.+");
  DrawH1ByPattern("hmp_Tof_dTime");
  DrawH1ByPattern("hmp_Tof_Time_FirstTrack");

  DrawH1ByPattern("hmp_TofTrack_All_.+");

  DrawH1ByPattern("hmp_TofTrack_Z");
  gPad->SetLogy(true);
  DrawH1ByPattern("hmp_Tof_Z");
  gPad->SetLogy(true);

  FitHistograms();
}

void CbmLitTofQaReport::FitHistograms()
{
  vector<string> categories = list_of("Electron")("Muon")("Pion")("Proton")("Kaon");
  Int_t nofCategories       = categories.size();
  for (Int_t iCat = 0; iCat < nofCategories; iCat++) {
    string canvasName = "tof_qa_fit_histograms_" + categories[iCat];
    TCanvas* canvas   = CreateCanvas(canvasName.c_str(), canvasName.c_str(), 1200, 1000);
    canvas->cd(1);
    string histName = "hmp_Tof_RecoMCID_" + categories[iCat] + "_m2p";
    TH2* hist       = HM()->H2(histName);
    DrawH2(hist);
    hist->FitSlicesY();
    TH1* meanHist   = gDirectory->Get<TH1>(string(histName + "_1").c_str());  // mean
    TH1* sigmaHist  = gDirectory->Get<TH1>(string(histName + "_2").c_str());  // sigma
    Int_t nofBins   = meanHist->GetNbinsX();
    TGraph* upGraph = new TGraph(nofBins);
    upGraph->GetXaxis()->SetRangeUser(meanHist->GetXaxis()->GetXmin(), meanHist->GetXaxis()->GetXmax());
    TGraph* downGraph = new TGraph(nofBins);
    downGraph->GetXaxis()->SetRangeUser(meanHist->GetXaxis()->GetXmin(), meanHist->GetXaxis()->GetXmax());
    TGraph* meanGraph = new TGraph(nofBins);
    meanGraph->GetXaxis()->SetRangeUser(meanHist->GetXaxis()->GetXmin(), meanHist->GetXaxis()->GetXmax());
    for (Int_t iBin = 1; iBin <= nofBins; iBin++) {
      Double_t p     = meanHist->GetBinCenter(iBin);
      Double_t mean  = meanHist->GetBinContent(iBin);
      Double_t sigma = sigmaHist->GetBinContent(iBin);
      upGraph->SetPoint(iBin - 1, p, mean + sigma);
      downGraph->SetPoint(iBin - 1, p, mean - sigma);
      meanGraph->SetPoint(iBin - 1, p, mean);
    }
    std::cout << "Upper function for " << categories[iCat] << std::endl;
    FitFunction(upGraph);
    std::cout << "Lower function for " << categories[iCat] << std::endl;
    FitFunction(downGraph);
    DrawGraph(meanGraph, kLinear, kLinear, "PSAME");
  }
}

void CbmLitTofQaReport::FitFunction(TGraph* graph)
{
  //TF1* f1 = new TF1("f1", "[0]*x*x*x*x+[1]*x*x*x+[2]*x*x+[3]*x+[4]", graph->GetXaxis()->GetXmin(), graph->GetXaxis()->GetXmax());
  TF1* f1 = new TF1("f1", "[0]*x*x*x*x+[1]*x*x*x+[2]*x*x+[3]*x+[4]", 1., 8.);
  f1->SetLineColor(kRed);
  DrawGraph(graph, kLinear, kLinear, "PSAME", kRed, 2, 1, 1, 21);
  graph->Fit(f1, "RQ");
  Double_t p0 = f1->GetParameter(0);
  Double_t p1 = f1->GetParameter(1);
  Double_t p2 = f1->GetParameter(2);
  Double_t p3 = f1->GetParameter(3);
  Double_t p4 = f1->GetParameter(4);
  std::cout << "Function: " << p0 << "*x^4" << string((p1 > 0) ? "+" : "") << p1 << "*x^3"
            << string((p2 > 0) ? "+" : "") << p2 << "*x^2" << string((p3 > 0) ? "+" : "") << p3 << "*x"
            << string((p4 > 0) ? "+" : "") << p4 << std::endl;
}

ClassImp(CbmLitTofQaReport)
