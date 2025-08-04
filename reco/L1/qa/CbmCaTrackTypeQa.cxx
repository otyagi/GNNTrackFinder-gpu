/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaTrackTypeQa.h
/// @brief  QA submodule for different track types (header)
/// @since  27.03.2023
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "CbmCaTrackTypeQa.h"

#include "CaToolsMCData.h"
#include "CbmCaTrackFitQa.h"
#include "CbmL1Track.h"

using cbm::algo::kf::FieldRegion;
using cbm::ca::TrackTypeQa;
using cbm::ca::tools::MCPoint;
using cbm::ca::tools::MCTrack;

using namespace cbm::algo;

// ---------------------------------------------------------------------------------------------------------------------
//
TrackTypeQa::TrackTypeQa(const char* typeName, const char* prefix, bool bUseMC, std::shared_ptr<ObjList_t> pObjList)
  : CbmQaIO(Form("%s_%s", prefix, typeName), pObjList)
  , fbUseMC(bUseMC)
{
  fStoringMode = EStoringMode::kSAMEDIR;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackTypeQa::Init()
{
  // TODO: Replace assertions with exceptions
  assert(fpvRecoTracks);
  assert(fpvHits);
  assert(!fbUseMC || fpMCData);

  //
  // ** Distributions of reconstructed tracks vs. reconstructed quantities **
  //
  fph_reco_p     = MakeQaObject<TH1F>("reco_p", "", kBinsP, kLoP, kUpP);
  fph_reco_pt    = MakeQaObject<TH1F>("reco_pt", "", kBinsPT, kLoPT, kUpPT);
  fph_reco_phi   = MakeQaObject<TH1F>("reco_phi", "", kBinsPHI, kLoPHI, kUpPHI);
  fph_reco_theta = MakeQaObject<TH1F>("reco_theta", "", kBinsTHETA, kLoTHETA, kUpTHETA);
  fph_reco_theta_phi =
    MakeQaObject<TH2F>("reco_theta_phi", "", kBinsPHI, kLoPHI, kUpPHI, kBinsTHETA, kLoTHETA, kUpTHETA);
  fph_reco_tx            = MakeQaObject<TH1F>("reco_tx", "", kBinsTX, kLoTX, kUpTX);
  fph_reco_ty            = MakeQaObject<TH1F>("reco_ty", "", kBinsTY, kLoTY, kUpTY);
  fph_reco_ty_tx         = MakeQaObject<TH2F>("reco_ty_tx", "", kBinsTX, kLoTX, kUpTX, kBinsTY, kLoTY, kUpTY);
  fph_reco_eta           = MakeQaObject<TH1F>("reco_eta", "", kBinsETA, kLoETA, kUpETA);
  fph_reco_fhitR         = MakeQaObject<TH1F>("reco_fhitR", "", kBinsFHITR, kLoFHITR, kUpFHITR);
  fph_reco_nhits         = MakeQaObject<TH1F>("reco_nhits", "", kBinsNHITS, kLoNHITS, kUpNHITS);
  fph_reco_fsta          = MakeQaObject<TH1F>("reco_fsta", "", kBinsNSTA, kLoNSTA, kUpNSTA);
  fph_reco_lsta          = MakeQaObject<TH1F>("reco_lsta", "", kBinsNSTA, kLoNSTA, kUpNSTA);
  fph_reco_chi2_ndf      = MakeQaObject<TH1F>("reco_chi2_ndf", "", kBinsCHI2NDF, kLoCHI2NDF, kUpCHI2NDF);
  fph_reco_chi2_ndf_time = MakeQaObject<TH1F>("reco_chi2_ndf_time", "", kBinsCHI2NDF, kLoCHI2NDF, kUpCHI2NDF);

  fph_reco_p->SetTitle("Total momentum of reconstructed track;p^{reco} [GeV/c];Counts");
  fph_reco_pt->SetTitle("Transverse momentum of reconstructed track;p_{T}^{reco} [GeV/c];Counts");
  fph_reco_phi->SetTitle("Azimuthal angle of reconstructed track;#phi^{reco} [rad];Counts");
  fph_reco_theta->SetTitle("Polar angle of reconstructed track;#theta^{reco} [rad];Counts");
  fph_reco_theta_phi->SetTitle(
    "Polar angle vs. azimuthal angle of reconstructed track;#phi^{reco} [rad];#theta^{reco} [rad];Counts");
  fph_reco_tx->SetTitle("Slope along x-axis of reconstructed tracks;t_{x}^{reco};Counts");
  fph_reco_ty->SetTitle("Slope along y-axis of reconstructed tracks;t_{y}^{reco};Counts");
  fph_reco_ty_tx->SetTitle("Slope along y-axis vs. x-axis of reconstructed tracks;t_{x}^{reco};t_{y}^{reco};");
  fph_reco_eta->SetTitle("Pseudorapidity of reconstructed track;#eta^{reco};Counts");
  fph_reco_fhitR->SetTitle("Distance of the first hit from z-axis for reconstructed tracks;R^{reco} [cm];Counts");
  fph_reco_nhits->SetTitle("Number of hits of reconstructed tracks;N_{hits};Counts");
  fph_reco_fsta->SetTitle("First station index of reconstructed tracks;ID_{station};Counts");
  fph_reco_lsta->SetTitle("Last station index of reconstructed tracks;ID_{station};Counts");
  fph_reco_chi2_ndf->SetTitle("#chi^{2}/NDF of reconstructed tracks;#chi^{2}/NDF;Counts");
  fph_reco_chi2_ndf_time->SetTitle("Time #chi^{2}/NDF of reconstructed tracks;#chi^{2}/NDF;Counts");

  fCounterRecoTotal = 0;

  if (fbUseMC) {
    //
    // ** Distributions of reconstructed tracks vs. MC quantities **
    //
    fph_reco_pMC      = MakeQaObject<TH1F>("reco_pMC", "", kBinsP, kLoP, kUpP);
    fph_reco_ptMC     = MakeQaObject<TH1F>("reco_ptMC", "", kBinsPT, kLoPT, kUpPT);
    fph_reco_yMC      = MakeQaObject<TH1F>("reco_yMC", "", kBinsY, kLoY, kUpY);
    fph_reco_etaMC    = MakeQaObject<TH1F>("reco_etaMC", "", kBinsETA, kLoETA, kUpETA);
    fph_reco_ptMC_yMC = MakeQaObject<TH2F>("reco_ptMC_yMC", "", kBinsY, kLoY, kUpY, kBinsPT, kLoPT, kUpPT);
    fph_reco_phiMC    = MakeQaObject<TH1F>("reco_phiMC", "", kBinsPHI, kLoPHI, kUpPHI);
    fph_reco_thetaMC  = MakeQaObject<TH1F>("reco_thetaMC", "", kBinsTHETA, kLoTHETA, kUpTHETA);
    fph_reco_thetaMC_phiMC =
      MakeQaObject<TH2F>("reco_thetaMC_phiMC", "", kBinsPHI, kLoPHI, kUpPHI, kBinsTHETA, kLoTHETA, kUpTHETA);
    fph_reco_txMC = MakeQaObject<TH1F>("reco_txMC", "", kBinsTX, kLoTX, kUpTX);
    fph_reco_tyMC = MakeQaObject<TH1F>("reco_tyMC", "", kBinsTY, kLoTY, kUpTY);

    fph_reco_pMC->SetTitle("MC total momentum of reconstructed track;p^{MC} [GeV/c];Counts");
    fph_reco_ptMC->SetTitle("MC transverse momentum of reconstructed track;p_{T}^{MC} [GeV/c];Counts");
    fph_reco_yMC->SetTitle("MC rapidity of reconstructed track;y^{MC};Counts");
    fph_reco_etaMC->SetTitle("MC pseudorapidity of reconstructed track;#eta^{MC};Counts");
    fph_reco_ptMC_yMC->SetTitle("MC Transverse momentum of reconstructed track;y^{MC};p_{T}^{MC} [GeV/c];Counts");
    fph_reco_phiMC->SetTitle("MC Azimuthal angle of reconstructed track;#phi^{MC} [rad];Counts");
    fph_reco_thetaMC->SetTitle("MC Polar angle of reconstructed track;#theta^{MC} [rad];Counts");
    fph_reco_thetaMC_phiMC->SetTitle(
      "MC Polar angle vs. MC azimuthal angle of reconstructed track;#phi^{MC} [rad];#theta^{MC} [rad];Counts");
    fph_reco_txMC->SetTitle("MC Slope along x-axis of reconstructed tracks;t_{x}^{MC};Counts");
    fph_reco_tyMC->SetTitle("MC Slope along y-axis of reconstructed tracks;t_{y}^{MC};Counts");


    //
    // ** Distributions of MC tracks **
    //
    fph_mc_pMC      = MakeQaObject<TH1F>("mc_pMC", "", kBinsP, kLoP, kUpP);
    fph_mc_etaMC    = MakeQaObject<TH1F>("mc_etaMC", "", kBinsETA, kLoETA, kUpETA);
    fph_mc_yMC      = MakeQaObject<TH1F>("mc_yMC", "", kBinsY, kLoY, kUpY);
    fph_mc_ptMC_yMC = MakeQaObject<TH2F>("mc_ptMC_yMC", "", kBinsY, kLoY, kUpY, kBinsPT, kLoPT, kUpPT);
    fph_mc_ptMC     = MakeQaObject<TH1F>("mc_ptMC", "", kBinsPT, kLoPT, kUpPT);
    fph_mc_phiMC    = MakeQaObject<TH1F>("mc_phiMC", "", kBinsPHI, kLoPHI, kUpPHI);
    fph_mc_thetaMC  = MakeQaObject<TH1F>("mc_thetaMC", "", kBinsTHETA, kLoTHETA, kUpTHETA);
    fph_mc_thetaMC_phiMC =
      MakeQaObject<TH2F>("mc_thetaMC_phiMC", "", kBinsPHI, kLoPHI, kUpPHI, kBinsTHETA, kLoTHETA, kUpTHETA);
    fph_mc_txMC      = MakeQaObject<TH1F>("mc_txMC", "", kBinsTX, kLoTX, kUpTX);
    fph_mc_tyMC      = MakeQaObject<TH1F>("mc_tyMC", "", kBinsTY, kLoTY, kUpTY);
    fph_mc_tyMC_txMC = MakeQaObject<TH2F>("mc_tyMC_txMC", "", kBinsTX, kLoTX, kUpTX, kBinsTY, kLoTY, kUpTY);

    fph_mc_pMC->SetTitle("Total momentum of MC tracks;p^{MC} [GeV/c];Counts");
    fph_mc_etaMC->SetTitle("Pseudorapidity of MC tracks;#eta^{MC};Counts");
    fph_mc_yMC->SetTitle("Rapidity of MC tracks;y^{MC};Counts");
    fph_mc_ptMC_yMC->SetTitle("Transverse momentum vs. rapidity of MC tracks;y^{MC};p_{T}^{MC} [GeV/c];Counts");
    fph_mc_ptMC->SetTitle("Transverse momentum of MC track;p_{T}^{MC} [GeV/c];Counts");
    fph_mc_phiMC->SetTitle("Azimuthal angle of MC track;#phi^{MC} [rad];Counts");
    fph_mc_thetaMC->SetTitle("Polar angle of MC track;#theta^{MC} [rad];Counts");
    fph_mc_thetaMC_phiMC->SetTitle("Polar angle vs. azimuthal angle of MC track;#phi^{MC} [rad];#theta^{MC} [rad]");
    fph_mc_txMC->SetTitle("Slope along x-axis of MC tracks;t_{x}^{MC};Counts");
    fph_mc_tyMC->SetTitle("Slope along y-axis of MC tracks;t_{y}^{MC};Counts");
    fph_mc_tyMC_txMC->SetTitle("Slope along y-axis vs. x-axis of MC tracks;t_{x}^{MC};t_{y}^{MC};");

    //
    // ** Efficiencies **
    //
    fph_eff_int     = MakeQaObject<TProfile>("eff_int", "", 1, -0.5, 0.5, 0., 1.);
    fph_eff_pMC     = MakeQaObject<TProfile>("eff_pMC", "", kBinsP, kLoP, kUpP, 0., 1.);
    fph_eff_yMC     = MakeQaObject<TProfile>("eff_yMC", "", kBinsY, kLoY, kUpY, 0., 1.);
    fph_eff_ptMC    = MakeQaObject<TProfile>("eff_ptMC", "", kBinsPT, kLoPT, kUpPT, 0., 1.);
    fph_eff_thetaMC = MakeQaObject<TProfile>("eff_thetaMC", "", kBinsTHETA, kLoTHETA, kUpTHETA, 0., 1.);
    fph_eff_etaMC   = MakeQaObject<TProfile>("eff_etaMC", "", kBinsTHETA, kLoTHETA, kUpTHETA, 0., 1.);
    fph_eff_phiMC   = MakeQaObject<TProfile>("eff_phiMC", "", kBinsPHI, kLoPHI, kUpPHI, 0., 1.);
    fph_eff_nhitsMC = MakeQaObject<TProfile>("eff_nhitsMC", "", kBinsNSTA, kLoNSTA, kUpNSTA, 0., 1.);
    fph_eff_txMC    = MakeQaObject<TProfile>("eff_txMC", "", kBinsTX, kLoTX, kUpTX, 0., 1.);
    fph_eff_tyMC    = MakeQaObject<TProfile>("eff_tyMC", "", kBinsTY, kLoTY, kUpTX, 0., 1.);

    // clones

    fph_clone_pMC     = MakeQaObject<TProfile>("clone_pMC", "", kBinsP, kLoP, kUpP, 0., 1.);
    fph_clone_yMC     = MakeQaObject<TProfile>("clone_yMC", "", kBinsY, kLoY, kUpY, 0., 1.);
    fph_clone_ptMC    = MakeQaObject<TProfile>("clone_ptMC", "", kBinsPT, kLoPT, kUpPT, 0., 1.);
    fph_clone_thetaMC = MakeQaObject<TProfile>("clone_thetaMC", "", kBinsTHETA, kLoTHETA, kUpTHETA, 0., 1.);
    fph_clone_etaMC   = MakeQaObject<TProfile>("clone_etaMC", "", kBinsTHETA, kLoTHETA, kUpTHETA, 0., 1.);
    fph_clone_phiMC   = MakeQaObject<TProfile>("clone_phiMC", "", kBinsPHI, kLoPHI, kUpPHI, 0., 1.);
    fph_clone_nhitsMC = MakeQaObject<TProfile>("clone_nhitsMC", "", kBinsNSTA, kLoNSTA, kUpNSTA, 0., 1.);
    fph_clone_txMC    = MakeQaObject<TProfile>("clone_txMC", "", kBinsTX, kLoTX, kUpTX, 0., 1.);
    fph_clone_tyMC    = MakeQaObject<TProfile>("clone_tyMC", "", kBinsTY, kLoTY, kUpTX, 0., 1.);

    //

    fph_rate_reco   = MakeQaObject<TProfile>("rate_reco", "", 1, -0.5, 0.5, 0., 1.);
    fph_rate_killed = MakeQaObject<TProfile>("rate_killed", "", 1, -0.5, 0.5, 0., 1.);
    fph_rate_clones = MakeQaObject<TProfile>("rate_clones", "", 1, -0.5, 0.5, 0., 1.);

    double nStations   = fpParameters->GetNstationsActive();
    fph_stations_hit   = MakeQaObject<TProfile>("rate_stations_hit", "", 1, -0.5, 0.5, 0., nStations);
    fph_stations_point = MakeQaObject<TProfile>("rate_stations_point", "", 1, -0.5, 0.5, 0., nStations);

    fCounterMC     = 0;
    fCounterClones = 0;
    fRecoLength    = 0.;
    fFakeLength    = 0.;

    fph_eff_int->SetTitle("Integrated efficiency;;#epsilon_{CA}");
    fph_eff_pMC->SetTitle("Efficiency vs. MC total momentum;p_{MC} [GeV/c];#epsilon_{CA}");
    fph_eff_yMC->SetTitle("Efficiency vs. MC rapidity;y_{MC};#epsilon");
    fph_eff_ptMC->SetTitle("Efficiency vs. MC transverse momentum;p_{T}^{MC} [GeV/c];#epsilon_{CA}");
    fph_eff_thetaMC->SetTitle("Efficiency vs. MC polar angle;#theta^{MC};#epsilon_{CA}");
    fph_eff_etaMC->SetTitle("Efficiency vs. MC pseudorapidity;#eta^{MC};#epsilon_{CA}");
    fph_eff_phiMC->SetTitle("Efficiency vs. MC azimuthal angle;#phi^{MC};#epsilon_{CA}");
    fph_eff_nhitsMC->SetTitle("Efficiency vs. number of hits;N_{hit}^{MC};#epsilon_{CA}");
    fph_eff_txMC->SetTitle("Efficiency vs. MC slope along x-axis;t_{x}^{MC};#epsilon_{CA}");
    fph_eff_tyMC->SetTitle("Efficiency vs. MC slope along y-axis;t_{y}^{MC};#epsilon_{CA}");

    fph_eff_ptMC_yMC = MakeQaObject<TProfile2D>("eff_ptMC_yMC", "", kBinsY, kLoY, kUpY, kBinsPT, kLoPT, kUpPT, 0., 1.);
    fph_eff_thetaMC_phiMC =
      MakeQaObject<TProfile2D>("eff_theta_phi", "", kBinsPHI, kLoPHI, kUpPHI, kBinsTHETA, kLoTHETA, kUpTHETA, 0., 1.);

    fph_eff_ptMC_yMC->SetTitle(
      "Efficiency vs. MC transverse momentum and MC rapidity;y^{MC};p_{T}^{MC} [GeV/c];#epsilon_{CA}");
    fph_eff_thetaMC_phiMC->SetTitle(
      "Efficiency vs. MC polar and MC azimuthal angles;#phi^{MC} [rad];#theta^{MC} [rad];#epsilon_{CA}");

    fph_clone_pMC->SetTitle("Clone rate vs. MC total momentum;p_{MC} [GeV/c];clone rate_{CA}");
    fph_clone_yMC->SetTitle("Clone vs. MC rapidity;y_{MC};clone rate_{CA}");
    fph_clone_ptMC->SetTitle("Clone vs. MC transverse momentum;p_{T}^{MC} [GeV/c];clone rate_{CA}");
    fph_clone_thetaMC->SetTitle("Clone vs. MC polar angle;#theta^{MC};clone rate_{CA}");
    fph_clone_etaMC->SetTitle("Clone vs. MC pseudorapidity;#eta^{MC};clone rate_{CA}");
    fph_clone_phiMC->SetTitle("Clone vs. MC azimuthal angle;#phi^{MC};clone rate_{CA}");
    fph_clone_nhitsMC->SetTitle("Clone vs. number of hits;N_{hit}^{MC};clone rate_{CA}");
    fph_clone_txMC->SetTitle("Clone vs. MC slope along x-axis;t_{x}^{MC};clone rate_{CA}");
    fph_clone_tyMC->SetTitle("Clone vs. MC slope along y-axis;t_{y}^{MC};clone rate_{CA}");

    fph_clone_ptMC_yMC =
      MakeQaObject<TProfile2D>("clone_ptMC_yMC", "", kBinsY, kLoY, kUpY, kBinsPT, kLoPT, kUpPT, 0., 1.);
    fph_clone_thetaMC_phiMC =
      MakeQaObject<TProfile2D>("clone_theta_phi", "", kBinsPHI, kLoPHI, kUpPHI, kBinsTHETA, kLoTHETA, kUpTHETA, 0., 1.);

    fph_clone_ptMC_yMC->SetTitle(
      "Clone vs. MC transverse momentum and MC rapidity;y^{MC};p_{T}^{MC} [GeV/c];clone rate_{CA}");
    fph_clone_thetaMC_phiMC->SetTitle(
      "Clone vs. MC polar and MC azimuthal angles;#phi^{MC} [rad];#theta^{MC} [rad];clone rate_{CA}");


    //
    // ** Track fit parameter properties (residuals and pulls) **
    //
    fpFitQaFirstHit = std::make_unique<TrackFitQa>("fst_hit", fsPrefix, fpvObjList);
    fpFitQaFirstHit->SetRootFolderName(fsRootFolderName + "/fst_hit");
    fpFitQaFirstHit->SetTitle("First hit");
    fpFitQaFirstHit->Init();

    fpFitQaLastHit = std::make_unique<TrackFitQa>("lst_hit", fsPrefix, fpvObjList);
    fpFitQaLastHit->SetRootFolderName(fsRootFolderName + "/lst_hit");
    fpFitQaLastHit->SetTitle("Last hit");
    fpFitQaLastHit->SetResidualHistoProperties(ETrackParType::kX, 200, -0.4, +0.4);
    fpFitQaLastHit->SetResidualHistoProperties(ETrackParType::kY, 200, -0.8, +0.8);
    fpFitQaLastHit->Init();

    fpFitQaVertex = std::make_unique<TrackFitQa>("vertex", fsPrefix, fpvObjList);
    fpFitQaVertex->SetRootFolderName(fsRootFolderName + "/vertex");
    fpFitQaVertex->SetTitle("Vertex");
    fpFitQaVertex->Init();
  }

  // Track fitter
  fTrackFit.SetDoFitVelocity(true);  // TODO: set flag to the configuration
  fTrackFit.SetMask(true);
  fFieldRegion = kf::FieldRegion<double>(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackTypeQa::FillRecoTrack(int iTrkReco, double weight)
{
  const auto& recoTrack = (*fpvRecoTracks)[iTrkReco];
  const auto& fstHit    = (*fpvHits)[recoTrack.GetFirstHitIndex()];
  const auto& lstHit    = (*fpvHits)[recoTrack.GetLastHitIndex()];

  fph_reco_p->Fill(recoTrack.GetP(), weight);
  fph_reco_pt->Fill(recoTrack.GetPt(), weight);
  fph_reco_eta->Fill(recoTrack.GetEta(), weight);
  fph_reco_phi->Fill(recoTrack.GetPhi(), weight);
  fph_reco_theta->Fill(recoTrack.GetTheta(), weight);
  fph_reco_theta_phi->Fill(recoTrack.GetPhi(), recoTrack.GetTheta(), weight);
  fph_reco_tx->Fill(recoTrack.GetTx(), weight);
  fph_reco_ty->Fill(recoTrack.GetTy(), weight);
  fph_reco_ty_tx->Fill(recoTrack.GetTx(), recoTrack.GetTy(), weight);
  fph_reco_nhits->Fill(recoTrack.GetNofHits(), weight);
  fph_reco_fhitR->Fill(fstHit.GetR());
  fph_reco_fsta->Fill(fstHit.GetStationId(), weight);
  fph_reco_lsta->Fill(lstHit.GetStationId(), weight);
  fph_reco_chi2_ndf->Fill(recoTrack.GetChiSq() / recoTrack.GetNdf());
  fph_reco_chi2_ndf_time->Fill(recoTrack.GetChiSqTime() / recoTrack.GetNdfTime());

  ++fCounterRecoTotal;
  if (fbUseMC) {
    int iTrkMC = recoTrack.GetMatchedMCTrackIndex();
    if (iTrkMC > -1) {
      /// TODO: fill mass hypothesis into CbmL1Track
      const auto& mcTrack = fpMCData->GetTrack(iTrkMC);
      fph_reco_pMC->Fill(mcTrack.GetP(), weight);
      fph_reco_ptMC->Fill(mcTrack.GetPt(), weight);
      fph_reco_yMC->Fill(mcTrack.GetRapidity(), weight);
      fph_reco_etaMC->Fill(mcTrack.GetEta(), weight);
      fph_reco_ptMC_yMC->Fill(mcTrack.GetRapidity(), mcTrack.GetPt(), weight);
      fph_reco_phiMC->Fill(mcTrack.GetPhi(), weight);
      fph_reco_thetaMC->Fill(mcTrack.GetTheta(), weight);
      fph_reco_thetaMC_phiMC->Fill(mcTrack.GetPhi(), mcTrack.GetTheta(), weight);
      fph_reco_txMC->Fill(mcTrack.GetTx(), weight);
      fph_reco_tyMC->Fill(mcTrack.GetTy(), weight);

      // *****************************
      // ** Track fit parameters QA **
      // *****************************

      int nTimeMeasurements = 0;
      for (int iH : recoTrack.GetHitIndexes()) {
        int iSt = (*fpvHits)[iH].GetStationId();
        if (fpParameters->GetStation(iSt).timeInfo) {
          ++nTimeMeasurements;
        }
      }
      bool isTimeFitted = (nTimeMeasurements > 1);

      // ** First hit **
      fTrackFit.SetParticleMass(mcTrack.GetMass());
      {
        int iHfst = recoTrack.GetFirstHitIndex();
        int iPfst = (*fpvHits)[iHfst].GetBestMcPointId();
        if (iPfst > -1) {
          const auto& mcPoint = fpMCData->GetPoint(iPfst);
          TrackParamV trPar(recoTrack);
          fpFitQaFirstHit->Fill(trPar, mcPoint, isTimeFitted);
        }
      }

      // ** Last hit **
      {
        int iHlst = recoTrack.GetLastHitIndex();
        int iPlst = (*fpvHits)[iHlst].GetBestMcPointId();
        if (iPlst > -1) {
          const auto& mcPoint = fpMCData->GetPoint(iPlst);
          TrackParamV trPar(recoTrack.TLast);
          fpFitQaLastHit->Fill(trPar, mcPoint, isTimeFitted);
        }
      }

      // ** Vertex **
      {
        // Create an MC point for track vertex
        MCPoint mcTrkVertex = mcTrack.GetVertexPoint();
        kf::TrackParamD trPar(recoTrack);
        fTrackFit.SetTrack(trPar);

        // Add material effects
        int iStFst    = (*fpvHits)[recoTrack.GetFirstHitIndex()].GetStationId();
        double dZ     = mcTrkVertex.GetZ() - fpParameters->GetStation(iStFst).GetZ();
        int direction = (dZ > 0.) ? 1 : -1;
        for (int iSt = iStFst; (iSt >= 0) && (iSt < fpParameters->GetNstationsActive())
                               && (direction * (mcTrkVertex.GetZ() - fpParameters->GetStation(iSt).GetZ()) > 0);
             iSt += direction) {
          fTrackFit.Extrapolate(fpParameters->GetStation(iSt).fZ, fFieldRegion);
          auto radThick = fpParameters->GetActiveSetup().GetMaterial(iSt).GetThicknessX0(fTrackFit.Tr().GetX(),
                                                                                         fTrackFit.Tr().GetY());
          fTrackFit.MultipleScattering(radThick);
          fTrackFit.EnergyLossCorrection(radThick,
                                         (direction > 0) ? kf::FitDirection::kDownstream : kf::FitDirection::kUpstream);
        }
        fTrackFit.Extrapolate(mcTrkVertex.GetZ(), fFieldRegion);
        const TrackParamV& trParExtr = fTrackFit.Tr();
        if (mcTrkVertex.GetZ() == trParExtr.GetZ()[0]) {
          fpFitQaVertex->Fill(trParExtr, mcTrkVertex, isTimeFitted);
        }
        else {
          LOG(warn) << "iTrkReco = " << iTrkReco << ", mcTrkVertex = " << mcTrkVertex.GetZ()
                    << ", par = " << trParExtr.GetZ()[0];
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackTypeQa::FillMCTrack(int iTrkMC, double weight)
{
  assert(fbUseMC);
  const MCTrack& mcTrack = fpMCData->GetTrack(iTrkMC);

  // ** Distributions **
  fph_mc_pMC->Fill(mcTrack.GetP(), weight);
  fph_mc_etaMC->Fill(mcTrack.GetEta(), weight);
  fph_mc_yMC->Fill(mcTrack.GetRapidity(), weight);
  fph_mc_ptMC_yMC->Fill(mcTrack.GetRapidity(), mcTrack.GetPt(), weight);
  fph_mc_ptMC->Fill(mcTrack.GetPt(), weight);
  fph_mc_phiMC->Fill(mcTrack.GetPhi(), weight);
  fph_mc_thetaMC->Fill(mcTrack.GetTheta(), weight);
  fph_mc_thetaMC_phiMC->Fill(mcTrack.GetPhi(), mcTrack.GetTheta(), weight);
  fph_mc_txMC->Fill(mcTrack.GetTx(), weight);
  fph_mc_tyMC->Fill(mcTrack.GetTy(), weight);
  fph_mc_tyMC_txMC->Fill(mcTrack.GetTx(), mcTrack.GetTy(), weight);

  // ** Efficiencies **
  bool bReco           = mcTrack.IsReconstructed();
  bool bKilled         = !bReco && mcTrack.IsDisturbed();
  double nStaWithHit   = mcTrack.GetTotNofStationsWithHit();
  double nStaWithPoint = mcTrack.GetTotNofStationsWithPoint();
  int nClones          = mcTrack.GetNofClones();

  // NOTE: Weight is ignored in efficiencies
  fph_rate_reco->Fill(0., bReco);
  fph_rate_killed->Fill(0., bKilled);
  fph_rate_clones->Fill(0., nClones);
  fph_stations_hit->Fill(0., nStaWithHit);
  fph_stations_point->Fill(0., nStaWithPoint);
  fCounterClones += nClones;
  ++fCounterMC;
  if (bReco) {  // NOTE: ghost tracks are ignored (?)
    for (auto iTrkReco : mcTrack.GetRecoTrackIndexes()) {
      const auto& recoTrack = (*fpvRecoTracks)[iTrkReco];
      fRecoLength += static_cast<double>(recoTrack.GetNofHits()) * recoTrack.GetMaxPurity() / nStaWithHit;
      fFakeLength += (1. - recoTrack.GetMaxPurity());
    }
  }
  fMCLength += mcTrack.GetTotNofStationsWithPoint();

  fph_eff_pMC->Fill(mcTrack.GetP(), bReco);
  fph_eff_etaMC->Fill(mcTrack.GetEta(), bReco);
  fph_eff_yMC->Fill(mcTrack.GetRapidity(), bReco);
  fph_eff_ptMC->Fill(mcTrack.GetPt(), bReco);
  fph_eff_thetaMC->Fill(mcTrack.GetTheta(), bReco);
  fph_eff_phiMC->Fill(mcTrack.GetPhi(), bReco);
  fph_eff_nhitsMC->Fill(mcTrack.GetTotNofStationsWithHit(), bReco);
  fph_eff_txMC->Fill(mcTrack.GetTx(), bReco);
  fph_eff_tyMC->Fill(mcTrack.GetTy(), bReco);

  fph_eff_ptMC_yMC->Fill(mcTrack.GetRapidity(), mcTrack.GetPt(), bReco);
  fph_eff_thetaMC_phiMC->Fill(mcTrack.GetPhi(), mcTrack.GetTheta(), bReco);

  if (bReco) {  // clone rate is normalised to the number of reconstructed tracks
    fph_clone_pMC->Fill(mcTrack.GetP(), nClones);
    fph_clone_etaMC->Fill(mcTrack.GetEta(), nClones);
    fph_clone_yMC->Fill(mcTrack.GetRapidity(), nClones);
    fph_clone_ptMC->Fill(mcTrack.GetPt(), nClones);
    fph_clone_thetaMC->Fill(mcTrack.GetTheta(), nClones);
    fph_clone_phiMC->Fill(mcTrack.GetPhi(), nClones);
    fph_clone_nhitsMC->Fill(mcTrack.GetTotNofStationsWithHit(), nClones);
    fph_clone_txMC->Fill(mcTrack.GetTx(), nClones);
    fph_clone_tyMC->Fill(mcTrack.GetTy(), nClones);
    fph_clone_ptMC_yMC->Fill(mcTrack.GetRapidity(), mcTrack.GetPt(), nClones);
    fph_clone_thetaMC_phiMC->Fill(mcTrack.GetPhi(), mcTrack.GetTheta(), nClones);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackTypeQa::SetDrawAtt(Color_t markerCol, Style_t markerSty, Color_t lineCol, Style_t lineSty)
{
  fMarkerColor = markerCol;
  fMarkerStyle = markerSty;
  fLineColor   = (lineCol > -1) ? lineCol : markerCol;
  fLineStyle   = lineSty;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackTypeQa::SetTH1Properties(TH1* pHist) const
{
  pHist->SetStats(true);
  pHist->Sumw2();
  pHist->SetMarkerStyle(fMarkerStyle);
  pHist->SetLineStyle(fLineStyle);
  pHist->SetMarkerColor(fMarkerColor);
  pHist->SetLineColor(fLineColor);
}
