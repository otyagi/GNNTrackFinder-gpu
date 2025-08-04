/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerHist1DHandler.cxx
/// @brief  Handler class for 1D-histograms (including TProfile objects) (implementation)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  09.02.2023

#include "CbmQaCheckerHist1DHandler.h"

#include "CbmQaCheckerTypedefs.h"
#include "Logger.h"
#include "TCanvas.h"
#include "TError.h"
#include "TFolder.h"
#include "TGraphAsymmErrors.h"
#include "TH1.h"
#include "TLegend.h"
#include "TMath.h"
#include "TMultiGraph.h"
#include "TObject.h"
#include "TStyle.h"

#include <bitset>
#include <limits>

using cbm::qa::checker::ECmpInference;
using cbm::qa::checker::Hist1DHandler;

// ---------------------------------------------------------------------------------------------------------------------
//
Hist1DHandler::Hist1DHandler(int iObject, int iFile, int iDataset) : ObjectHandler(iObject, iFile, iDataset, "TH1") {}

// ---------------------------------------------------------------------------------------------------------------------
//


// ---------------------------------------------------------------------------------------------------------------------
// TODO: move somewhere else (probably unite with the CbmQaCompare in a future merger request)
ECmpInference Hist1DHandler::Compare(int iVersion) const
{
  auto* pNumerator   = static_cast<const TH1*>(fvpObjects[iVersion]);
  auto* pDenominator = static_cast<const TH1*>(fvpObjects[fpObjDB->GetDefaultID()]);
  if (pNumerator->GetNbinsX() != pDenominator->GetNbinsX() || pNumerator->GetNbinsY() != pDenominator->GetNbinsY()
      || pNumerator->GetNbinsZ() != pDenominator->GetNbinsZ()) {
    return ECmpInference::Different;
  }

  bool bCompareExact{fCmpBits[static_cast<uint8_t>(ECmpMethod::Exact)]};
  bool bCompareRatio{fCmpBits[static_cast<uint8_t>(ECmpMethod::Ratio)]};
  bool bCompareChi2{fCmpBits[static_cast<uint8_t>(ECmpMethod::Chi2)]};

  bool bEqualExactly{true};
  bool bEqualByRatio{true};

  double ratioLo{1.};
  double ratioUp{1.};

  if (bCompareExact || bCompareRatio) {
    bool bRatioMayBeWrong =
      false;  // handling a case, when the histograms contain different empty bins, but the ratio remains 1
    for (int iBinX{0}; iBinX <= pNumerator->GetNbinsX(); ++iBinX) {
      for (int iBinY{0}; iBinY <= pNumerator->GetNbinsY(); ++iBinY) {
        for (int iBinZ{0}; iBinZ <= pNumerator->GetNbinsZ(); ++iBinZ) {
          auto numBinContent{pNumerator->GetBinContent(iBinX, iBinY, iBinZ)};
          auto denBinContent{pDenominator->GetBinContent(iBinX, iBinY, iBinZ)};
          double ratio{static_cast<double>(numBinContent) / denBinContent};
          // Check bin content
          if (!TMath::IsNaN(numBinContent) && !TMath::IsNaN(denBinContent)) {
            if (numBinContent != denBinContent) {
              bEqualExactly = false;
              if (bCompareRatio) {
                bool numEmpty{numBinContent < 1.e-4};
                bool denEmpty{denBinContent < 1.e-4};
                if (numEmpty || denEmpty) {
                  bRatioMayBeWrong &= (!numEmpty || !denEmpty);
                  continue;  // Ignoring empty bin in ratio estimation (but only, if it is empty in both histograms)
                }
                ratioLo = std::min(ratioLo, ratio);
                ratioUp = std::max(ratioUp, ratio);
              }
            }
          }
          else {
            if (TMath::IsNaN(numBinContent) != TMath::IsNaN(denBinContent)) {
              bEqualExactly = false;
            }
          }
          auto numBinError{pNumerator->GetBinError(iBinX, iBinY, iBinZ)};
          auto denBinError{pDenominator->GetBinError(iBinX, iBinY, iBinZ)};
          // Check bin error
          if (!TMath::IsNaN(numBinError) && !TMath::IsNaN(denBinError)) {
            if (numBinError != denBinError) {
              bEqualExactly = false;
            }
          }
          else {
            if (TMath::IsNaN(numBinError) != TMath::IsNaN(denBinError)) {
              bEqualExactly = false;
            }
          }
        }
      }
    }

    if (bCompareExact && bEqualExactly) {
      return ECmpInference::StronglyEqual;  // chi2 and ratio tests can be ignored
    }

    if (bRatioMayBeWrong && (std::fabs(1 - 1.e-4) < ratioLo) && std::fabs(1 + 1.e+4) > ratioUp) {
      LOG(warn) << "Hist1DHandler::Compare: file " << fpObjDB->GetInputFileName(fDatasetID, fFileID, 0)
                << ", object: " << pNumerator->GetName()
                << " has a ratio equal to 1., but some of bins are still different "
                << "(empty/non-empty bin case)";
    }

    if (bCompareRatio) {
      bEqualByRatio = (ratioLo >= fpObjDB->GetRatioRangeMin() && ratioUp <= fpObjDB->GetRatioRangeMax());
    }
  }

  bool bEqualByChi2{true};
  double pVal{0.};
  if (bCompareChi2) {  // perform only, if the histograms were different
    auto currErrorLevel{gErrorIgnoreLevel};
    gErrorIgnoreLevel = kFatal;
    double chi2{0.};
    int ndf{0};
    int igood{0};
    pVal              = pDenominator->Chi2TestX(pNumerator, chi2, ndf, igood);
    bEqualByChi2      = (pVal >= fpObjDB->GetPvalThreshold());
    gErrorIgnoreLevel = currErrorLevel;
  }

  // Do not account for method, if it was not required
  // strongly eq = (!bCmp_0 v bEq_0) ^ (!bCmp_1 v bEq_1) ^ .. ^ (!bCmp_N v bEq_N)
  // weakly eq   = (!bCmp_0 v bEq_0) v (!bCmp_1 v bEq_1) v .. v (!bCmp_N v bEq_N)
  bEqualExactly       = !bCompareExact || bEqualExactly;
  bEqualByRatio       = !bCompareRatio || bEqualByRatio;
  bEqualByChi2        = !bCompareChi2 || bEqualByChi2;
  bool bEqualStrongly = bEqualExactly && bEqualByRatio && bEqualByChi2;
  bool bEqualWeakly   = bEqualExactly || bEqualByRatio || bEqualByChi2;
  ECmpInference res   = bEqualStrongly ? ECmpInference::StronglyEqual
                                       : (bEqualWeakly ? ECmpInference::WeaklyEqual : ECmpInference::Different);

  if (ECmpInference::StronglyEqual != res) {
    auto ResultMsg = [](bool flag) -> std::string {
      constexpr const char* kEqual = "\e[1;32mconsistent\e[0m";
      constexpr const char* kDiff  = "\e[1;31minconsistent\e[0m";
      return flag ? kEqual : kDiff;
    };
    std::stringstream msg;
    msg << "Found mismatch: file: " << fpObjDB->GetInputFileName(iVersion, fFileID, fDatasetID)
        << ", object: " << fpObjDB->GetObject(fFileID, fObjectID) << "\n";
    if (bCompareExact) {
      msg << "\texact -> " << ResultMsg(bEqualExactly) << '\n';
    }
    if (bCompareRatio) {
      msg << "\tratio -> " << ResultMsg(bEqualByRatio) << "(lo: " << ratioLo << ", up: " << ratioUp << ")\n";
    }
    if (bCompareChi2) {
      msg << "\tchi2 ->  " << ResultMsg(bEqualByChi2) << "(p-val: " << pVal << ")\n";
    }
    msg << "Inference: " << ToString(res);

    LOG(info) << msg.str();
  }

  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Hist1DHandler::CreateCanvases(Option_t* opt)
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
  fpCanvas               = std::make_shared<TCanvas>(sCanvName.data(), sCanvTitle.data(), 500 * nPads, 500);
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

  // Title definitions
  const char* title      = fvpObjects[0]->GetTitle();
  const char* titleRatio = Form("Ratio to %s", fpObjDB->GetVersionLabel(iDef).data());
  const char* titleDiff  = Form("Difference from %s", fpObjDB->GetVersionLabel(iDef).data());
  const char* xAxisTitle = static_cast<TH1*>(fvpObjects[0])->GetXaxis()->GetTitle();
  const char* yAxisTitle = static_cast<TH1*>(fvpObjects[0])->GetYaxis()->GetTitle();

  // Original histograms
  pPadOrig->cd();
  auto* pMultiGraphOrig = new TMultiGraph(fsBaseName.data(), title);
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

  auto* pDefault = static_cast<TH1*>(fvpObjects[iDef]);

  // Histogram ratios to default
  if (bDrawRatio) {
    pPadRatio->cd();
    auto* pMultiGraphRatio = new TMultiGraph((fsBaseName + "_ratio").data(), titleRatio);
    for (int iV = 0; iV < nVersions; ++iV) {
      if (iV == iDef) {
        continue;
      }
      auto* pRatio = static_cast<TH1*>(fvpObjects[iV]->Clone());
      pRatio->SetDirectory(fpOutDir);
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
    pMultiGraphRatio->GetYaxis()->SetTitle("ratio");
    pMultiGraphRatio->GetXaxis()->SetTitle(xAxisTitle);
    pMultiGraphRatio->Draw("A pmc plc");
  }

  // Histogram differences to default
  if (bDrawDiff) {
    pPadDiff->cd();
    auto* pMultiGraphDiff = new TMultiGraph((fsBaseName + "_diff").data(), titleDiff);
    for (int iV = 0; iV < nVersions; ++iV) {
      if (iV == iDef) {
        continue;
      }
      auto* pDiff = static_cast<TH1*>(fvpObjects[iV]->Clone());
      pDiff->SetDirectory(fpOutDir);
      pDiff->Add(pDefault, -1.);
      auto* pCopy = new TGraphAsymmErrors(pDiff);
      pCopy->SetMarkerStyle(20);
      pMultiGraphDiff->Add(pCopy, "P");
      if (pDiff) {
        delete pDiff;
        pDiff = nullptr;
      }
    }
    pMultiGraphDiff->GetYaxis()->SetTitle("difference");
    pMultiGraphDiff->GetXaxis()->SetTitle(xAxisTitle);
    pMultiGraphDiff->Draw("A pmc plc");
  }
  fpCanvas->cd();
  pPadOrig->Draw();
  if (pPadRatio) pPadRatio->Draw();
  if (pPadDiff) pPadDiff->Draw();
}

