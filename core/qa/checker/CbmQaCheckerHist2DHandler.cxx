/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerHist2DHandler.h
/// @brief  Handler class for 2D-histograms (implementation)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  21.02.2023

#include "CbmQaCheckerHist2DHandler.h"

#include "TCanvas.h"
#include "TH2.h"

using cbm::qa::checker::Hist2DHandler;

// ---------------------------------------------------------------------------------------------------------------------
//
Hist2DHandler::Hist2DHandler(int iObject, int iFile, int iDataset) : Hist1DHandler(iObject, iFile, iDataset) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void Hist2DHandler::CreateCanvases(Option_t*)
{
  std::string sCanvName = fsBaseName + "_cmp_canvas";

  // Title definitions
  const char* title      = fvpObjects[0]->GetTitle();
  const char* xAxisTitle = static_cast<TH1*>(fvpObjects[0])->GetXaxis()->GetTitle();
  const char* yAxisTitle = static_cast<TH1*>(fvpObjects[0])->GetYaxis()->GetTitle();
  const char* zAxisTitle = static_cast<TH1*>(fvpObjects[0])->GetZaxis()->GetTitle();

  int nVersions = fpObjDB->GetNofVersions();
  fpCanvas      = std::make_shared<TCanvas>(sCanvName.data(), title, 1500, 700);
  fpCanvas->DivideSquare(nVersions);
  for (int iVer = 0; iVer < nVersions; ++iVer) {
    fpCanvas->cd(iVer + 1);
    gPad->SetMargin(0.20, 0.20, 0.20, 0.10);
    auto* pCopy = static_cast<TH2*>(static_cast<TH2*>(fvpObjects[iVer])->Clone());
    pCopy->SetTitle(Form("%s: %s", title, fpObjDB->GetVersionLabel(iVer).c_str()));
    pCopy->GetXaxis()->SetTitle(xAxisTitle);
    pCopy->GetYaxis()->SetTitle(yAxisTitle);
    pCopy->GetZaxis()->SetTitle(zAxisTitle);
    pCopy->Draw("colz");
  }
}
