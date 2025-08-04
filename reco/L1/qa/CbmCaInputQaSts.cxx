/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaSts.cxx
/// \date   13.01.2023
/// \brief  QA-task for CA tracking input from MuCh detector (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaSts.h"

#include "CbmAddress.h"
#include "CbmMCDataArray.h"
#include "CbmMCEventList.h"
#include "CbmMCTrack.h"
#include "CbmMatch.h"
#include "CbmQaCanvas.h"
#include "CbmQaTable.h"
#include "CbmQaUtil.h"
#include "CbmStsCluster.h"
#include "CbmStsHit.h"
#include "CbmStsPoint.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTimeSlice.h"
#include "FairRootManager.h"
#include "Logger.h"
#include "TBox.h"
#include "TClonesArray.h"
#include "TEllipse.h"
#include "TF1.h"
#include "TFormula.h"
#include "TGraphAsymmErrors.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMath.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TStyle.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <tuple>

ClassImp(CbmCaInputQaSts);

// ---------------------------------------------------------------------------------------------------------------------
//
CbmCaInputQaSts::CbmCaInputQaSts(int verbose, bool isMCUsed) : CbmCaInputQaBase("CbmCaInputQaSts", verbose, isMCUsed)
{
  // Default parameters of task
  DefineParameters();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaSts::Check()
{
  CbmCaInputQaBase::Check();
  if (IsMCUsed()) {
    for (int idig = 0; idig <= fkMaxDigisInClusterForPulls; idig++) {
      cbm::qa::util::SetLargeStats(fvph_pull_u_Ndig[idig]);
      auto [msgU, resU] = CheckRangePull(fvph_pull_u_Ndig[idig]);
      StoreCheckResult(Form("pull_u_%d_digis", idig), resU, msgU);

      cbm::qa::util::SetLargeStats(fvph_pull_v_Ndig[idig]);
      auto [msgV, resV] = CheckRangePull(fvph_pull_v_Ndig[idig]);
      StoreCheckResult(Form("pull_v_%d_digis", idig), resV, msgV);
    }
  }  // McUsed
}


// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaSts::CreateSummary()
{
  gStyle->SetOptFit(1);

  if (IsMCUsed()) {
    {  // u-coordinate vs N digis
      auto* canv = MakeQaObject<CbmQaCanvas>("vs N digi/pull_u", "Pulls for u coordinate vs N digis", 1600, 800);
      canv->Divide2D(fkMaxDigisInClusterForPulls + 1);
      for (int i = 0; i <= fkMaxDigisInClusterForPulls; ++i) {
        canv->cd((i > 0) ? i : fkMaxDigisInClusterForPulls + 1);
        fvph_pull_u_Ndig[i]->DrawCopy("", "");
      }
    }

    {  // v-coordinate vs N digis
      auto* canv = MakeQaObject<CbmQaCanvas>("vs N digi/pull_v", "Pulls for v coordinate vs N digis", 1600, 800);
      canv->Divide2D(fkMaxDigisInClusterForPulls + 1);
      for (int i = 0; i <= fkMaxDigisInClusterForPulls; ++i) {
        canv->cd((i > 0) ? i : fkMaxDigisInClusterForPulls + 1);
        fvph_pull_v_Ndig[i]->DrawCopy("", "");
      }
    }
  }

  CbmCaInputQaBase::CreateSummary();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaSts::DeInit()
{
  CbmCaInputQaBase::DeInit();

  // Vectors with pointers to histograms
  fvph_pull_u_Ndig.clear();
  fvph_pull_v_Ndig.clear();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaSts::DefineParameters()
{
  auto SetRange = [](std::array<double, 2>& range, double min, double max) {
    range[0] = min;
    range[1] = max;
  };
  // Hit errors
  SetRange(fRHitDx, 0.0000, 0.0050);  // [cm]
  SetRange(fRHitDy, 0.0000, 0.0200);  // [cm]
  SetRange(fRHitDu, 0.0000, 0.0050);  // [cm]
  SetRange(fRHitDv, 0.0000, 0.0050);  // [cm]
  SetRange(fRHitDt, 0.0000, 10.000);  // [ns]
  // Residuals
  SetRange(fRResX, -0.02, 0.02);
  SetRange(fRResY, -0.10, 0.10);
  SetRange(fRResU, -0.02, 0.02);
  SetRange(fRResV, -0.02, 0.02);
  SetRange(fRResT, -25.0, 25.0);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaSts::FillHistogramsPerHit()
{
  const auto* pHit = dynamic_cast<const CbmStsHit*>(fpHits->At(fHitQaData.GetHitIndex()));

  if (IsMCUsed()) {  // u-coordinate residual per number of digis
    const auto* pCluster = dynamic_cast<const CbmStsCluster*>(fpClusters->At(pHit->GetFrontClusterId()));
    assert(pCluster);
    int nDigis = pCluster->GetNofDigis();
    if (nDigis > fkMaxDigisInClusterForPulls) {
      nDigis = 0;
    }
    fvph_pull_u_Ndig[nDigis]->Fill(fHitQaData.GetPullU());
  }

  if (IsMCUsed()) {  // v-coordinate residual per number of digis
    const auto* pCluster = dynamic_cast<const CbmStsCluster*>(fpClusters->At(pHit->GetBackClusterId()));
    assert(pCluster);
    int nDigis = pCluster->GetNofDigis();
    if (nDigis > fkMaxDigisInClusterForPulls) {
      nDigis = 0;
    }
    fvph_pull_v_Ndig[nDigis]->Fill(fHitQaData.GetPullV());
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaSts::FillHistogramsPerPoint() {}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmCaInputQaSts::InitQa()
{
  // ----- Detector intereface
  //
  fpDetInterface = CbmStsTrackingInterface::Instance();

  auto initStatus = CbmCaInputQaBase::InitQa();
  if (initStatus != kSUCCESS) {
    return initStatus;
  }

  // ----- Input data initialization
  //
  fpClusters = dynamic_cast<TClonesArray*>(FairRootManager::Instance()->GetObject("StsCluster"));
  LOG_IF(fatal, !fpClusters) << "\033[1;31m" << fName << ": container of hit clusters in STS is not found\033[0m";

  // ----- Histogram initialization
  //
  int nSt             = fpDetInterface->GetNtrackingStations();
  std::string detName = fpDetInterface->GetDetectorName();

  // rename some histogramms with respect to STS specifics
  for (int iSt = 0; iSt <= nSt; ++iSt) {
    TString tsuff = (iSt == nSt) ? "" : Form(" in %s station %d", detName.c_str(), iSt);  // Histogram title suffix
    fvph_hit_du[iSt]->SetTitle((TString) "Hit position error across front strips" + tsuff);
    fvph_hit_dv[iSt]->SetTitle((TString) "Hit position error across back strips" + tsuff);
    fvph_hit_kuv[iSt]->SetTitle((TString) "Hit error correlation between front and back strips" + tsuff);
    if (IsMCUsed()) {
      fvph_res_u[iSt]->SetTitle((TString) "Residuals for Front strip coordinate" + tsuff);
      fvph_res_v[iSt]->SetTitle((TString) "Residuals for Back strip coordinate" + tsuff);
      fvph_pull_u[iSt]->SetTitle((TString) "Pulls for Front strip coordinate" + tsuff);
      fvph_pull_v[iSt]->SetTitle((TString) "Pulls for Back strip coordinate" + tsuff);
      fvph_res_u_vs_u[iSt]->SetTitle((TString) "Residuals for Front strip coordinate" + tsuff);
      fvph_res_v_vs_v[iSt]->SetTitle((TString) "Residuals for Back strip coordinate" + tsuff);
      fvph_pull_u_vs_u[iSt]->SetTitle((TString) "Pulls for Front strip coordinate" + tsuff);
      fvph_pull_v_vs_v[iSt]->SetTitle((TString) "Pulls for Back strip coordinate" + tsuff);
    }
  }

  if (IsMCUsed()) {
    fvph_pull_u_Ndig.resize(fkMaxDigisInClusterForPulls + 1, nullptr);
    fvph_pull_v_Ndig.resize(fkMaxDigisInClusterForPulls + 1, nullptr);

    for (int idig = 0; idig <= fkMaxDigisInClusterForPulls; idig++) {
      TString sN = "All stations/pull/Ndigi/pull_u_";
      TString sT = "Pulls for Front strip coordinate, hits with ";
      if (idig == 0) {
        sN += Form("%d+_digi", fkMaxDigisInClusterForPulls + 1);
        sT += Form(">=%d digi", fkMaxDigisInClusterForPulls + 1);
      }
      else {
        sN += Form("%d_digi", idig);
        sT += Form("%d digi", idig);
      }
      sT += "; (u_{reco} - u_{MC}) / #sigma_{u}^{reco}";
      fvph_pull_u_Ndig[idig] = MakeQaObject<TH1F>(sN, sT, kNbinsPull, kRPull[0], kRPull[1]);
    }

    for (int idig = 0; idig <= fkMaxDigisInClusterForPulls; idig++) {
      TString sN = "All stations/pull/Ndigi/pull_v_";
      TString sT = "Pulls for Back strip coordinate, hits with ";
      if (idig == 0) {
        sN += Form("%d+_digi", fkMaxDigisInClusterForPulls + 1);
        sT += Form(">=%d digi", fkMaxDigisInClusterForPulls + 1);
      }
      else {
        sN += Form("%d_digi", idig);
        sT += Form("%d digi", idig);
      }
      sT += "; (v_{reco} - v_{MC}) / #sigma_{v}^{reco}";
      fvph_pull_v_Ndig[idig] = MakeQaObject<TH1F>(sN, sT, kNbinsPull, kRPull[0], kRPull[1]);
    }
  }

  return kSUCCESS;
}
