/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaOutputQa.cxx
/// @brief  Tracking output QA-task (implementation)
/// @since  24.02.2023
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "CbmCaOutputQa.h"

#include "CbmCaMCModule.h"
#include "CbmCaParametersHandler.h"
#include "FairRootManager.h"
#include "Logger.h"
#include "TAttLine.h"
#include "THStack.h"
#include "TMarker.h"
#include "TPad.h"
#include "TText.h"

using cbm::algo::ca::InitManager;
using cbm::algo::ca::Parameters;
using cbm::ca::MCModule;
using cbm::ca::OutputQa;
using cbm::ca::tools::Debugger;
using cbm::ca::tools::MCTrack;

// ---------------------------------------------------------------------------------------------------------------------
//
OutputQa::OutputQa(int verbose, bool isMCUsed, ECbmRecoMode recoMode)
  : CbmQaTask("CbmCaOutputQa", verbose, isMCUsed, recoMode)
{
  // Create TS reader
  fpTSReader = std::make_unique<TimeSliceReader>();

  // Turn on default track classes
  AddTrackType(ETrackType::kAll);
  AddTrackType(ETrackType::kGhost);
  AddTrackType(ETrackType::kPrim);
  AddTrackType(ETrackType::kPrimFast);
  AddTrackType(ETrackType::kPrimLongFast);
  AddTrackType(ETrackType::kPrimLong);

  AddTrackType(ETrackType::kSec);
  AddTrackType(ETrackType::kSecFast);
  AddTrackType(ETrackType::kPrimE);
  AddTrackType(ETrackType::kPrimPI);
  AddTrackType(ETrackType::kPrimK);
  AddTrackType(ETrackType::kPrimMU);
  AddTrackType(ETrackType::kPrimPPBAR);
  AddTrackType(ETrackType::kSecE);
  AddTrackType(ETrackType::kSecPI);
  AddTrackType(ETrackType::kSecK);
  AddTrackType(ETrackType::kSecMU);
  AddTrackType(ETrackType::kSecPPBAR);

  AddTrackType(ETrackType::kPrimPIP);
  AddTrackType(ETrackType::kPrimPIM);
  AddTrackType(ETrackType::kSecPIP);
  AddTrackType(ETrackType::kSecPIM);
  AddTrackType(ETrackType::kPrimKP);
  AddTrackType(ETrackType::kPrimKM);
  AddTrackType(ETrackType::kSecKP);
  AddTrackType(ETrackType::kSecKM);
  AddTrackType(ETrackType::kPrimP);
  AddTrackType(ETrackType::kPrimPBAR);
  AddTrackType(ETrackType::kSecP);
  AddTrackType(ETrackType::kSecPBAR);

  //AddTrackType(ETrackType::kAllE);
  //AddTrackType(ETrackType::kAllMU);
  //AddTrackType(ETrackType::kAllPI);
  //AddTrackType(ETrackType::kAllK);
  //AddTrackType(ETrackType::kAllPPBAR);

  // Init track type histograms drawing attributes
  InitDrawingAttributes();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void OutputQa::Check()
{
  // Create summary table
  if (IsMCUsed()) {
    fpMCModule->Finish();

    // TODO: Add cuts on entries from fmSummaryTableEntries
    std::vector<ETrackType> vTypesToPlot;
    int nRows = std::count_if(fmSummaryTableEntries.begin(), fmSummaryTableEntries.end(),
                              [&](const auto& f) { return fvbTrackTypeOn[f] && fvpTrackHistograms[f]->IsMCUsed(); })
                + 1;

    CbmQaTable* aTable = MakeQaObject<CbmQaTable>("summary_table", "Tracking summary table", nRows + 1, 9);
    int iRow           = 0;
    std::vector<std::string> colNames = {"Efficiency", "Killed", "Length",   "Fakes",     "Clones",
                                         "Reco/Evt",   "MC/Evt", "Nst(hit)", "Nst(point)"};
    aTable->SetNamesOfCols(colNames);
    aTable->SetColWidth(14);
    double nEvents = static_cast<double>(GetEventNumber());
    LOG(info) << "Number of events: " << GetEventNumber();
    for (auto trType : fmSummaryTableEntries) {
      if (!fvbTrackTypeOn[trType] || !fvpTrackHistograms[trType]->IsMCUsed()) {
        continue;
      }
      aTable->SetRowName(iRow, fvpTrackHistograms[trType]->GetTitle());
      aTable->SetCell(iRow, 0, fvpTrackHistograms[trType]->GetIntegratedEff());
      aTable->SetCell(iRow, 1, fvpTrackHistograms[trType]->GetKilledRate());
      aTable->SetCell(iRow, 2, fvpTrackHistograms[trType]->GetAverageRecoLength());
      aTable->SetCell(iRow, 3, fvpTrackHistograms[trType]->GetAverageFakeLength());
      aTable->SetCell(iRow, 4, fvpTrackHistograms[trType]->GetClonesRate());
      aTable->SetCell(iRow, 5, fvpTrackHistograms[trType]->GetNofRecoTracksMatched() / nEvents);
      aTable->SetCell(iRow, 6, fvpTrackHistograms[trType]->GetNofMCTracks() / nEvents);
      aTable->SetCell(iRow, 7, fvpTrackHistograms[trType]->GetAverageNofStationsWithHit());
      aTable->SetCell(iRow, 8, fvpTrackHistograms[trType]->GetAverageNofStationsWithPoint());
      ++iRow;
    }
    double nGhosts = 0.;
    if (fvpTrackHistograms[ETrackType::kGhost] && fvpTrackHistograms[ETrackType::kAll]) {
      nGhosts = fvpTrackHistograms[ETrackType::kGhost]->fph_reco_p->GetEntries();
      aTable->SetRowName(iRow, "N ghosts");
      aTable->SetCell(iRow, 0, nGhosts);
      aTable->SetRowName(iRow + 1, "Ghost rate");
      aTable->SetCell(iRow + 1, 0, nGhosts / fMonitor.GetCounterValue(EMonitorKey::kTrack));
    }
    LOG(info) << '\n' << aTable->ToString(3);
  }

  LOG(info) << '\n' << fMonitor.ToString();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void OutputQa::DrawEvent()
{
  constexpr double kZmin                         = 0.;
  constexpr double kZmax                         = 300.;
  constexpr double kXmin                         = -100.;
  constexpr double kXmax                         = +100.;
  constexpr double kYmin                         = -100.;
  constexpr double kYmax                         = +100.;
  constexpr std::array<Color_t, 11> kColorMC     = {205, 209, 213, 217, 225, 208, 213, 216, 219, 224, 227};
  constexpr std::array<Color_t, 3> kColorGhost   = {201, 202, 203};
  constexpr int kCanvX                           = 1920;
  constexpr int kCanvY                           = 1080;
  constexpr double kLMargin                      = 0.05;  // Left margin
  constexpr double kVEMargin                     = 0.15;  // Vertical margin (top + bottom)
  constexpr double kRMarginDispl                 = 0.4;
  constexpr double kVIMargin                     = 0.0001;
  constexpr Marker_t kMarkerPointWHit            = 25;
  constexpr Marker_t kMarkerPointWOHit           = 5;
  constexpr Marker_t kMarkerHitWPoint            = 24;
  constexpr Marker_t kMarkerHitWOPoint           = 28;
  constexpr double kFontSize                     = 0.035;
  constexpr Width_t kLineWidth                   = 1;
  constexpr Style_t kLineMCTrackReconstructed    = 9;
  constexpr Style_t kLineMCTrackNotReconstructed = 10;
  constexpr Style_t kLineRecoTrackGhost          = 2;
  constexpr Style_t kLineRecoTrackNotGhost       = 1;

  int iEvent  = GetEventNumber();
  auto* pCanv = MakeQaObject<CbmQaCanvas>(Form("events/event_%d", iEvent), Form("event_%d", iEvent), kCanvX, kCanvY);
  pCanv->Divide(1, 2, kVIMargin, kVIMargin);
  pCanv->cd(1);
  gPad->SetMargin(kLMargin, kRMarginDispl, kVIMargin, kVEMargin);
  gPad->SetGrid();
  auto* pHistX = new TH2F(Form("hFrameX_%d", iEvent), ";z [cm];x [cm]", 2, kZmin, kZmax, 2, kXmin, kXmax);
  pHistX->GetYaxis()->SetTitleOffset(0.6);
  pHistX->GetXaxis()->SetLabelSize(kFontSize);
  pHistX->GetYaxis()->SetLabelSize(kFontSize);
  pHistX->GetXaxis()->SetTitleSize(kFontSize);
  pHistX->GetYaxis()->SetTitleSize(kFontSize);
  pHistX->SetStats(false);
  pHistX->Draw();
  pCanv->cd(2);
  gPad->SetMargin(kLMargin, kRMarginDispl, kVEMargin, kVIMargin);
  gPad->SetGrid();
  auto* pHistY = new TH2F(Form("hFrameY_%d", iEvent), ";z [cm];y [cm]", 2, kZmin, kZmax, 2, kYmin, kYmax);
  pHistY->GetYaxis()->SetTitleOffset(0.6);
  pHistY->GetXaxis()->SetLabelSize(kFontSize);
  pHistY->GetYaxis()->SetLabelSize(kFontSize);
  pHistY->GetXaxis()->SetTitleSize(kFontSize);
  pHistY->GetYaxis()->SetTitleSize(kFontSize);
  pHistY->SetStats(false);
  pHistY->Draw();

  pCanv->cd(1);

  auto* pHeader = new TLegend(kLMargin, 1 - kVEMargin + 0.01, 0.99, 0.99);
  pHeader->SetNColumns(6);
  pHeader->SetTextSize(kFontSize);
  pHeader->SetMargin(0.1);
  pHeader->AddEntry(static_cast<TObject*>(nullptr), Form("event #%d", iEvent), "");
  pHeader->AddEntry(new TMarker(0, 0, kMarkerPointWHit), "point w/ hit", "p");
  pHeader->AddEntry(new TMarker(0, 0, kMarkerPointWOHit), "point w/o hit", "p");
  pHeader->AddEntry(new TMarker(0, 0, kMarkerHitWPoint), "hit w/ point", "p");
  pHeader->AddEntry(new TMarker(0, 0, kMarkerHitWOPoint), "hit w/o point", "p");
  pHeader->Draw("same");

  auto* pLegendReco = new TLegend(1 - kRMarginDispl, kVIMargin, 0.99, 1 - kVEMargin, "Reco tracks");
  pLegendReco->SetMargin(0.1);
  pLegendReco->SetTextSize(kFontSize);
  //pLegendReco->SetHeader("Reco Tracks", "C");
  pCanv->cd(2);
  auto* pLegendMC = new TLegend(1 - kRMarginDispl, kVEMargin, 0.99, 1 - kVIMargin, "MC tracks");
  pLegendMC->SetMargin(0.1);
  pLegendMC->SetTextSize(kFontSize);
  //pLegendMC->SetHeader("MC Tracks", "C");

  int iColorRec   = 0;
  int iColorGhost = 0;

  // Draw MC tracks
  std::map<int, Color_t> mMCtrkColors;  // Trk ID vs. color
  if (IsMCUsed()) {
    // Draw MC tracks
    int nMCTracks = fMCData.GetNofTracks();
    for (int iTmc = 0; iTmc < nMCTracks; ++iTmc) {
      const auto& trk = fMCData.GetTrack(iTmc);
      int nPoints     = trk.GetNofPoints();
      if (nPoints == 0) {
        continue;
      }
      std::vector<double> trkPointX(nPoints);
      std::vector<double> trkPointY(nPoints);
      std::vector<double> trkPointZ(nPoints);
      for (int iPLoc = 0; iPLoc < nPoints; ++iPLoc) {
        const auto& point = fMCData.GetPoint(trk.GetPointIndexes()[iPLoc]);
        trkPointX[iPLoc]  = point.GetX();
        trkPointY[iPLoc]  = point.GetY();
        trkPointZ[iPLoc]  = point.GetZ();
      }
      Color_t currColor         = 1;
      Style_t currStyle         = trk.IsReconstructed() ? kLineMCTrackReconstructed : kLineMCTrackNotReconstructed;
      currColor                 = kColorMC[iColorRec];
      iColorRec                 = (iColorRec + 1) % static_cast<int>(kColorMC.size());
      mMCtrkColors[trk.GetId()] = currColor;
      {
        pCanv->cd(1);
        auto* gr = new TGraph(nPoints, trkPointZ.data(), trkPointX.data());
        gr->SetMarkerStyle(1);
        gr->SetMarkerColor(currColor);
        gr->SetLineColor(currColor);
        gr->SetLineStyle(currStyle);
        gr->SetLineWidth(kLineWidth);
        gr->Draw("LSAME");

        std::stringstream msg;
        msg << "ID=" << trk.GetId() << ", ";
        msg << "PDG=" << trk.GetPdgCode() << ", ";
        msg << "p=" << trk.GetP() << " GeV/c, ";
        msg << "rec-able=" << trk.IsReconstructable() << ", ";
        msg << "rec-ed=" << trk.IsReconstructed() << ", ";
        if (trk.GetNofRecoTracks() > 0) {
          msg << "reco_IDs=(";
          for (int iTr : trk.GetRecoTrackIndexes()) {
            msg << iTr << ' ';
          }
          msg << "), ";
        }
        if (trk.GetNofTouchTracks() > 0) {
          msg << "touch_IDs=(";
          for (int iTr : trk.GetTouchTrackIndexes()) {
            msg << iTr << ' ';
          }
          msg << "), ";
        }
        pLegendMC->AddEntry(gr, msg.str().c_str(), "l");
      }
      {
        pCanv->cd(2);
        auto* gr = new TGraph(nPoints, trkPointZ.data(), trkPointY.data());
        gr->SetMarkerStyle(1);
        gr->SetMarkerColor(currColor);
        gr->SetLineColor(currColor);
        gr->SetLineStyle(currStyle);
        gr->SetLineWidth(kLineWidth);
        gr->Draw("LSAME");
      }
    }

    // Draw MC points
    int nPoints = fMCData.GetNofPoints();
    for (int iP = 0; iP < nPoints; ++iP) {
      const auto& point = fMCData.GetPoint(iP);
      bool bHasHit      = point.GetHitIndexes().size() > 0;
      Marker_t style    = bHasHit ? kMarkerPointWHit : kMarkerPointWOHit;
      Color_t color     = mMCtrkColors.at(point.GetTrackId());
      {
        pCanv->cd(1);
        auto* marker = new TMarker(point.GetZ(), point.GetX(), style);
        marker->SetMarkerColor(color);
        marker->Draw("same");

        auto* pText = new TText(point.GetZ() + 2., point.GetX() + 2., Form("%d", point.GetStationId()));
        pText->SetTextColor(color);
        pText->SetTextSize(kFontSize);
        pText->Draw("same");
      }
      {
        pCanv->cd(2);
        auto* marker = new TMarker(point.GetZ(), point.GetY(), style);
        marker->SetMarkerColor(color);
        marker->Draw("same");

        auto* pText = new TText(point.GetZ() + 2., point.GetY() + 2., Form("%d", point.GetStationId()));
        pText->SetTextColor(color);
        pText->SetTextSize(kFontSize);
        pText->Draw("same");
      }
    }

    // Draw reconstructed tracks
    int nRecoTracks = fvRecoTracks.size();
    std::vector<char> vbHitUsed(fvHits.size());
    std::vector<Color_t> vRecoTrkColors(fvHits.size());  // Reco hit ID vs. color
    if (nRecoTracks > 0) {
      for (int iTr = 0; iTr < nRecoTracks; ++iTr) {
        const auto& trk   = fvRecoTracks[iTr];
        Color_t currColor = 1;
        Style_t currStyle = trk.IsGhost() ? kLineRecoTrackGhost : kLineRecoTrackNotGhost;
        if (trk.IsGhost()) {
          currColor   = kColorGhost[iColorGhost];
          iColorGhost = (iColorGhost + 1) % static_cast<int>(kColorGhost.size());
        }
        else {
          int iTmc  = trk.GetMatchedMCTrackIndex();
          currColor = iTmc > -1 ? mMCtrkColors[iTmc] : 1;
        }

        int nHits = trk.GetNofHits();
        std::vector<double> trkHitX(nHits);
        std::vector<double> trkHitY(nHits);
        std::vector<double> trkHitZ(nHits);
        for (int iHLoc = 0; iHLoc < nHits; ++iHLoc) {
          int iH             = trk.GetHitIndexes()[iHLoc];
          const auto& hit    = fvHits[iH];
          vbHitUsed[iH]      = true;
          trkHitX[iHLoc]     = hit.GetX();
          trkHitY[iHLoc]     = hit.GetY();
          trkHitZ[iHLoc]     = hit.GetZ();
          vRecoTrkColors[iH] = currColor;
        }

        {
          pCanv->cd(1);
          auto* gr = new TGraph(nHits, trkHitZ.data(), trkHitX.data());
          gr->SetMarkerStyle(1);
          gr->SetMarkerColor(currColor);
          gr->SetLineColor(currColor);
          gr->SetLineStyle(currStyle);
          gr->SetLineWidth(kLineWidth + 2);
          gr->Draw("LSAME");
        }
        {
          pCanv->cd(2);
          auto* gr = new TGraph(nHits, trkHitZ.data(), trkHitY.data());
          gr->SetMarkerStyle(1);
          gr->SetMarkerColor(currColor);
          gr->SetLineColor(currColor);
          gr->SetLineStyle(currStyle);
          gr->SetLineWidth(kLineWidth + 2);
          gr->Draw("LSAME");
          std::stringstream msg;
          msg << "ID=" << trk.index << ", ";
          msg << "MC_IDs=(";
          for (int iTmc : trk.GetMCTrackIndexes()) {
            msg << iTmc << ' ';
          }
          msg << "), ";
          msg << "purity=" << trk.GetMaxPurity();
          pLegendReco->AddEntry(gr, msg.str().c_str(), "l");
        }
      }
    }
    // Draw hits
    int nHits = fvHits.size();
    if (nHits > 0) {
      for (int iH = 0; iH < nHits; ++iH) {
        const auto& hit = fvHits[iH];
        bool bFake      = hit.GetBestMcPointId() == -1;
        bool bUsed      = vbHitUsed[iH];

        Marker_t style = bFake ? kMarkerHitWOPoint : kMarkerHitWPoint;
        Color_t color  = bUsed ? vRecoTrkColors[iH] : 1;
        {
          pCanv->cd(1);
          auto* marker = new TMarker(hit.GetZ(), hit.GetX(), style);
          marker->SetMarkerColor(color);
          marker->Draw("same");
        }
        {
          pCanv->cd(2);
          auto* marker = new TMarker(hit.GetZ(), hit.GetY(), style);
          marker->SetMarkerColor(color);
          marker->Draw("same");
        }
      }
    }
  }
  pCanv->cd(1);
  pLegendReco->Draw();
  pCanv->cd(2);
  pLegendMC->Draw();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void OutputQa::ExecQa()
{
  fMonitor.IncrementCounter(EMonitorKey::kEvent);

  // Read reconstructed input
  fpTSReader->ReadEvent(this->GetCurrentEvent());
  int nHits       = fvHits.size();
  int nRecoTracks = fvRecoTracks.size();

  fMonitor.IncrementCounter(EMonitorKey::kTrack, nRecoTracks);
  fMonitor.IncrementCounter(EMonitorKey::kHit, nHits);

  // Match tracks and points
  // Read MC tracks and points
  int nMCTracks = 0;
  int nMCPoints = 0;
  if (IsMCUsed()) {
    fpMCModule->InitEvent(this->GetCurrentEvent());
    nMCPoints = fMCData.GetNofPoints();
    nMCTracks = fMCData.GetNofTracks();
    fpMCModule->MatchHits();
    fpMCModule->MatchTracks();
    fMonitor.IncrementCounter(EMonitorKey::kMcPoint, nMCPoints);
    fMonitor.IncrementCounter(EMonitorKey::kMcTrack, nMCTracks);

    if (fbDrawEvents && nMCPoints > std::max(0, fEvtDisplayMinNofPoints)) {
      DrawEvent();
    }
  }
  LOG_IF(info, fVerbose > 2) << fName << ": Data sample consists of " << nHits << " hits, " << nRecoTracks
                             << " reco tracks, " << nMCTracks << " MC tracks, " << nMCPoints << " MC points";


  // ************************************************************************
  // ** Fill distributions for reconstructed tracks (including ghost ones) **
  // ************************************************************************

  for (int iTrkReco = 0; iTrkReco < nRecoTracks; ++iTrkReco) {
    const auto& recoTrk = fvRecoTracks[iTrkReco];

    // Reject tracks, which do not contain hits
    if (recoTrk.GetNofHits() < 1) {
      continue;
    }

    FillRecoTrack(ETrackType::kAll, iTrkReco);

    if (IsMCUsed()) {
      // NOTE: The ghost status of track is now defined by its purity, thus it can still contain MC information
      if (recoTrk.IsGhost()) {
        FillRecoTrack(ETrackType::kGhost, iTrkReco);
      }

      int iTrkMC = recoTrk.GetMatchedMCTrackIndex();
      if (iTrkMC > -1) {
        const auto& mcTrk = fMCData.GetTrack(iTrkMC);
        int pdg           = mcTrk.GetPdgCode();
        bool isPrimary    = mcTrk.IsPrimary();

        // Cut tracks, which did not leave hits in tracker
        if (mcTrk.GetNofHits() == 0) {
          continue;
        }

        if (isPrimary) {
          FillRecoTrack(ETrackType::kPrim, iTrkReco);
          bool bFast = mcTrk.GetP() > CbmL1Constants::MinFastMom;
          bool bLong = mcTrk.GetTotNofStationsWithHit() == fpParameters->GetNstationsActive();
          if (bFast) {
            FillRecoTrack(ETrackType::kPrimFast, iTrkReco);
          }
          if (bLong) {
            FillRecoTrack(ETrackType::kPrimLong, iTrkReco);
          }
          if (bLong && bFast) {
            FillRecoTrack(ETrackType::kPrimLongFast, iTrkReco);
          }
        }
        else {
          FillRecoTrack(ETrackType::kSec, iTrkReco);
          if (mcTrk.GetP() > CbmL1Constants::MinFastMom) {
            FillRecoTrack(ETrackType::kSecFast, iTrkReco);
          }
        }

        // Track distributions for different particle species
        switch (std::abs(pdg)) {
          case 211:  // pion
            FillRecoTrack(ETrackType::kAllPI, iTrkReco);
            if (isPrimary) {
              FillRecoTrack(ETrackType::kPrimPI, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kPrimPIP : ETrackType::kPrimPIM, iTrkReco);
            }
            else {
              FillRecoTrack(ETrackType::kSecPI, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kSecPIP : ETrackType::kSecPIM, iTrkReco);
            }
            break;
          case 2212:  // proton
            FillRecoTrack(ETrackType::kAllPPBAR, iTrkReco);
            if (isPrimary) {
              FillRecoTrack(ETrackType::kPrimPPBAR, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kPrimP : ETrackType::kPrimPBAR, iTrkReco);
            }
            else {
              FillRecoTrack(ETrackType::kSecPPBAR, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kSecP : ETrackType::kSecPBAR, iTrkReco);
            }
            break;
          case 321:  // kaon
            FillRecoTrack(ETrackType::kAllK, iTrkReco);
            if (isPrimary) {
              FillRecoTrack(ETrackType::kPrimK, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kPrimKP : ETrackType::kPrimKM, iTrkReco);
            }
            else {
              FillRecoTrack(ETrackType::kSecK, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kSecKP : ETrackType::kSecKM, iTrkReco);
            }
            break;
          case 11:  // electron
            FillRecoTrack(ETrackType::kAllE, iTrkReco);
            if (isPrimary) {
              FillRecoTrack(ETrackType::kPrimE, iTrkReco);
              FillRecoTrack((pdg < 0) ? ETrackType::kPrimEP : ETrackType::kPrimEM, iTrkReco);
            }
            else {
              FillRecoTrack(ETrackType::kSecE, iTrkReco);
              FillRecoTrack((pdg < 0) ? ETrackType::kSecEP : ETrackType::kSecEM, iTrkReco);
            }
            break;
          case 13:  // muon
            FillRecoTrack(ETrackType::kAllMU, iTrkReco);
            if (isPrimary) {
              FillRecoTrack(ETrackType::kPrimMU, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kPrimMUP : ETrackType::kPrimMUM, iTrkReco);
            }
            else {
              FillRecoTrack(ETrackType::kSecMU, iTrkReco);
              FillRecoTrack((pdg > 0) ? ETrackType::kSecMUP : ETrackType::kSecMUM, iTrkReco);
            }
            break;
        }  // switch abs(pdg): end
      }
    }
  }  // loop over recoTrk: end


  // *************************************
  // ** Fill distributions of MC-tracks **
  // *************************************
  if (IsMCUsed()) {
    for (int iTrkMC = 0; iTrkMC < fMCData.GetNofTracks(); ++iTrkMC) {
      const auto& mcTrk = fMCData.GetTrack(iTrkMC);

      // ----- CUTS ON MC TRACKS
      // Cut tracks, which did not leave hits in tracker
      if (mcTrk.GetNofHits() == 0) {
        continue;
      }

      // Cut tracks, which cannot be reconstructed
      if (!mcTrk.IsReconstructable()) {
        continue;
      }
      int pdg        = mcTrk.GetPdgCode();
      bool isPrimary = mcTrk.IsPrimary();

      // Fill different track categories
      FillMCTrack(ETrackType::kAll, iTrkMC);
      if (isPrimary) {
        FillMCTrack(ETrackType::kPrim, iTrkMC);
        bool bFast = mcTrk.GetP() > CbmL1Constants::MinFastMom;
        bool bLong = mcTrk.GetTotNofStationsWithHit() == fpParameters->GetNstationsActive();
        if (bFast) {
          FillMCTrack(ETrackType::kPrimFast, iTrkMC);
        }
        if (bLong) {
          FillMCTrack(ETrackType::kPrimLong, iTrkMC);
        }
        if (bLong && bFast) {
          FillMCTrack(ETrackType::kPrimLongFast, iTrkMC);
        }
      }
      else {
        FillMCTrack(ETrackType::kSec, iTrkMC);
        if (mcTrk.GetP() > CbmL1Constants::MinFastMom) {
          FillMCTrack(ETrackType::kSecFast, iTrkMC);
        }
      }

      // Track distributions for different particle species
      switch (std::abs(pdg)) {
        case 211:  // pion
          FillMCTrack(ETrackType::kAllPI, iTrkMC);
          if (isPrimary) {
            FillMCTrack(ETrackType::kPrimPI, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kPrimPIP : ETrackType::kPrimPIM, iTrkMC);
          }
          else {
            FillMCTrack(ETrackType::kSecPI, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kSecPIP : ETrackType::kSecPIM, iTrkMC);
          }
          break;
        case 2212:  // proton
          FillMCTrack(ETrackType::kAllPPBAR, iTrkMC);
          if (isPrimary) {
            FillMCTrack(ETrackType::kPrimPPBAR, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kPrimP : ETrackType::kPrimPBAR, iTrkMC);
          }
          else {
            FillMCTrack(ETrackType::kSecPPBAR, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kSecP : ETrackType::kSecPBAR, iTrkMC);
          }
          break;
        case 321:  // kaon
          FillMCTrack(ETrackType::kAllK, iTrkMC);
          if (isPrimary) {
            FillMCTrack(ETrackType::kPrimK, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kPrimKP : ETrackType::kPrimKM, iTrkMC);
          }
          else {
            FillMCTrack(ETrackType::kSecK, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kSecKP : ETrackType::kSecKM, iTrkMC);
          }
          break;
        case 11:  // electron
          FillMCTrack(ETrackType::kAllE, iTrkMC);
          if (isPrimary) {
            FillMCTrack(ETrackType::kPrimE, iTrkMC);
            FillMCTrack((pdg < 0) ? ETrackType::kPrimEP : ETrackType::kPrimEM, iTrkMC);
          }
          else {
            FillMCTrack(ETrackType::kSecE, iTrkMC);
            FillMCTrack((pdg < 0) ? ETrackType::kSecEP : ETrackType::kSecEM, iTrkMC);
          }
          break;
        case 13:  // muon
          FillMCTrack(ETrackType::kAllMU, iTrkMC);
          if (isPrimary) {
            FillMCTrack(ETrackType::kPrimMU, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kPrimMUP : ETrackType::kPrimMUM, iTrkMC);
          }
          else {
            FillMCTrack(ETrackType::kSecMU, iTrkMC);
            FillMCTrack((pdg > 0) ? ETrackType::kSecMUP : ETrackType::kSecMUM, iTrkMC);
          }
          break;
      }  // switch abs(pdg): end
    }    // iTrkMC
  }      // IsMCUsed()
}

// ---------------------------------------------------------------------------------------------------------------------
//
void OutputQa::CreateSummary()
{
  /// Set of track types to compare
  std::vector<ETrackType> vCmpTypesGeneral = {kAll, kPrim, kSec};
  std::vector<ETrackType> vCmpTypesPrim    = {kPrim, kPrimE, kPrimMU, kPrimPI, kPrimK, kPrimPPBAR};
  std::vector<ETrackType> vCmpTypesSec     = {kSec, kSecE, kSecMU, kSecPI, kSecK, kSecPPBAR};
  std::vector<ETrackType> vCmpTypesPions   = {kAllPI, kPrimPIP, kPrimPIM, kSecPIP, kSecPIM};
  std::vector<ETrackType> vCmpTypesKaons   = {kAllK, kPrimKP, kPrimKM, kSecKP, kSecKM};
  std::vector<ETrackType> vCmpTypesProtons = {kAllPPBAR, kPrimP, kPrimPBAR, kSecP, kSecPBAR};

  /// @brief Function to draw generic canvas of histogram comparison
  auto DrawTrackDistributions = [&](TCanvas* pCanv, std::function<TH1F*(ETrackType)> Hist) {
    pCanv->Divide(3, 2);
    pCanv->cd(1);
    gPad->SetLogy();
    DrawSetOf<TH1F>(vCmpTypesGeneral, Hist);
    pCanv->cd(2);
    gPad->SetLogy();
    DrawSetOf<TH1F>(vCmpTypesPrim, Hist);
    pCanv->cd(3);
    gPad->SetLogy();
    DrawSetOf<TH1F>(vCmpTypesSec, Hist);
    pCanv->cd(4);
    gPad->SetLogy();
    DrawSetOf<TH1F>(vCmpTypesPions, Hist);
    pCanv->cd(5);
    gPad->SetLogy();
    DrawSetOf<TH1F>(vCmpTypesKaons, Hist);
    pCanv->cd(6);
    gPad->SetLogy();
    DrawSetOf<TH1F>(vCmpTypesProtons, Hist);
  };

  /// @brief Function to draw generic canvas of efficiencies comparison
  auto DrawTrackEfficiens = [&](TCanvas* pCanv, std::function<TProfile*(ETrackType)> Prof) {
    pCanv->Divide(3, 1);
    pCanv->cd(1);
    DrawSetOf<TProfile>(vCmpTypesGeneral, Prof);
    pCanv->cd(2);
    DrawSetOf<TProfile>(vCmpTypesPrim, Prof);
    pCanv->cd(3);
    DrawSetOf<TProfile>(vCmpTypesSec, Prof);
  };


  if (IsMCUsed()) {
    // **  Reconstructed track distributions  **
    // Reconstructed pseudorapidity
    auto* pc_reco_eta =
      MakeQaObject<TCanvas>("reco_eta", "Reconstructed track pseudorapidity", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawTrackDistributions(pc_reco_eta, [&](ETrackType t) -> TH1F* { return fvpTrackHistograms[t]->fph_reco_eta; });

    // MC pseudorapidity
    auto* pc_reco_etaMC =
      MakeQaObject<TCanvas>("reco_etaMC", "Reconstructed track MC pseudorapidity", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawTrackDistributions(pc_reco_etaMC, [&](ETrackType t) -> TH1F* { return fvpTrackHistograms[t]->fph_reco_etaMC; });

    // MC momentum
    auto* pc_reco_pMC =
      MakeQaObject<TCanvas>("reco_pMC", "Reconstructed track MC momentum", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawTrackDistributions(pc_reco_pMC, [&](ETrackType t) -> TH1F* { return fvpTrackHistograms[t]->fph_reco_pMC; });

    // MC rapidity
    auto* pc_reco_yMC =
      MakeQaObject<TCanvas>("reco_yMC", "Reconstructed track MC rapidity", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawTrackDistributions(pc_reco_yMC, [&](ETrackType t) -> TH1F* { return fvpTrackHistograms[t]->fph_reco_yMC; });

    // **  MC track distributions  **

    // MC momentum
    auto* pc_mc_pMC =
      MakeQaObject<TCanvas>("mc_pMC", "MC reconstructable track MC momentum", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawTrackDistributions(pc_mc_pMC, [&](ETrackType t) -> TH1F* { return fvpTrackHistograms[t]->fph_mc_pMC; });

    // MC rapidity
    auto* pc_mc_yMC =
      MakeQaObject<TCanvas>("mc_yMC", "MC reconstructable track MC rapidity", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawTrackDistributions(pc_mc_yMC, [&](ETrackType t) -> TH1F* { return fvpTrackHistograms[t]->fph_mc_yMC; });

    // MC rapidity vs. MC momentum
    // auto* pc_mc_pMC_yMC =
    MakeQaObject<TCanvas>("mc_ptMC_yMC", "MC track MC transverse mom. vs. rapidity ", kCXSIZEPX * 3, kCYSIZEPX * 2);
    DrawSetOf<TH2F>(vCmpTypesGeneral, [&](ETrackType t) -> TH2F* { return fvpTrackHistograms[t]->fph_reco_ptMC_yMC; });

    // **  Efficiencies  **

    // MC momentum
    auto* pc_eff_pMC = MakeQaObject<TCanvas>("eff_pMC", "Tracking Eff. vs. MC momentum", kCXSIZEPX * 3, kCYSIZEPX);
    DrawTrackEfficiens(pc_eff_pMC, [&](ETrackType t) -> TProfile* { return fvpTrackHistograms[t]->fph_eff_pMC; });

    auto* pc_eff_yMC = MakeQaObject<TCanvas>("eff_yMC", "Tracking Eff. vs. MC rapidity", kCXSIZEPX * 3, kCYSIZEPX);
    DrawTrackEfficiens(pc_eff_yMC, [&](ETrackType t) -> TProfile* { return fvpTrackHistograms[t]->fph_eff_yMC; });

    auto* pc_eff_thetaMC =
      MakeQaObject<TCanvas>("eff_thetaMC", "Tracking Eff. vs. MC polar angle", kCXSIZEPX * 3, kCYSIZEPX);
    DrawTrackEfficiens(pc_eff_thetaMC,
                       [&](ETrackType t) -> TProfile* { return fvpTrackHistograms[t]->fph_eff_thetaMC; });

    auto* pc_eff_phiMC =
      MakeQaObject<TCanvas>("eff_phiMC", "Tracking Eff. vs. MC azimuthal angle", kCXSIZEPX * 3, kCYSIZEPX);
    DrawTrackEfficiens(pc_eff_phiMC, [&](ETrackType t) -> TProfile* { return fvpTrackHistograms[t]->fph_eff_phiMC; });

    auto* pc_eff_etaMC =
      MakeQaObject<TCanvas>("eff_etaMC", "Tracking Eff. vs. MC pseudorapidity", kCXSIZEPX * 3, kCYSIZEPX);
    DrawTrackEfficiens(pc_eff_etaMC, [&](ETrackType t) -> TProfile* { return fvpTrackHistograms[t]->fph_eff_etaMC; });


    // ** Pulls and residuals **
    // NOTE: stored in a subdirectory for a given track type and point type
    for (int iType = 0; iType < ETrackType::END; ++iType) {
      if (fvbTrackTypeOn[iType] && fvpTrackHistograms[iType]->IsMCUsed()) {
        fvpTrackHistograms[iType]->fpFitQaFirstHit->CreateResidualPlot();
        fvpTrackHistograms[iType]->fpFitQaFirstHit->CreatePullPlot();
        fvpTrackHistograms[iType]->fpFitQaLastHit->CreateResidualPlot();
        fvpTrackHistograms[iType]->fpFitQaLastHit->CreatePullPlot();
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus OutputQa::InitQa()
try {

  if (fsParametersFilename.empty()) {
    std::stringstream errMsg;
    errMsg << "CA parameters input filename is not set. Please, provide initializer or read parameters from binary "
           << "via OutputQa::ReadParameters(filename) from the qa macro\n";
    throw std::runtime_error(errMsg.str());
  }

  fpParameters = cbm::ca::ParametersHandler::Instance()->Get(fsParametersFilename);
  LOG(info) << fName << ": parameters instance, reference: " << fpParameters.use_count();


  // Turn off detectors that are not used in the reconstruction setup

  fbUseMvd  = fbUseMvd && (fpParameters->GetNstationsActive(ca::EDetectorID::kMvd) > 0);
  fbUseSts  = fbUseSts && (fpParameters->GetNstationsActive(ca::EDetectorID::kSts) > 0);
  fbUseMuch = fbUseMuch && (fpParameters->GetNstationsActive(ca::EDetectorID::kMuch) > 0);
  fbUseTrd  = fbUseTrd && (fpParameters->GetNstationsActive(ca::EDetectorID::kTrd) > 0);
  fbUseTof  = fbUseTof && (fpParameters->GetNstationsActive(ca::EDetectorID::kTof) > 0);

  // Turn off detectors, which hits are not presented in input tree

  auto* fairManager = FairRootManager::Instance();
  fbUseMvd          = fbUseMvd && fairManager->GetObject("MvdHit");
  fbUseSts          = fbUseSts && fairManager->GetObject("StsHit");
  fbUseMuch         = fbUseMuch && fairManager->GetObject("MuchPixelHit");
  fbUseTrd          = fbUseTrd && fairManager->GetObject("TrdHit");
  fbUseTof          = fbUseTof && fairManager->GetObject("TofHit");


  LOG(info) << fName << ": Detector subsystems used:";
  LOG(info) << "\tMVD:  " << (fbUseMvd ? "ON" : "OFF");
  LOG(info) << "\tSTS:  " << (fbUseSts ? "ON" : "OFF");
  LOG(info) << "\tMuCh: " << (fbUseMuch ? "ON" : "OFF");
  LOG(info) << "\tTRD:  " << (fbUseTrd ? "ON" : "OFF");
  LOG(info) << "\tTOF:  " << (fbUseTof ? "ON" : "OFF");

  LOG(info) << fName << ": Initializing data branches";

  // Initialize IO data manager
  if (!fpDataManager.get()) {
    fpDataManager = std::make_shared<ca::DataManager>();
  }

  // Initialize time slice reader instance
  fpTSReader->SetTrackingMode(fTrackingMode);
  fpTSReader->SetDetector(ca::EDetectorID::kMvd, fbUseMvd);
  fpTSReader->SetDetector(ca::EDetectorID::kSts, fbUseSts);
  fpTSReader->SetDetector(ca::EDetectorID::kMuch, fbUseMuch);
  fpTSReader->SetDetector(ca::EDetectorID::kTrd, fbUseTrd);
  fpTSReader->SetDetector(ca::EDetectorID::kTof, fbUseTof);

  fpTSReader->RegisterParameters(fpParameters);
  fpTSReader->RegisterTracksContainer(fvRecoTracks);
  fpTSReader->RegisterQaHitContainer(fvHits);
  fpTSReader->RegisterHitIndexContainer(fvHitIds);
  fpTSReader->SetSortQaHits(true);
  if (!fpTSReader->InitRun()) {
    throw std::runtime_error("Initialization of the CbmCaTimesliceReader object failed");
  }

  // Initialize MC module
  if (IsMCUsed()) {
    fpMCModule = std::make_shared<MCModule>(fVerbose, fPerformanceMode);
    fpMCModule->SetDetector(ca::EDetectorID::kMvd, fbUseMvd);
    fpMCModule->SetDetector(ca::EDetectorID::kSts, fbUseSts);
    fpMCModule->SetDetector(ca::EDetectorID::kMuch, fbUseMuch);
    fpMCModule->SetDetector(ca::EDetectorID::kTrd, fbUseTrd);
    fpMCModule->SetDetector(ca::EDetectorID::kTof, fbUseTof);

    fpMCModule->RegisterMCData(fMCData);
    fpMCModule->RegisterRecoTrackContainer(fvRecoTracks);
    fpMCModule->RegisterHitIndexContainer(fvHitIds);
    fpMCModule->RegisterQaHitContainer(fvHits);
    fpMCModule->RegisterParameters(fpParameters);
    fpMCModule->RegisterFirstHitIndexes(fpTSReader->GetHitFirstIndexDet());
    if (!fpMCModule->InitRun()) {
      throw std::runtime_error("Initialization of the CbmCaMCModule object failed");
    }
  }

  // Initialize monitor
  fMonitor.SetCounterName(EMonitorKey::kEvent, "N events");
  fMonitor.SetCounterName(EMonitorKey::kTrack, "N reco tracks");
  fMonitor.SetCounterName(EMonitorKey::kHit, "N hits");
  fMonitor.SetCounterName(EMonitorKey::kMcTrack, "N MC tracks");
  fMonitor.SetCounterName(EMonitorKey::kMcPoint, "N MC points");
  fMonitor.SetRatioKeys({EMonitorKey::kEvent, EMonitorKey::kTrack});

  // ----- Histograms initialization
  //
  auto RegisterTrackQa = [&](const char* typeName, const char* title, ETrackType type, bool bSuppressMC = false) {
    if (!fvbTrackTypeOn[type]) {
      return;
    }
    bool bUseMC              = IsMCUsed() && !bSuppressMC;
    fvsTrackTypeName[type]   = typeName;
    fvpTrackHistograms[type] = std::make_unique<TrackTypeQa>(typeName, fsPrefix.Data(), bUseMC, fpvObjList);
    fvpTrackHistograms[type]->SetRootFolderName(fsRootFolderName + "/" + typeName);
    fvpTrackHistograms[type]->SetTitle(title);
    fvpTrackHistograms[type]->RegisterParameters(fpParameters);
    fvpTrackHistograms[type]->RegisterRecoHits(fvHits);
    fvpTrackHistograms[type]->RegisterRecoTracks(fvRecoTracks);
    fvpTrackHistograms[type]->RegisterMCData(fMCData);
    fvpTrackHistograms[type]->SetDrawAtt(fvTrackDrawAtts[type].fColor, fvTrackDrawAtts[type].fMarker);
    fvpTrackHistograms[type]->Init();
  };

  //for (int i = 0; i < ETrackType::END; ++i) {
  //LOG(info) << i << ' ' << fvpTrackHistograms[i].get() << ' ' << fvbTrackTypeOn[i];
  //}

  // TODO: Replace these parameters into the AddTrackType method!!!
  RegisterTrackQa("all", "all", ETrackType::kAll);
  if (IsMCUsed()) {
    RegisterTrackQa("prim_long_fast", "primary long fast", ETrackType::kPrimLongFast);
    RegisterTrackQa("prim_long", "primary long", ETrackType::kPrimLong);
    RegisterTrackQa("ghost", "ghost", ETrackType::kGhost, true);
    RegisterTrackQa("prim", "primary", ETrackType::kPrim);
    RegisterTrackQa("prim_fast", "primary fast", ETrackType::kPrimFast);
    RegisterTrackQa("sec", "secondary", ETrackType::kSec);
    RegisterTrackQa("sec_fast", "secondary fast", ETrackType::kSecFast);
    RegisterTrackQa("all_pi", "all #pi^{#pm}", ETrackType::kAllPI);
    RegisterTrackQa("prim_pi", "primary #pi^{#pm}", ETrackType::kPrimPI);
    RegisterTrackQa("prim_pip", "primary #pi^{#plus}", ETrackType::kPrimPIP);
    RegisterTrackQa("prim_pim", "primary #pi^{#minus}", ETrackType::kPrimPIM);
    RegisterTrackQa("sec_pi", "secondary #pi^{#pm}", ETrackType::kSecPI);
    RegisterTrackQa("sec_pip", "secondary #pi^{#plus}", ETrackType::kSecPIP);
    RegisterTrackQa("sec_pim", "secondary #pi^{#minus}", ETrackType::kSecPIM);
    RegisterTrackQa("all_e", "all e^{#pm}", ETrackType::kAllE);
    RegisterTrackQa("prim_e", "primary e^{#pm}", ETrackType::kPrimE);
    RegisterTrackQa("prim_ep", "primary e^{#plus}", ETrackType::kPrimEP);
    RegisterTrackQa("prim_em", "primary e^{#minus}", ETrackType::kPrimEM);
    RegisterTrackQa("sec_e", "secondary e^{#pm}", ETrackType::kSecE);
    RegisterTrackQa("sec_ep", "secondary e^{#plus}", ETrackType::kSecEP);
    RegisterTrackQa("sec_em", "secondary e^{#minus}", ETrackType::kSecEM);
    RegisterTrackQa("all_mu", "all #mu^{#pm}", ETrackType::kAllMU);
    RegisterTrackQa("prim_mu", "primary #mu^{#pm}", ETrackType::kPrimMU);
    RegisterTrackQa("prim_mup", "primary #mu^{#plus}", ETrackType::kPrimMUP);
    RegisterTrackQa("prim_mum", "primary #mu^{#minus}", ETrackType::kPrimMUM);
    RegisterTrackQa("sec_mu", "secondary #mu^{#pm}", ETrackType::kSecMU);
    RegisterTrackQa("sec_mup", "secondary #mu^{#plus}", ETrackType::kSecMUP);
    RegisterTrackQa("sec_mum", "secondary #mu^{#minus}", ETrackType::kSecMUM);
    RegisterTrackQa("all_k", "all K^{#pm}", ETrackType::kAllK);
    RegisterTrackQa("prim_k", "primary K^{#pm}", ETrackType::kPrimK);
    RegisterTrackQa("prim_kp", "primary K^{#plus}", ETrackType::kPrimKP);
    RegisterTrackQa("prim_km", "primary K^{#minus}", ETrackType::kPrimKM);
    RegisterTrackQa("sec_k", "secondary K^{#pm}", ETrackType::kSecK);
    RegisterTrackQa("sec_kp", "secondary K^{#plus}", ETrackType::kSecKP);
    RegisterTrackQa("sec_km", "secondary K^{#minus}", ETrackType::kSecKM);
    RegisterTrackQa("all_ppbar", "all p/#bar{p}", ETrackType::kAllPPBAR);
    RegisterTrackQa("prim_ppbar", "primary p/#bar{p}", ETrackType::kPrimPPBAR);
    RegisterTrackQa("prim_p", "primary p", ETrackType::kPrimP);
    RegisterTrackQa("prim_pbar", "primary #bar{p}", ETrackType::kPrimPBAR);
    RegisterTrackQa("sec_ppbar", "secondary p/#bar{p}", ETrackType::kSecPPBAR);
    RegisterTrackQa("sec_p", "secondary p", ETrackType::kSecP);
    RegisterTrackQa("sec_pbar", "secondary #bar{p}", ETrackType::kSecPBAR);
  }

  // Init default track types for the summary table
  if (!fmSummaryTableEntries.size()) {
    // clang-format off
    fmSummaryTableEntries = {
      ETrackType::kPrimLongFast, 
      ETrackType::kPrimLong,
      ETrackType::kPrimFast,
      ETrackType::kAll, 
      ETrackType::kPrim, 
      ETrackType::kSec
    };
    // clang-format on
  }

  return kSUCCESS;
}
catch (const std::exception& err) {
  LOG(error) << fName << ": Initialization failed. Reason: " << err.what();
  return kFATAL;
}


// ---------------------------------------------------------------------------------------------------------------------
//
void OutputQa::InitDrawingAttributes()
{
  fvTrackDrawAtts[ETrackType::kAll]   = {1, 20};
  fvTrackDrawAtts[ETrackType::kGhost] = {kGray, 20};
  fvTrackDrawAtts[ETrackType::kPrim]  = {kGray + 3, 21};
  fvTrackDrawAtts[ETrackType::kSec]   = {kGray + 2, 25};

  fvTrackDrawAtts[ETrackType::kAllPI]   = {kRed - 4, 20};
  fvTrackDrawAtts[ETrackType::kPrimPI]  = {kRed - 2, 21};
  fvTrackDrawAtts[ETrackType::kPrimPIP] = {kRed - 1, 22};
  fvTrackDrawAtts[ETrackType::kPrimPIM] = {kRed - 3, 23};
  fvTrackDrawAtts[ETrackType::kSecPI]   = {kRed - 8, 25};
  fvTrackDrawAtts[ETrackType::kSecPIP]  = {kRed - 6, 26};
  fvTrackDrawAtts[ETrackType::kSecPIM]  = {kRed - 10, 32};

  fvTrackDrawAtts[ETrackType::kAllK]   = {kBlue - 4, 20};
  fvTrackDrawAtts[ETrackType::kPrimK]  = {kBlue - 2, 21};
  fvTrackDrawAtts[ETrackType::kPrimKP] = {kBlue - 1, 22};
  fvTrackDrawAtts[ETrackType::kPrimKM] = {kBlue - 3, 23};
  fvTrackDrawAtts[ETrackType::kSecK]   = {kBlue - 8, 25};
  fvTrackDrawAtts[ETrackType::kSecKP]  = {kBlue - 6, 26};
  fvTrackDrawAtts[ETrackType::kSecKM]  = {kBlue - 10, 32};

  fvTrackDrawAtts[ETrackType::kAllPPBAR]  = {kGreen - 4, 20};
  fvTrackDrawAtts[ETrackType::kPrimPPBAR] = {kGreen - 2, 21};
  fvTrackDrawAtts[ETrackType::kPrimP]     = {kGreen - 1, 22};
  fvTrackDrawAtts[ETrackType::kPrimPBAR]  = {kGreen - 3, 23};
  fvTrackDrawAtts[ETrackType::kSecPPBAR]  = {kGreen - 8, 25};
  fvTrackDrawAtts[ETrackType::kSecP]      = {kGreen - 6, 26};
  fvTrackDrawAtts[ETrackType::kSecPBAR]   = {kGreen - 10, 32};

  fvTrackDrawAtts[ETrackType::kAllE]   = {kCyan - 4, 20};
  fvTrackDrawAtts[ETrackType::kPrimE]  = {kCyan - 2, 21};
  fvTrackDrawAtts[ETrackType::kPrimEP] = {kCyan - 1, 22};
  fvTrackDrawAtts[ETrackType::kPrimEM] = {kCyan - 3, 23};
  fvTrackDrawAtts[ETrackType::kSecE]   = {kCyan - 8, 25};
  fvTrackDrawAtts[ETrackType::kSecEP]  = {kCyan - 6, 26};
  fvTrackDrawAtts[ETrackType::kSecEM]  = {kCyan - 10, 32};

  fvTrackDrawAtts[ETrackType::kAllMU]   = {kMagenta - 4, 20};
  fvTrackDrawAtts[ETrackType::kPrimMU]  = {kMagenta - 2, 21};
  fvTrackDrawAtts[ETrackType::kPrimMUP] = {kMagenta - 1, 22};
  fvTrackDrawAtts[ETrackType::kPrimMUM] = {kMagenta - 3, 23};
  fvTrackDrawAtts[ETrackType::kSecMU]   = {kMagenta - 8, 25};
  fvTrackDrawAtts[ETrackType::kSecMUP]  = {kMagenta - 6, 26};
  fvTrackDrawAtts[ETrackType::kSecMUM]  = {kMagenta - 10, 32};
}
