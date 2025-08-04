/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaCompare.cxx
/// \brief  A histogram comparison module for the QA task (implementation)
/// \author S.Zharko <s.zharko@lx-pool.gsi.de>
/// \since  23.12.2023

#include "CbmQaCompare.h"

#include "CbmQaCanvas.h"
#include "Logger.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TError.h"
#include "TGraphAsymmErrors.h"
#include "TH1.h"
#include "TH2.h"
#include "TLegend.h"
#include "TMath.h"
#include "TMultiGraph.h"
#include "TObject.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TStyle.h"

#include <sstream>
#include <type_traits>


// ---------------------------------------------------------------------------------------------------------------------
//
template<class Obj>
CbmQaCompare<Obj>::CbmQaCompare(const Obj* pHistL, const Obj* pHistR, int verbose)
  : fpObjL(pHistL)
  , fpObjR(pHistR)
  , fVerbose(verbose)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class Obj>
CbmQaCompare<Obj>::~CbmQaCompare()
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class Obj>
typename CbmQaCompare<Obj>::Result CbmQaCompare<Obj>::operator()(const std::string& opt,
                                                                 const std::string& optStat) const
{
  // ---- Parse options
  bool bCmpPointByPoint = opt.find("p") != std::string::npos;
  bool bCmpStatTest     = opt.find("s") != std::string::npos;
  bool bCmpRatio        = opt.find("r") != std::string::npos;

  Result res;


  if constexpr (std::is_base_of_v<TH1, Obj>) {
    auto CheckAxes = [&](const TAxis* pL, const TAxis* pR, const char* name) {
      bool ret = true;
      if (pL->GetNbins() != pR->GetNbins()) {
        LOG(warn) << "histogram " << name << " has inconsistent bin number with the default histogram";
        ret = false;
      }
      if (pL->GetXmin() != pR->GetXmin()) {
        LOG(warn) << "histogram " << name << " has inconsistent min of an axis x, y or z with the default histogram";
        ret = false;
      }
      if (pL->GetXmax() != pR->GetXmax()) {
        LOG(warn) << "histogram " << name << " has inconsistent max of an axis x, y or z with the default histogram";
        ret = false;
      }
      return ret;
    };

    // ---- Consistency check ------------------------------------------------------------------------------------------
    if (!(CheckAxes(fpObjL->GetXaxis(), fpObjR->GetXaxis(), fpObjL->GetName())
          && CheckAxes(fpObjL->GetYaxis(), fpObjR->GetYaxis(), fpObjL->GetName())
          && CheckAxes(fpObjL->GetZaxis(), fpObjR->GetZaxis(), fpObjL->GetName()))) {
      res.fConsistent = false;
      return res;
    }
    else {
      res.fConsistent = true;
    }

    // ---- Point-by-point comparison ----------------------------------------------------------------------------------
    if (bCmpPointByPoint || bCmpRatio) {
      res.fPointByPoint = true;
      res.fRatioLo      = 1.;
      res.fRatioUp      = 1.;
      for (int iBinX = 0; iBinX <= fpObjL->GetNbinsX(); ++iBinX) {
        for (int iBinY = 0; iBinY <= fpObjL->GetNbinsY(); ++iBinY) {
          for (int iBinZ = 0; iBinZ <= fpObjL->GetNbinsZ(); ++iBinZ) {
            auto numBinContent = fpObjL->GetBinContent(iBinX, iBinY, iBinZ);
            auto denBinContent = fpObjR->GetBinContent(iBinX, iBinY, iBinZ);
            double ratio       = static_cast<double>(numBinContent) / denBinContent;
            // Check bin content
            if (!TMath::IsNaN(numBinContent) && !TMath::IsNaN(denBinContent)) {
              if (numBinContent != denBinContent) {
                res.fPointByPoint = false;
                if (bCmpRatio) {
                  if (res.fRatioLo > ratio) {
                    res.fRatioLo = ratio;
                  }
                  if (res.fRatioUp < ratio) {
                    res.fRatioUp = ratio;
                  }
                  // NOTE: case numBinContent == denBinContent == 0 is accounted as true
                }
              }
            }
            else {
              if (TMath::IsNaN(numBinContent) != TMath::IsNaN(denBinContent)) {
                res.fPointByPoint = false;
              }
            }
            auto numBinError = fpObjL->GetBinError(iBinX, iBinY, iBinZ);
            auto denBinError = fpObjR->GetBinError(iBinX, iBinY, iBinZ);
            // Check bin error
            if (!TMath::IsNaN(numBinError) && !TMath::IsNaN(denBinError)) {
              if (numBinError != denBinError) {
                res.fPointByPoint = false;
              }
            }
            else {
              if (TMath::IsNaN(numBinError) != TMath::IsNaN(denBinError)) {
                res.fPointByPoint = false;
              }
            }
          }
        }
      }
    }
    // ---- Stat test comparison ---------------------------------------------------------------------------------------
    if (bCmpStatTest) {
      TString optParam = optStat + "CHI2/NDF";  // Set CHI2/NDF as an output
      res.fChi2NDF     = fpObjL->Chi2Test(fpObjR, optParam);
    }
  }
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class Obj>
TCanvas* CbmQaCompare<Obj>::GetComparisonCanvas(const std::string& opt)
{
  // ---- Constant parameters
  constexpr int kPadPX         = 500;   // Pad x-size in pixels
  constexpr int kPadPY         = 500;   // Pad y-size in pixels
  constexpr double kPadMarginL = 0.15;  // Left margin of a pad
  constexpr double kPadMarginR = 0.05;  // Right margin of a pad
  constexpr double kPadMarginB = 0.12;  // Bottom margin of a pad
  constexpr double kPadMarginT = 0.10;  // Top margin of a pad
  constexpr Style_t kMarkerL   = 24;
  constexpr Style_t kMarkerR   = 25;
  constexpr Style_t kMarkerDif = 20;
  constexpr Style_t kMarkerRat = 20;

  // ---- Parse options
  bool bDrawCmp = opt.find("c") != std::string::npos;
  bool bDrawRat = opt.find("r") != std::string::npos;
  bool bDrawDif = opt.find("d") != std::string::npos;
  int nPads     = bDrawCmp + bDrawDif + bDrawRat;

  if (!nPads) {
    return nullptr;
  }

  auto* pCanv = new TCanvas("comparison", "comparison", kPadPX * nPads, kPadPY);
  pCanv->Divide(nPads, 1);

  if constexpr (std::is_base_of_v<TH1, Obj>) {
    if (bDrawCmp) {
      pCanv->cd(1);
      gPad->SetMargin(kPadMarginL, kPadMarginR, kPadMarginB, kPadMarginT);
      auto* pMGraph = new TMultiGraph(Form("%s_cmp", fpObjL->GetName()), fpObjL->GetTitle());
      auto* pGrL    = new TGraphAsymmErrors(fpObjL);
      pGrL->SetLineColor(kOrange + 5);
      pGrL->SetMarkerColor(kOrange + 5);
      pGrL->SetMarkerStyle(kMarkerL);
      auto* pGrR = new TGraphAsymmErrors(fpObjR);
      pGrR->SetLineColor(kCyan - 2);
      pGrR->SetMarkerColor(kCyan - 2);
      pGrR->SetMarkerStyle(kMarkerR);
      pMGraph->Add(pGrL, "P");
      pMGraph->Add(pGrR, "P");
      pMGraph->GetXaxis()->SetTitle(fpObjL->GetXaxis()->GetTitle());
      pMGraph->GetYaxis()->SetTitle(fpObjL->GetYaxis()->GetTitle());
      pMGraph->Draw("AP");
      auto* pLegend = new TLegend(0.55, 0.80, (1. - kPadMarginR), (1. - kPadMarginT));
      pLegend->SetTextSize(0.035);
      //pLegend->SetBorderSize(0);
      pLegend->SetFillStyle(0);
      pLegend->SetMargin(0.2);
      pLegend->AddEntry(pGrL, fsLabelL.c_str(), "P");
      pLegend->AddEntry(pGrR, fsLabelR.c_str(), "P");
      pLegend->Draw();
    }

    TH1* pHistL = nullptr;  // Temporary histogram
    TH1* pHistR = nullptr;  // Temporary histogram
    if constexpr (std::is_base_of_v<TProfile2D, Obj>) {
      pHistL = fpObjL->ProjectionXY(Form("tmp_%s", fsLabelL.c_str()));
      pHistR = fpObjR->ProjectionXY(Form("tmp_%s", fsLabelR.c_str()));
    }
    else if constexpr (std::is_base_of_v<TProfile, Obj>) {
      pHistL = fpObjL->ProjectionX(Form("tmp_%s", fsLabelL.c_str()));
      pHistR = fpObjR->ProjectionX(Form("tmp_%s", fsLabelR.c_str()));
    }
    else {
      pHistL = static_cast<Obj*>(fpObjL->Clone(Form("tmp_%s", fsLabelL.c_str())));
      pHistR = static_cast<Obj*>(fpObjR->Clone(Form("tmp_%s", fsLabelR.c_str())));
    }

    // Ratio plot
    if (bDrawRat) {
      pCanv->cd(static_cast<int>(bDrawCmp) + static_cast<int>(bDrawDif) + 1);
      gPad->SetMargin(kPadMarginL, kPadMarginR, kPadMarginB, kPadMarginT);
      auto currErrorLevel = gErrorIgnoreLevel;  // Suppress log error about skipping null points
      gErrorIgnoreLevel   = kError;
      auto* pGrRat        = new TGraphAsymmErrors(pHistL, pHistR, "pois");
      gErrorIgnoreLevel   = currErrorLevel;
      pGrRat->GetXaxis()->SetTitle(pHistL->GetXaxis()->GetTitle());
      pGrRat->GetYaxis()->SetTitle("ratio");
      pGrRat->SetTitle(Form("Ratio of \"%s\" and \"%s\"", fsLabelL.c_str(), fsLabelR.c_str()));
      pGrRat->SetMarkerStyle(kMarkerRat);
      pGrRat->Draw("AP");
    }

    // Difference plot
    if (bDrawDif) {
      pCanv->cd(static_cast<int>(bDrawCmp) + 1);
      gPad->SetMargin(kPadMarginL, kPadMarginR, kPadMarginB, kPadMarginT);
      auto* pDif = static_cast<Obj*>(pHistL->Clone());
      pDif->Add(pHistR, -1.);
      auto* pGrDif = new TGraphAsymmErrors(pDif);
      pGrDif->GetXaxis()->SetTitle(pHistL->GetXaxis()->GetTitle());
      pGrDif->GetYaxis()->SetTitle("difference");
      pGrDif->SetTitle(Form("Difference of \"%s\" and \"%s\"", fsLabelL.c_str(), fsLabelR.c_str()));
      pGrDif->SetMarkerStyle(kMarkerDif);
      pGrDif->Draw("AP");
      delete pDif;
    }

    if (pHistL) {
      delete pHistL;
    }
    if (pHistR) {
      delete pHistR;
    }
  }

  return pCanv;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<class Obj>
void CbmQaCompare<Obj>::SetObjectLabels(const std::string& labelL, const std::string& labelR)
{
  fsLabelL = labelL;
  fsLabelR = labelR;
}

template class CbmQaCompare<TH1>;
template class CbmQaCompare<TH2>;
template class CbmQaCompare<TProfile>;
