/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

/// \file   CbmQaUtil.cxx
/// \brief  Useful utilities for CBM QA tasks
/// \author S.Gorbunov
/// \data   31.07.2023


#include "CbmQaUtil.h"

#include "CbmQaCanvas.h"
#include "TAxis.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TH1.h"
#include "TPaletteAxis.h"
#include "TPaveStats.h"
#include "TStyle.h"
#include "TVirtualPad.h"

namespace cbm::qa::util
{

  // ---------------------------------------------------------------------------------------------------------------------
  //
  TPaveStats* GetHistStats(TH1* pHist)
  {
    //
    // Returns histogram stats window. Creates is if it doesn't exists.
    //

    TPaveStats* stats = (TPaveStats*) pHist->FindObject("stats");
    if (stats) {
      return stats;
    }

    TVirtualPad* padsav = gPad;
    auto styleSave      = gStyle->GetOptStat();

    CbmQaCanvas::GetDummyCanvas().cd();
    //pHist->SetStats(false);                                  // remove the old stat window
    pHist->SetStats(true);  // set the flag to create the stat window during Draw()
    if (styleSave == 0) {
      gStyle->SetOptStat(1);
    }  // set some stat option

    // protection of a warning for automatic axis binning
    TAxis* x = pHist->GetXaxis();
    TAxis* y = pHist->GetYaxis();
    TAxis* z = pHist->GetZaxis();
    assert(x);
    assert(y);
    assert(z);
    double rx[2]{x->GetXmin(), x->GetXmax()};
    double ry[2]{y->GetXmin(), y->GetXmax()};
    double rz[2]{z->GetXmin(), z->GetXmax()};

    x->SetLimits(0., 1.);
    y->SetLimits(0., 1.);
    z->SetLimits(0., 1.);

    pHist->Draw("");
    CbmQaCanvas::GetDummyCanvas().Update();
    CbmQaCanvas::GetDummyCanvas().Clear();

    x->SetLimits(rx[0], rx[1]);
    y->SetLimits(ry[0], ry[1]);
    z->SetLimits(rz[0], rz[1]);

    if (styleSave == 0) {
      gStyle->SetOptStat(styleSave);
    }
    if (padsav) padsav->cd();

    stats = (TPaveStats*) pHist->FindObject("stats");

    // at this point the statistics window should exist under all circumstances
    assert(stats);

    return stats;
  }


  // ---------------------------------------------------------------------------------------------------------------------
  //
  std::tuple<double, double> FitKaniadakisGaussian(TH1* pHist)
  {
    //
    // Fit function - Kaniadakis Gaussian Distribution:
    // https://en.wikipedia.org/wiki/Kaniadakis_Gaussian_distribution
    //
    // where exp_k(x) is calculated via arcsinh():
    // https://en.wikipedia.org/wiki/Kaniadakis_statistics
    //
    // parameters:
    //
    // [0] - mean
    // [1] - Std. Dev ( calculated after the fit )
    // [2] - peak
    // [3] - hwhm - half width at half maximum, scaled.  ( == standard deviation for a gaussian )
    // [4] - k - Kaniadakis parameter, k in [0.,1.]. ( == 0.0 for a gaussian. The formula below breaks when k is exactly 0.)
    //
    // returns mean and std.dev
    //

    auto retValue = std::tuple(0., -1.);

    double xMin = pHist->GetXaxis()->GetXmin();
    double xMax = pHist->GetXaxis()->GetXmax();

    TF1 fit("FitKaniadakisGaussian", "[2] * TMath::Exp(TMath::ASinH(-0.5*[4]*((x-[0])/[3])**2)/[4]) + 0.0*[1]", xMin,
            xMax, "NL");

    fit.SetParName(0, "fit_Mean");
    fit.SetParName(1, "fit_StdDev");
    fit.SetParName(2, "fit_Peak");
    fit.SetParName(3, "fit_Hwhm");
    fit.SetParName(4, "fit_k");

    // set reasonable initial values

    double mean = pHist->GetMean();
    double peak = pHist->GetBinContent(pHist->GetMaximumBin());
    double hwhm = pHist->GetStdDev();

    fit.SetParameters(mean, hwhm, peak, hwhm, .001);

    // limit parameter k

    fit.SetParLimits(4, 0.00001, 5.);

    // check if the histogram has enough entries excluding underflow and overflow bins
    double entries = 0.;
    for (int i = 1; i <= pHist->GetNbinsX(); ++i) {
      if (fabs(pHist->GetBinContent(i)) > 0.) {
        entries++;
      }
    }

    // fit the histogram in quite mode
    if (entries > 0) {
      pHist->Fit(&fit, "Q");
      TF1* f = pHist->GetFunction("FitKaniadakisGaussian");
      assert(f);

      // calculate the Std Dev and put it to parameter [1]

      if (entries > 1) {
        f->SetParameter(1, sqrt(f->CentralMoment(2, xMin, xMax)));
      }
      else {
        f->SetParameter(1, f->GetParameter(3));
      }
      f->SetParError(1, f->GetParError(3));

      // fix some parameters to prevent them from showing up in the stat window

      f->FixParameter(2, f->GetParameter(2));
      f->FixParameter(3, f->GetParameter(3));
      f->FixParameter(4, f->GetParameter(4));
      retValue = std::tuple(f->GetParameter(0), f->GetParameter(1));
    }

    TPaveStats* stats = GetHistStats(pHist);
    assert(stats);
    stats->SetX1NDC(0.7);
    stats->SetY1NDC(0.5);
    stats->SetOptStat(111110);
    stats->SetOptFit(100001);

    return retValue;
  }


  // ---------------------------------------------------------------------------------------------------------------------
  //
  void SetLargeStats(TH1* pHist)
  {
    //
    // Set large stat. window
    //

    TPaveStats* stats = GetHistStats(pHist);
    assert(stats);
    stats->SetX1NDC(0.6);
    stats->SetY1NDC(0.5);
    stats->SetOptStat(111110);
    stats->SetOptFit(100001);
  }


}  // namespace cbm::qa::util
