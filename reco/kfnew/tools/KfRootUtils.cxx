/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfRootUtils.cxx
/// \brief  Different ROOT utility functions for the KF-framework (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \date   01.11.2024

#include "KfRootUtils.h"

#include "KfMaterialMap.h"
#include "TH2F.h"
#include "TString.h"

namespace kf::tools
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  TH2F* RootUtils::ToHistogram(const cbm::algo::kf::MaterialMap& material, const TString& name, const TString& title)
  {
    int nBins = material.GetNbins();
    int xMin  = -material.GetXYmax();
    int xMax  = +material.GetXYmax();
    auto* h   = new TH2F(name, title, nBins, xMin, xMax, nBins, xMin, xMax);
    h->GetXaxis()->SetTitle("X [cm]");
    h->GetYaxis()->SetTitle("Y [cm]");
    h->GetZaxis()->SetTitle("thickness [% of X0]");
    for (int iX = 0; iX < material.GetNbins(); iX++) {
      for (int iY = 0; iY < material.GetNbins(); iY++) {
        h->SetBinContent(iX + 1, iY + 1, 100. * material.GetBinThicknessX0<float>(iX, iY));
      }
    }
    return h;
  }
}  // namespace kf::tools
