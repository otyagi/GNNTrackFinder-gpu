/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerProfile1DHandler.h
/// @brief  Handler class for 1D-profiles (implementation)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  19.02.2023

#include "CbmQaCheckerProfile1DHandler.h"

#include "CbmQaCheckerHist1DHandler.h"
#include "CbmQaCheckerTypedefs.h"
#include "Logger.h"
#include "TCanvas.h"
#include "TError.h"
#include "TFolder.h"
#include "TGraphAsymmErrors.h"
#include "TLegend.h"
#include "TMath.h"
#include "TMultiGraph.h"
#include "TObject.h"
#include "TProfile.h"
#include "TRatioPlot.h"
#include "TStyle.h"

#include <limits>

using cbm::qa::checker::Profile1DHandler;

// ---------------------------------------------------------------------------------------------------------------------
//
Profile1DHandler::Profile1DHandler(int iObject, int iFile, int iDataset) : Hist1DHandler(iObject, iFile, iDataset) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void Profile1DHandler::CreateCanvases(Option_t* opt)
{
  int nVersions = fpObjDB->GetNofVersions();
  int iDef      = fpObjDB->GetDefaultID();

  // ----- Option parsing
  std::string sOption = opt;
  for (auto& ch : sOption) {
    ch = std::tolower(ch);
  }
  bool bDrawRatio = sOption.find("r") != std::string::npos;
  bool bDrawDiff  = sOption.find("d") != std::string::npos;
  int nPads       = static_cast<int>(bDrawRatio) + static_cast<int>(bDrawDiff) + 1;

  // ----- Canvas: comparison of original histograms, ratios and subtractions
  std::string sCanvName  = fsBaseName + "_cmp_canvas";
  std::string sCanvTitle = "Comparison result for \"" + fsBaseName + "\"";
  fpCanvas               = std::make_shared<TCanvas>(sCanvName.data(), sCanvTitle.data(), 1500, 500);
  fpCanvas->cd();

  TPad* pPadOrig  = nullptr;
  TPad* pPadRatio = nullptr;
  TPad* pPadDiff  = nullptr;

  double xPadSize = 1.0 / nPads;
  double xNext    = 0.;

  pPadOrig = new TPad("p1", "p1", 0.0, 0.0, xPadSize, 1.0);
  pPadOrig->SetMargin(0.20, 0.05, 0.20, 0.10);
  fpCanvas->cd();
  xNext += xPadSize;

  if (bDrawRatio) {
    pPadRatio = new TPad("p2", "p2", xNext, 0.0, xNext + xPadSize, 1.0);
    pPadRatio->SetMargin(0.20, 0.05, 0.20, 0.10);
    fpCanvas->cd();
    xNext += xPadSize;
  }

  if (bDrawDiff) {
    pPadDiff = new TPad("p3", "p3", xNext, 0.0, xNext + xPadSize, 1.0);
    pPadDiff->SetMargin(0.20, 0.05, 0.20, 0.10);
    fpCanvas->cd();
    xNext += xPadSize;
  }

  const char* title      = fvpObjects[0]->GetTitle();
  const char* titleRatio = Form("Ratio to %s", fpObjDB->GetVersionLabel(iDef).data());
  const char* titleDiff  = Form("Difference from %s", fpObjDB->GetVersionLabel(iDef).data());
  const char* xAxisTitle = static_cast<TProfile*>(fvpObjects[0])->GetXaxis()->GetTitle();
  const char* yAxisTitle = static_cast<TProfile*>(fvpObjects[0])->GetYaxis()->GetTitle();

  // Original histograms
  pPadOrig->cd();
  auto* pMultiGraphOrig = new TMultiGraph(fsBaseName.data(), title);
  //auto* pLegend = new TLegend(kLegendSize[0], kLegendSize[1]);
  for (int iV = 0; iV < nVersions; ++iV) {
    auto* pCopy = new TGraphAsymmErrors((TH1*) fvpObjects[iV]);
    pCopy->SetMarkerStyle(20);
    pCopy->SetTitle(fpObjDB->GetVersionLabel(iV).data());
    pMultiGraphOrig->Add(pCopy, "P");
  }
  pMultiGraphOrig->GetXaxis()->SetTitle(xAxisTitle);
  pMultiGraphOrig->GetYaxis()->SetTitle(yAxisTitle);
  pMultiGraphOrig->Draw("A pmc plc");
  pPadOrig->BuildLegend();
  //pLegend->Draw();

  auto* pDefault = static_cast<TProfile*>(fvpObjects[iDef])->ProjectionX();

  // Histogram ratios to default
  if (bDrawRatio) {
    pPadRatio->cd();
    auto* pMultiGraphRatio = new TMultiGraph((fsBaseName + "_ratio").data(), titleRatio);
    for (int iV = 0; iV < nVersions; ++iV) {
      if (iV == iDef) {
        continue;
      }
      auto* pRatio        = static_cast<TProfile*>(fvpObjects[iV])->ProjectionX();
      auto currErrorLevel = gErrorIgnoreLevel;
      gErrorIgnoreLevel   = kError;
      auto* pCopy         = new TGraphAsymmErrors(pRatio, pDefault, "pois");
      gErrorIgnoreLevel   = currErrorLevel;
      pCopy->SetMarkerStyle(20);
      pMultiGraphRatio->Add(pCopy, "P");

      if (pRatio) {
        delete pRatio;
        pRatio = nullptr;
      }
    }
    pMultiGraphRatio->GetXaxis()->SetTitle(xAxisTitle);
    pMultiGraphRatio->GetYaxis()->SetTitle("ratio");
    pMultiGraphRatio->Draw("A pmc plc");
  }

  // Histogram ratios to default
  if (bDrawDiff) {
    pPadDiff->cd();
    auto* pMultiGraphDiff = new TMultiGraph((fsBaseName + "_diff").data(), titleDiff);
    for (int iV = 0; iV < nVersions; ++iV) {
      if (iV == iDef) {
        continue;
      }
      auto* pDiff = static_cast<TProfile*>(fvpObjects[iV])->ProjectionX();
      pDiff->Add(pDefault, -1.);
      auto* pCopy = new TGraphAsymmErrors(pDiff);
      pCopy->GetYaxis()->SetTitle("difference");
      pCopy->SetMarkerStyle(20);
      pMultiGraphDiff->Add(pCopy, "P");

      if (pDiff) {
        delete pDiff;
        pDiff = nullptr;
      }
    }
    pMultiGraphDiff->GetXaxis()->SetTitle(xAxisTitle);
    pMultiGraphDiff->GetYaxis()->SetTitle("difference");
    pMultiGraphDiff->Draw("A pmc plc");
  }

  fpCanvas->cd();
  pPadOrig->Draw();
  if (pPadRatio) pPadRatio->Draw();
  if (pPadDiff) pPadDiff->Draw();

  if (pDefault) {
    delete pDefault;
    pDefault = nullptr;
  }
}
