/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaTrackFitQa.cxx
/// @brief  QA submodule for track fit results (residuals and puls) at selected z-coordinate (implementation)
/// @since  03.04.2023
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "CbmCaTrackFitQa.h"

#include "CaToolsField.h"
#include "CaToolsMCData.h"
#include "CbmL1Track.h"
#include "CbmQaCanvas.h"
#include "CbmQaUtil.h"
#include "KfFieldRegion.h"
#include "KfTrackKalmanFilter.h"
#include "TF1.h"
#include "TFormula.h"
#include "TH1.h"
#include "TProfile.h"
#include "TString.h"

#include <algorithm>

using namespace cbm::algo::ca;
using cbm::algo::kf::TrackParamD;

// *******************************************************
// **  Implementation of cbm::ca::TrackFitQa functions  **
// *******************************************************

using cbm::ca::TrackFitQa;
using cbm::ca::tools::MCPoint;

using namespace cbm::algo;  // TODO: SZh 16.05.2023: Remove this line

// ---------------------------------------------------------------------------------------------------------------------
//
TrackFitQa::TrackFitQa(const char* pointTag, const char* prefix, std::shared_ptr<ObjList_t> pObjList)
  : CbmQaIO(Form("%s_%s", prefix, pointTag), pObjList)
{
  fStoringMode = EStoringMode::kSAMEDIR;
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmQaCanvas* TrackFitQa::CreateResidualPlot()
{
  auto* pCanv = MakeQaObject<CbmQaCanvas>("canv_residuals", " residuals", kCXSIZEPX * 4, kCYSIZEPX * 2);
  pCanv->Divide2D(7);


  for (int iType = static_cast<int>(ETrackParType::BEGIN); iType < static_cast<int>(ETrackParType::END); ++iType) {
    ETrackParType type = static_cast<ETrackParType>(iType);
    if (fvbParIgnored[type]) {
      continue;
    }
    pCanv->cd(iType + 1);
    fvphResiduals[type]->Draw();
  }

  return pCanv;
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmQaCanvas* TrackFitQa::CreatePullPlot()
{
  auto* pCanv = MakeQaObject<CbmQaCanvas>(fsPrefix + "_canv_pulls", " pulls", kCXSIZEPX * 4, kCYSIZEPX * 2);
  pCanv->Divide2D(7);

  for (int iType = static_cast<int>(ETrackParType::BEGIN); iType < static_cast<int>(ETrackParType::END); ++iType) {
    ETrackParType type = static_cast<ETrackParType>(iType);
    if (fvbParIgnored[type]) {
      continue;
    }
    auto fit = TF1("fitpull", "[0] * TMath::Exp(TMath::ASinH(-0.5*[3]*((x-[1])/[2])**2)/[3])", -10., 10.);
    fit.SetParameters(100, 0., 1., .3);
    fit.SetParLimits(3, 0., 2.);
    pCanv->cd(iType + 1);
    fvphPulls[type]->Draw();
    cbm::qa::util::FitKaniadakisGaussian(fvphPulls[type]);
  }

  return pCanv;
}

// ---------------------------------------------------------------------------------------------------------------------
void TrackFitQa::FillResAndPull(ETrackParType type, double recoVal, double recoErr, double trueVal)
{
  if (fvbParIgnored[type]) {
    return;
  }
  double res  = recoVal - trueVal;
  double pull = res / recoErr;
  if (type == ETrackParType::kQP) {  // for the q/p parameter, the residual is calculated for the momentum
    res = (recoVal / trueVal - 1.);
  }
  fvphResiduals[type]->Fill(res);
  fvphPulls[type]->Fill(pull);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackFitQa::Init()
{
  // Init default distribution properties
  SetDefaultProperties();

  cbm::ca::tools::SetOriginalCbmField();

  auto CreateResidualHisto = [&](ETrackParType t, const char* name, const char* title) {
    if (fvbParIgnored[t]) {
      return;
    }
    TString sPrefix  = (fsTitle.Length() > 0) ? fsTitle + " point residual for " : "residual for ";
    fvphResiduals[t] = MakeQaObject<TH1F>(name, sPrefix + title, fvRBins[t], fvRLo[t], fvRUp[t]);
  };

  auto CreatePullHisto = [&](ETrackParType t, const char* name, const char* title) {
    if (fvbParIgnored[t]) {
      return;
    }
    TString sPrefix = (fsTitle.Length() > 0) ? fsTitle + " point pull for " : "pull for ";
    fvphPulls[t]    = MakeQaObject<TH1F>(name, sPrefix + title, fvPBins[t], fvPLo[t], fvPUp[t]);
  };

  CreateResidualHisto(ETrackParType::kX, "res_x", "x-coordinate;x^{reco} - x^{MC} [cm]");
  CreateResidualHisto(ETrackParType::kY, "res_y", "y-coordinate;y^{reco} - y^{MC} [cm]");
  CreateResidualHisto(ETrackParType::kTX, "res_tx", "slope along x-axis;T_{x}^{reco} - T_{x}^{MC}");
  CreateResidualHisto(ETrackParType::kTY, "res_ty", "slope along y-axis;T_{y}^{reco} - T_{y}^{MC}");
  CreateResidualHisto(ETrackParType::kQP, "res_P", "momentum; (p^{reco} - p^{MC})/p^{MC} []");
  CreateResidualHisto(ETrackParType::kTIME, "res_t", "time;t^{reco} - t^{MC} [ns]");
  CreateResidualHisto(ETrackParType::kVI, "res_vi", "inverse speed;v^{-1}_{reco} - v^{-1}_{MC} [c^{-1}]");

  CreatePullHisto(ETrackParType::kX, "pull_x", "x-coordinate;(x^{reco} - x^{MC})/#sigma_{x}");
  CreatePullHisto(ETrackParType::kY, "pull_y", "y-coordinate;(y^{reco} - y^{MC})/#sigma_{y}");
  CreatePullHisto(ETrackParType::kTX, "pull_tx", "slope along x-axis;(T_{x}^{reco} - T_{x}^{MC})/#sigma_{T_{x}}");
  CreatePullHisto(ETrackParType::kTY, "pull_ty", "slope along y-axis;(T_{y}^{reco} - T_{y}^{MC})/#sigma_{T_{y}}");
  CreatePullHisto(ETrackParType::kQP, "pull_qp", "charge over mom.;((q/p)^{reco} - (q/p)^{MC})/#sigma_{q/p}");
  CreatePullHisto(ETrackParType::kTIME, "pull_t", "time;(t^{reco} - t^{MC})/#sigma_{t}");
  CreatePullHisto(ETrackParType::kVI, "pull_vi", "inverse speed;(v^{-1}_{reco} - v^{-1}_{MC})/#sigma_{v^{-1}}");

  // FIXME: Replace hardcoded parameters with variables
  fph_res_p_pMC         = MakeQaObject<TProfile>("res_p_vs_pMC", "", 100, 0.0, 10.0, -2., 2.);
  fph_res_phi_phiMC     = MakeQaObject<TProfile>("res_phi_vs_phiMC", "", 100, -3.2, 3.2, -2., 2.);
  fph_res_theta_thetaMC = MakeQaObject<TProfile>("res_theta_vs_phiMC", "", 100, 0., 3.2, -2., 2.);

  // Set histogram titles
  TString sPrefix = (fsTitle.Length() > 0) ? fsTitle + " point " : "";
  fph_res_p_pMC->SetTitle(sPrefix + " resolution of momentum;p^{MC} [GeV/c];#delta p [GeV/c]");
  fph_res_phi_phiMC->SetTitle(sPrefix + " resolution of polar angle;#phi^{MC} [rad];#delta#phi [rad]");
  fph_res_theta_thetaMC->SetTitle(sPrefix + " resolution of polar angle;#theta^{MC} [rad];#delta#theta [rad]");
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackFitQa::Fill(const TrackParamV& trPar, const tools::MCPoint& mcPoint, bool bTimeMeasured, double /*weight*/)
{
  // Probably, a bottleneck
  cbm::algo::kf::TrackKalmanFilter<ca::fvec> fitter;
  fitter.SetParticleMass(fMass);
  fitter.SetMask(fmask::One());
  fitter.SetDoFitVelocity(true);
  fitter.SetTrack(trPar);
  cbm::algo::kf::FieldRegion<ca::fvec> fieldRegion(kf::GlobalField::fgOriginalFieldType,
                                                   kf::GlobalField::fgOriginalField);
  fitter.Extrapolate(mcPoint.GetZ(), fieldRegion);

  const TrackParamV& trParExtr = fitter.Tr();  // Track parameters extrapolated to given MC point

  // ** Time-independent measurements **
  FillResAndPull(ETrackParType::kX, trParExtr.GetX()[0], trParExtr.GetXError()[0], mcPoint.GetX());
  FillResAndPull(ETrackParType::kY, trParExtr.GetY()[0], trParExtr.GetYError()[0], mcPoint.GetY());
  FillResAndPull(ETrackParType::kTX, trParExtr.GetTx()[0], trParExtr.GetTxError()[0], mcPoint.GetTx());
  FillResAndPull(ETrackParType::kTY, trParExtr.GetTy()[0], trParExtr.GetTyError()[0], mcPoint.GetTy());
  FillResAndPull(ETrackParType::kQP, trParExtr.GetQp()[0], trParExtr.GetQpError()[0], mcPoint.GetQp());

  // Momentum resolution
  double recoP = std::fabs(mcPoint.GetCharge() / trParExtr.GetQp()[0]);  // reco mom. (with MC charge)
  double resP  = recoP - mcPoint.GetP();                                 // residual of total momentum

  // Phi resolution
  double mcPhi  = mcPoint.GetPhi();
  double recoTx = trParExtr.Tx()[0];
  double recoTy = trParExtr.Ty()[0];
  double resPhi = atan2(recoTx * cos(mcPhi) + recoTy * sin(mcPhi), -recoTx * cos(mcPhi) + recoTy * sin(mcPhi));

  // Theta resolution
  double resTheta = trParExtr.GetTheta()[0] - mcPoint.GetTheta();  // residual of polar angle

  resPhi = std::atan2(std::sin(resPhi), std::cos(resPhi));  // overflow over (-pi, pi] protection

  fph_res_p_pMC->Fill(mcPoint.GetP(), resP);
  fph_res_phi_phiMC->Fill(mcPoint.GetPhi(), resPhi);
  fph_res_theta_thetaMC->Fill(mcPoint.GetTheta(), resTheta);

  // ** Time-dependent measurements **
  if (bTimeMeasured) {
    FillResAndPull(ETrackParType::kTIME, trParExtr.GetTime()[0], trParExtr.GetTimeError()[0], mcPoint.GetTime());
    FillResAndPull(ETrackParType::kVI, trParExtr.GetVi()[0], trParExtr.GetViError()[0], mcPoint.GetInvSpeed());
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackFitQa::SetDefaultProperties()
{
  // ** Residual distribution parameters **
  fvRBins[ETrackParType::kX]    = 200;     ///< Number of bins, residual of x
  fvRLo[ETrackParType::kX]      = -4.e-3;  ///< Lower boundary, residual of x [cm]
  fvRUp[ETrackParType::kX]      = +4.e-3;  ///< Upper boundary, residual of x [cm]
  fvRBins[ETrackParType::kY]    = 200;     ///< Number of bins, residual of y
  fvRLo[ETrackParType::kY]      = -4.e-2;  ///< Lower boundary, residual of y [cm]
  fvRUp[ETrackParType::kY]      = +4.e-2;  ///< Upper boundary, residual of y [cm]
  fvRBins[ETrackParType::kTX]   = 200;     ///< Number of bins, residual of slope along x-axis
  fvRLo[ETrackParType::kTX]     = -4.e-3;  ///< Lower boundary, residual of slope along x-axis
  fvRUp[ETrackParType::kTX]     = +4.e-3;  ///< Upper boundary, residual of slope along x-axis
  fvRBins[ETrackParType::kTY]   = 200;     ///< Number of bins, residual of slope along y-axis
  fvRLo[ETrackParType::kTY]     = -4.e-3;  ///< Lower boundary, residual of slope along y-axis
  fvRUp[ETrackParType::kTY]     = +4.e-3;  ///< Upper boundary, residual of slope along y-axis
  fvRBins[ETrackParType::kQP]   = 200;     ///< Number of bins, residual of q/p
  fvRLo[ETrackParType::kQP]     = -.3;     ///< Lower boundary, residual of q/p [ec/GeV]
  fvRUp[ETrackParType::kQP]     = +.3;     ///< Upper boundary, residual of q/p [ec/GeV]
  fvRBins[ETrackParType::kTIME] = 200;     ///< Number of bins, residual of time
  fvRLo[ETrackParType::kTIME]   = -10.;    ///< Lower boundary, residual of time [ns]
  fvRUp[ETrackParType::kTIME]   = +10.;    ///< Upper boundary, residual of time [ns]
  fvRBins[ETrackParType::kVI]   = 200;     ///< Number of bins, residual of inverse speed
  fvRLo[ETrackParType::kVI]     = -2.;     ///< Lower boundary, residual of inverse speed [1/c]
  fvRUp[ETrackParType::kVI]     = +2.;     ///< Upper boundary, residual of inverse speed [1/c]

  // ** Pulls distribution parameters **
  fvPBins[ETrackParType::kX]    = 200;   ///< Number of bins, pull of x
  fvPLo[ETrackParType::kX]      = -10.;  ///< Lower boundary, pull of x [cm]
  fvPUp[ETrackParType::kX]      = +10.;  ///< Upper boundary, pull of x [cm]
  fvPBins[ETrackParType::kY]    = 200;   ///< Number of bins, pull of y
  fvPLo[ETrackParType::kY]      = -10.;  ///< Lower boundary, pull of y [cm]
  fvPUp[ETrackParType::kY]      = +10.;  ///< Upper boundary, pull of y [cm]
  fvPBins[ETrackParType::kTX]   = 200;   ///< Number of bins, pull of slope along x-axis
  fvPLo[ETrackParType::kTX]     = -10.;  ///< Lower boundary, pull of slope along x-axis
  fvPUp[ETrackParType::kTX]     = +10.;  ///< Upper boundary, pull of slope along x-axis
  fvPBins[ETrackParType::kTY]   = 200;   ///< Number of bins, pull of slope along y-axis
  fvPLo[ETrackParType::kTY]     = -10.;  ///< Lower boundary, pull of slope along y-axis
  fvPUp[ETrackParType::kTY]     = +10.;  ///< Upper boundary, pull of slope along y-axis
  fvPBins[ETrackParType::kQP]   = 200;   ///< Number of bins, pull of q/p
  fvPLo[ETrackParType::kQP]     = -10.;  ///< Lower boundary, pull of q/p [ec/GeV]
  fvPUp[ETrackParType::kQP]     = +10.;  ///< Upper boundary, pull of q/p [ec/GeV]
  fvPBins[ETrackParType::kTIME] = 200;   ///< Number of bins, pull of time
  fvPLo[ETrackParType::kTIME]   = -10.;  ///< Lower boundary, pull of time [ns]
  fvPUp[ETrackParType::kTIME]   = +10.;  ///< Upper boundary, pull of time [ns]
  fvPBins[ETrackParType::kVI]   = 200;   ///< Number of bins, pull of inverse speed
  fvPLo[ETrackParType::kVI]     = -10.;  ///< Lower boundary, pull of inverse speed [1/c]
  fvPUp[ETrackParType::kVI]     = +10.;  ///< Upper boundary, pull of inverse speed [1/c]
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackFitQa::SetResidualHistoProperties(ETrackParType type, int nBins, double lo, double up)
{
  fvRBins[type] = nBins;
  fvRLo[type]   = lo;
  fvRUp[type]   = up;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackFitQa::SetPullHistoProperties(ETrackParType type, int nBins, double lo, double up)
{
  fvPBins[type] = nBins;
  fvPLo[type]   = lo;
  fvPUp[type]   = up;
}
