/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmKFV0FinderQa.cxx
/// \brief  A simple QA for the V0 finder class (runs together with the finder task) (implementation)
/// \since  16.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmKFV0FinderQa.h"

#include <Logger.h>

#include <TFile.h>

using cbm::kfp::V0FinderQa;

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderQa::InitHistograms()
{
  TString n{""};  // name of histogram
  TString t{""};  // title of histogram

  n                    = "tof_lst_hit_time";
  t                    = "Time of last TOF hit in track;t [ns];Counts";
  fph_tof_lst_hit_time = MakeQaObject<TH1D>(n, t, 200, -100., 100.);

  n            = "beta_all";
  t            = "Speed of tracks (all);#beta;Counts";
  fph_beta_all = MakeQaObject<TH1D>(n, t, 150, 0., 1.5);

  {
    n       = "dca";
    t       = "DCA to the origin of the selected tracks;dca [cm];Counts";
    fph_dca = MakeQaObject<TH1D>(n, t, 120, 0., 6.);

    n         = "dca2D";
    t         = "DCA to the origin of the selected tracks;dca_{x} [cm];dca_{y} [cm];Counts";
    fph_dca2D = MakeQaObject<TH2D>(n, t, 240, -6., 6., 240, -6., 6.);

    n                   = "dca_projectionX";
    t                   = "DCA to the origin of the selected tracks;dca_{x} [cm];Counts";
    fph_dca_projectionX = MakeQaObject<TH1D>(n, t, 240, -6., 6.);

    n                   = "dca_projectionY";
    t                   = "DCA to the origin of the selected tracks;dca_{y} [cm];Counts";
    fph_dca_projectionY = MakeQaObject<TH1D>(n, t, 240, -6., 6.);

    n                    = "lambda_cand_mass";
    t                    = "Mass of lambda candidate;m [GeV/^{2}];Counts";
    fph_lambda_cand_mass = MakeQaObject<TH1D>(n, t, 200, 1.05, 1.15);
  }
}


// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderQa::WriteHistograms(const TString& fileName)
{
  LOG(info) << "V0FinderQa: writing histograms to file: " << fileName;
  TFile fileOut(fileName, "RECREATE");
  this->WriteToFile(&fileOut);
  fileOut.Close();
}
