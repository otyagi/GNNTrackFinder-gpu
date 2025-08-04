/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaInputQaBase.cxx
/// @date   30.08.2023
/// @brief  Class providing basic functionality for CA input QA-tasks (implementation)
/// @author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaBase.h"

#include "CaDefs.h"
#include "CbmAddress.h"
#include "CbmL1DetectorID.h"
#include "CbmMCDataArray.h"
#include "CbmMCEventList.h"
#include "CbmMCTrack.h"
#include "CbmMatch.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchPoint.h"
#include "CbmMvdHit.h"
#include "CbmMvdPoint.h"
#include "CbmQaTable.h"
#include "CbmQaUtil.h"
#include "CbmStsCluster.h"
#include "CbmStsHit.h"
#include "CbmStsPoint.h"
#include "CbmTimeSlice.h"
#include "CbmTofAddress.h"
#include "CbmTofHit.h"
#include "CbmTofPoint.h"
#include "CbmTrackingDetectorInterfaceBase.h"
#include "CbmTrdHit.h"
#include "CbmTrdPoint.h"
#include "FairMCPoint.h"
#include "FairRootManager.h"
#include "Logger.h"
#include "TBox.h"
#include "TCanvas.h"
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

using namespace cbm::algo::ca::constants;

namespace
{
  namespace ca = cbm::algo::ca;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
CbmCaInputQaBase<DetID>::CbmCaInputQaBase(const char* name, int verbose, bool isMCUsed)
  : CbmQaTask(name, verbose, isMCUsed)
{
  SetStoringMode(EStoringMode::kSUBDIR);
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
void CbmCaInputQaBase<DetID>::Check()
{
  LOG(info) << "\n\n  ** Checking the " << fName << " **\n\n";

  int nSt = fpDetInterface->GetNtrackingStations();

  // **************************************************************
  // ** Basic checks, available both for real and simulated data **
  // **************************************************************

  // Fill efficiency distributions

  if (IsMCUsed()) {
    for (int iSt = 0; iSt < nSt; ++iSt) {
      TProfile2D* effxy = fvpe_reco_eff_vs_xy[iSt];
      for (int i = 1; i < effxy->GetNbinsX() - 1; i++) {
        for (int j = 1; j < effxy->GetNbinsY() - 1; j++) {
          int bin = effxy->GetBin(i, j);
          if (effxy->GetBinEntries(bin) >= 1) {
            fvph_reco_eff[iSt]->Fill(effxy->GetBinContent(bin));
            fvph_reco_eff[nSt]->Fill(effxy->GetBinContent(bin));
          }
        }
      }
    }
  }

  // ----- Checks for mismatches in the ordering of the stations
  //
  {
    bool res = true;

    std::vector<double> vStationPos(nSt, 0.);
    for (int iSt = 0; iSt < nSt; ++iSt) {
      vStationPos[iSt] = fpDetInterface->GetZref(iSt);
    }

    if (!std::is_sorted(vStationPos.cbegin(), vStationPos.cend(), [](int l, int r) { return l <= r; })) {
      if (fVerbose > 0) {
        LOG(error) << fName << ": stations are ordered improperly along the beam axis:";
        for (auto z : vStationPos) {
          LOG(error) << "\t- " << z;
        }
      }
      res = false;
    }
    StoreCheckResult("station_position_ordering", res);
  }
  // ----- Checks for mismatch between station and hit z positions
  //   The purpose of this block is to be ensured, that hits belong to the correct tracking station. For each tracking
  // station a unified position z-coordinate is defined, which generally differs from the corresponding positions of
  // reconstructed hits. This is due to non-regular position along the beam axis for each detector sensor. Thus, the
  // positions of the hits along z-axis are distributed relatively to the position of the station in some range.
  // If hits goes out this range, it can signal about a mismatch between hits and geometry. For limits of the range
  // one should select large enough values to cover the difference described above and in the same time small enough
  // to avoid overlaps with the neighboring stations.
  //   For each station, a distribution of z_{hit} - z_{st} is integrated over the defined range and scaled by the
  // total number of entries to the distribution. If this value is smaller then unity, then some of the hits belong to
  // another station.
  //
  {
    bool res = true;
    std::stringstream msgs;
    msgs.precision(3);
    for (int iSt = 0; iSt < nSt; ++iSt) {
      int nHits = fvph_hit_station_delta_z[iSt]->GetEntries();
      if (!nHits) {
        LOG_IF(error, fVerbose > 0) << fName << ": station " << iSt << " does not have hits";
        res = false;
        continue;
      }
      int iBinMin = fvph_hit_station_delta_z[iSt]->FindBin(-fConfig.fMaxDiffZStHit);
      int iBinMax = fvph_hit_station_delta_z[iSt]->FindBin(+fConfig.fMaxDiffZStHit);

      auto nHitsWithin = fvph_hit_station_delta_z[iSt]->Integral(iBinMin, iBinMax);
      if (nHitsWithin < nHits) {
        if (!msgs.str().empty()) {
          msgs << ", ";
        }
        msgs << (static_cast<double>((nHits - nHitsWithin) * 100) / nHits) << "% (st. " << iSt << ")";
        res = false;
      }
    }
    std::string msg = msgs.str().empty() ? "" : Form("Out of range z = +-%.2f cm: ", fConfig.fMaxDiffZStHit);
    if (!msg.empty()) {
      msg += msgs.str();
    }
    StoreCheckResult("station_position_hit_delta_z", res, msg);
  }

  // *******************************************************
  // ** Additional checks, if MC information is available **
  // *******************************************************

  if (IsMCUsed()) {
    // ----- Check efficiencies
    // Error is raised, if any station has integrated efficiency lower then a selected threshold.
    // Integrated efficiency is estimated with fitting the efficiency curve for hit distances (r) with a uniform
    // distribution in the range from kEffRangeMin to kEffRangeMax, where the efficiency is assigned to be constant
    //
    // Fit efficiency curves
    {
      LOG(info) << "-- Hit efficiency integrated over hit distance from station center";

      auto* pEffTable = MakeQaObject<CbmQaTable>("vs Station/eff_table", "Efficiency table", nSt, 1);
      pEffTable->SetNamesOfCols({"Efficiency"});
      pEffTable->SetColWidth(20);

      for (int iSt = 0; iSt < nSt; ++iSt) {
        auto eff = fvph_reco_eff[iSt]->GetMean();  // FIXME (GetMean(>>>2<<<))
        pEffTable->SetRowName(iSt, Form("station %d", iSt));
        pEffTable->SetCell(iSt, 0, eff);
        bool res = CheckRange("Hit finder efficiency in station " + std::to_string(iSt), eff, fConfig.fEffThrsh, 1.000);
        std::string msg = (res ? "" : Form("efficiency = %f lower then threshold = %f", eff, fConfig.fEffThrsh));
        StoreCheckResult(Form("hit_efficiency_station_%d", iSt), res, msg);
      }
      LOG(info) << '\n' << pEffTable->ToString(3);
    }

    // ----- Checks for residuals
    // Check hits for potential biases from central values
    {
      auto* pResidualsTable =
        MakeQaObject<CbmQaTable>("vs Station/residuals_mean", "Residual mean values in different stations", nSt, 3);
      pResidualsTable->SetNamesOfCols({"Residual(x) [cm]", "Residual(y) [cm]", "Residual(t) [ns]"});
      pResidualsTable->SetColWidth(20);

      // Fit residuals
      for (int iSt = 0; iSt <= nSt; ++iSt) {

        cbm::qa::util::SetLargeStats(fvph_res_x[iSt]);
        cbm::qa::util::SetLargeStats(fvph_res_y[iSt]);
        cbm::qa::util::SetLargeStats(fvph_res_t[iSt]);

        pResidualsTable->SetRowName(iSt, Form("station %d", iSt));
        pResidualsTable->SetCell(iSt, 0, fvph_res_x[iSt]->GetStdDev());
        pResidualsTable->SetCell(iSt, 1, fvph_res_y[iSt]->GetStdDev());
        pResidualsTable->SetCell(iSt, 2, fvph_res_t[iSt]->GetStdDev());
      }
      LOG(info) << '\n' << pResidualsTable->ToString(8);
    }
    // ----- Checks for pulls
    //
    // For the hit pull is defined as a ration of the difference between hit and MC-point position or time component
    // values and an error of the hit position (or time) component, calculated in the same z-positions. In ideal case,
    // when the resolutions are well determined for detecting stations, the pull distributions should have a RMS equal
    // to unity. Here we allow a RMS of the pull distributions to be varied in a predefined range. If the RMS runs out
    // this range, QA task fails.
    {
      std::vector<CbmQaTable*> vpPullTables = {
        // {x, y, t}
        MakeQaObject<CbmQaTable>("vs Station/pulls_x", "x-coordinate pull quantities in diff. stations", nSt, 2),
        MakeQaObject<CbmQaTable>("vs Station/pulls_y", "y-coordinate pull quantities in diff. stations", nSt, 2),
        MakeQaObject<CbmQaTable>("vs Station/pulls_t", "Time pull quantities in diff. stations", nSt, 2)};

      for (auto* pullTable : vpPullTables) {
        pullTable->SetNamesOfCols({"mean", "std.dev."});
        pullTable->SetColWidth(20);
      }

      // NOTE: The table format is assumed: mean | 3.5 * mean err. | sigm | 3.5 * sigm err. |
      auto DefinePullTableRow = [&](const TH1* h, CbmQaTable* table, int iSt) {
        table->SetCell(iSt, 0, h->GetMean(), 3.5 * h->GetMeanError());
        table->SetCell(iSt, 1, h->GetStdDev(), 3.5 * h->GetStdDevError());
      };

      for (int iSt = 0; iSt < nSt + 1; ++iSt) {
        // Fit pull distributions for nicer representation. Fit results are not used in further checks.
        cbm::qa::util::SetLargeStats(fvph_pull_x[iSt]);
        cbm::qa::util::SetLargeStats(fvph_pull_y[iSt]);
        cbm::qa::util::SetLargeStats(fvph_pull_t[iSt]);

        for (auto* pullTable : vpPullTables) {
          pullTable->SetRowName(iSt, (iSt == nSt ? "all stations" : Form("station %u", iSt)));
        }
        // Check the pull quality
        {
          auto [msg, status] = CheckRangePull(fvph_pull_x[iSt]);
          StoreCheckResult(Form("pull_x_station_%d", iSt), status, msg);
          DefinePullTableRow(fvph_pull_x[iSt], vpPullTables[0], iSt);
        }
        {
          auto [msg, status] = CheckRangePull(fvph_pull_y[iSt]);
          StoreCheckResult(Form("pull_y_station_%d", iSt), status, msg);
          DefinePullTableRow(fvph_pull_y[iSt], vpPullTables[1], iSt);
        }
        {
          auto [msg, status] = CheckRangePull(fvph_pull_t[iSt]);
          StoreCheckResult(Form("pull_t_station_%d", iSt), status, msg);
          DefinePullTableRow(fvph_pull_t[iSt], vpPullTables[2], iSt);
        }
      }

      for (auto* pullTable : vpPullTables) {
        LOG(info) << '\n' << pullTable->ToString(3, true);
      }
    }
  }  // McUsed

  // Print out monitor
  LOG(info) << fMonitor.ToString();
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
std::pair<std::string, bool> CbmCaInputQaBase<DetID>::CheckRangePull(TH1* h) const
{
  constexpr double factor = 3.5;
  // Function to check overflow and underflow
  constexpr auto Check = [&](double val, double err, double min, double max) -> std::pair<std::string, bool> {
    std::stringstream msg;
    bool res;
    if (val + factor * err < min) {
      msg << "underflow: " << val << " < " << min << " - " << factor << " x " << err;
      res = false;
    }
    if (val - factor * err > max) {
      msg << "overflow: " << val << " > " << max << " + " << factor << " x " << err;
      res = false;
    }
    return std::make_pair(msg.str(), res);
  };

  std::pair<std::string, bool> ret = {"", true};
  if (h->GetEntries() >= 10) {
    // Mean check
    {
      auto [msg, res] = Check(h->GetMean(), h->GetMeanError(), -fConfig.fPullMeanThrsh, +fConfig.fPullMeanThrsh);
      if (!msg.empty()) {
        ret.first += "mean ";
        ret.first += msg;
        ret.second = res;
      }
    }
    // Sigma check
    {
      auto [msg, res] =
        Check(h->GetStdDev(), h->GetStdDevError(), 1. - fConfig.fPullWidthThrsh, 1. + fConfig.fPullWidthThrsh);
      if (!msg.empty()) {
        if (!ret.first.empty()) {
          ret.first += "; ";
        }
        ret.first += "std.dev. ";
        ret.first += msg;
        ret.second = res;
      }
    }
  }
  return ret;
}


// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
void CbmCaInputQaBase<DetID>::DeInit()
{
  // Vectors with pointers to histograms
  fvph_hit_xy.clear();
  fvph_hit_zx.clear();
  fvph_hit_zy.clear();

  fvph_hit_station_delta_z.clear();

  fvph_hit_dx.clear();
  fvph_hit_dy.clear();
  fvph_hit_du.clear();
  fvph_hit_dv.clear();
  fvph_hit_kuv.clear();
  fvph_hit_dt.clear();

  fvph_n_points_per_hit.clear();

  fvph_point_xy.clear();
  fvph_point_zx.clear();
  fvph_point_zy.clear();

  fvph_point_hit_delta_z.clear();

  fvph_res_x.clear();
  fvph_res_y.clear();
  fvph_res_u.clear();
  fvph_res_v.clear();
  fvph_res_t.clear();

  fvph_pull_x.clear();
  fvph_pull_y.clear();
  fvph_pull_u.clear();
  fvph_pull_v.clear();
  fvph_pull_t.clear();

  fvph_res_x_vs_x.clear();
  fvph_res_y_vs_y.clear();
  fvph_res_u_vs_u.clear();
  fvph_res_v_vs_v.clear();
  fvph_res_t_vs_t.clear();

  fvph_pull_x_vs_x.clear();
  fvph_pull_y_vs_y.clear();
  fvph_pull_u_vs_u.clear();
  fvph_pull_v_vs_v.clear();
  fvph_pull_t_vs_t.clear();

  fvpe_reco_eff_vs_xy.clear();
  fvph_reco_eff.clear();
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
void CbmCaInputQaBase<DetID>::ExecQa()
{
  const int nSt       = fpDetInterface->GetNtrackingStations();
  const int nHits     = fpHits->GetEntriesFast();
  const int nMCevents = (IsMCUsed()) ? fpMCEventList->GetNofEvents() : -1;

  // TODO: SZh 06.09.2023: Probably, this approach can fail, if there are several input files are used. Thus I propose
  //                       to use unordered_map with a CbmLink key type.
  std::vector<std::vector<std::vector<int>>>
    vNofHitsPerMcTrack;  // Number of hits per MC track per station in different MC events

  if (IsMCUsed()) {
    vNofHitsPerMcTrack.resize(nMCevents);
    for (int iE = 0; iE < nMCevents; ++iE) {
      int iFile     = fpMCEventList->GetFileIdByIndex(iE);
      int iEvent    = fpMCEventList->GetEventIdByIndex(iE);
      int nMcTracks = fpMCTracks->Size(iFile, iEvent);
      vNofHitsPerMcTrack[iE].resize(nSt);
      for (int iSt = 0; iSt < nSt; iSt++) {
        vNofHitsPerMcTrack[iE][iSt].resize(nMcTracks, 0);
      }
    }
  }

  for (int iH = 0; iH < nHits; ++iH) {
    fHitQaData.Reset();
    fHitQaData.SetHitIndex(iH);

    const auto* pHit = dynamic_cast<const CbmPixelHit*>(fpHits->At(iH));
    if (!pHit) {
      LOG(error) << fName << ": hit with iH = " << iH << " is not an CbmStsHit (dynamic cast failed)";
      continue;
    }

    if constexpr (ca::EDetectorID::kTof == DetID) {
      auto address = pHit->GetAddress();
      if (5 == CbmTofAddress::GetSmType(address)) {
        continue;
      }  // Skip Bmon hits
    }

    fMonitor.IncrementCounter(EMonitorKey::kHit);
    fMonitor.IncrementCounter(EMonitorKey::kHitAccepted);

    // *************************
    // ** Reconstructed hit QA

    // Hit station geometry info
    int iSt = fpDetInterface->GetTrackingStationIndex(pHit);
    if (iSt < 0) {
      continue;
    }

    if (iSt >= nSt) {
      LOG(error) << fName << ": index of station (" << iSt << ") is out of range for hit with id = " << iH;
      continue;
    }

    auto [stPhiU, stPhiV] = fpDetInterface->GetStereoAnglesSensor(pHit->GetAddress());

    fHitQaData.SetPhiU(stPhiU);
    fHitQaData.SetPhiV(stPhiV);
    fHitQaData.SetHitX(pHit->GetX());
    fHitQaData.SetHitY(pHit->GetY());
    fHitQaData.SetHitZ(pHit->GetZ());
    fHitQaData.SetHitTime(pHit->GetTime());

    fHitQaData.SetHitDx(pHit->GetDx());
    fHitQaData.SetHitDy(pHit->GetDy());
    fHitQaData.SetHitDxy(pHit->GetDxy());
    fHitQaData.SetHitTimeError(pHit->GetTimeError());
    fHitQaData.SetStationID(iSt);

    // Per station distributions
    fvph_hit_xy[iSt]->Fill(fHitQaData.GetHitX(), fHitQaData.GetHitY());
    fvph_hit_zx[iSt]->Fill(fHitQaData.GetHitZ(), fHitQaData.GetHitX());
    fvph_hit_zy[iSt]->Fill(fHitQaData.GetHitZ(), fHitQaData.GetHitY());

    fvph_hit_station_delta_z[iSt]->Fill(fHitQaData.GetHitZ() - fpDetInterface->GetZref(iSt));

    fvph_hit_dx[iSt]->Fill(fHitQaData.GetHitDx());
    fvph_hit_dy[iSt]->Fill(fHitQaData.GetHitDy());
    fvph_hit_du[iSt]->Fill(fHitQaData.GetHitDu());
    fvph_hit_dv[iSt]->Fill(fHitQaData.GetHitDv());
    fvph_hit_kuv[iSt]->Fill(fHitQaData.GetHitRuv());
    fvph_hit_dt[iSt]->Fill(fHitQaData.GetHitTimeError());

    // Sum over station distributions
    fvph_hit_xy[nSt]->Fill(fHitQaData.GetHitX(), fHitQaData.GetHitY());
    fvph_hit_zx[nSt]->Fill(fHitQaData.GetHitZ(), fHitQaData.GetHitX());
    fvph_hit_zy[nSt]->Fill(fHitQaData.GetHitZ(), fHitQaData.GetHitY());

    fvph_hit_station_delta_z[nSt]->Fill(fHitQaData.GetHitZ() - fpDetInterface->GetZref(iSt));

    fvph_hit_dx[nSt]->Fill(fHitQaData.GetHitDx());
    fvph_hit_dy[nSt]->Fill(fHitQaData.GetHitDy());
    fvph_hit_du[nSt]->Fill(fHitQaData.GetHitDu());
    fvph_hit_dv[nSt]->Fill(fHitQaData.GetHitDv());
    fvph_hit_kuv[nSt]->Fill(fHitQaData.GetHitRuv());
    fvph_hit_dt[nSt]->Fill(fHitQaData.GetHitTimeError());

    // **********************
    // ** MC information QA

    if (IsMCUsed()) {
      const auto* pHitMatch = dynamic_cast<CbmMatch*>(fpHitMatches->At(iH));
      assert(pHitMatch);

      // Evaluate number of hits per one MC point
      int nMCpoints = 0;  // Number of MC points for this hit
      for (int iLink = 0; iLink < pHitMatch->GetNofLinks(); ++iLink) {
        const auto& link = pHitMatch->GetLink(iLink);

        int iP = link.GetIndex();  // Index of MC point

        // Skip noisy links
        if (iP < 0) {
          continue;
        }

        ++nMCpoints;

        int iE = fpMCEventList->GetEventIndex(link);
        // TODO: debug
        //if (iE < 0) continue;

        if (iE < 0 || iE >= nMCevents) {
          LOG(error) << fName << ": id of MC event is out of range (hit id = " << iH << ", link id = " << iLink
                     << ", event id = " << iE << ", mc point ID = " << iP << ')';
          continue;
        }

        // matched point
        const auto* pMCPoint = dynamic_cast<const Point_t*>(fpMCPoints->Get(link));
        if (!pMCPoint) {
          LOG(error) << fName << ": MC point object does not exist for hit " << iH;
          continue;
        }

        int iTr = pMCPoint->GetTrackID();

        if (iTr >= static_cast<int>(vNofHitsPerMcTrack[iE][iSt].size())) {
          LOG(error) << fName << ": index of MC track is out of range (hit id = " << iH << ", link id = " << iLink
                     << ", event id = " << iE << ", track id = " << iTr << ')';
          continue;
        }

        vNofHitsPerMcTrack[iE][iSt][iTr]++;
      }

      fvph_n_points_per_hit[iSt]->Fill(nMCpoints);
      fvph_n_points_per_hit[nSt]->Fill(nMCpoints);

      //
      // Fill the following histograms exclusively for isolated hits with a one-to-one correspondence to the mc track
      // The mc track must satisfy the cuts
      //

      if (pHitMatch->GetNofLinks() != 1) {
        continue;
      }

      // The best link to in the match (probably, the cut on nMCpoints is meaningless)
      assert(pHitMatch->GetNofLinks() > 0);  // Should be always true due to the cut above
      const auto& bestPointLink = pHitMatch->GetMatchedLink();

      // Skip noisy links
      if (bestPointLink.GetIndex() < 0) {
        continue;
      }

      // Point matched by the best link
      const auto* pMCPoint = dynamic_cast<const Point_t*>(fpMCPoints->Get(bestPointLink));
      fHitQaData.SetPointID(bestPointLink.GetIndex(), bestPointLink.GetEntry(), bestPointLink.GetFile());
      if (!pMCPoint) {
        LOG(error) << fName << ": MC point object does not exist for hit " << iH;
        continue;
      }

      // MC track for this point
      CbmLink bestTrackLink = bestPointLink;
      bestTrackLink.SetIndex(pMCPoint->GetTrackID());
      const auto* pMCTrack = dynamic_cast<const CbmMCTrack*>(fpMCTracks->Get(bestTrackLink));
      if (!pMCTrack) {
        LOG(error) << fName << ": MC track object does not exist for hit " << iH << " and link: ";
        continue;
      }

      double t0MC = fpMCEventList->GetEventTime(bestPointLink);
      if (t0MC < 0) {
        LOG(error) << fName << ": MC time zero is lower then 0 ns: " << t0MC;
        continue;
      }

      // cut on the mc track quality
      if (!IsTrackSelected(pMCTrack, pMCPoint)) {
        continue;
      }

      {  // skip the case when the mc point participates to several hits
        int iE = fpMCEventList->GetEventIndex(bestTrackLink);
        if (vNofHitsPerMcTrack[iE][iSt][pMCPoint->GetTrackID()] != 1) {
          continue;
        }
      }

      // ----- MC point properties
      //
      double mass = pMCTrack->GetMass();
      //double charge = pMCTrack->GetCharge();
      //double pdg    = pMCTrack->GetPdgCode();

      // Entrance position and time
      // NOTE: SZh 04.09.2023: Methods GetX(), GetY(), GetZ() for MVD, STS, MUCH, TRD and TOF always return
      //                       positions of track at entrance to the active volume.
      double xMC = pMCPoint->FairMCPoint::GetX();
      double yMC = pMCPoint->FairMCPoint::GetY();
      double zMC = pMCPoint->FairMCPoint::GetZ();
      double tMC = pMCPoint->GetTime() + t0MC;

      // MC point entrance momenta
      // NOTE: SZh 04.09.2023: Methods GetPx(), GetPy(), GetPz() for MVD, STS, MUCH, TRD and TOF always return
      //                       the momentum components of track at entrance to the active volume.
      double pxMC = pMCPoint->GetPx();
      double pyMC = pMCPoint->GetPy();
      double pzMC = pMCPoint->GetPz();
      double pMC  = sqrt(pxMC * pxMC + pyMC * pyMC + pzMC * pzMC);

      // MC point exit momenta
      // double pxMCo = pMCPoint->GetPxOut();
      // double pyMCo = pMCPoint->GetPyOut();
      // double pzMCo = pMCPoint->GetPzOut();
      // double pMCo  = sqrt(pxMCo * pxMCo + pyMCo * pyMCo + pzMCo * pzMCo);

      // Position and time shifted to z-coordinate of the hit
      // TODO: SZh 04.09.2023: Probably, the approximation
      double shiftZ = fHitQaData.GetHitZ() - zMC;  // Difference between hit and point z positions
      double xMCs   = xMC + shiftZ * pxMC / pzMC;
      double yMCs   = yMC + shiftZ * pyMC / pzMC;
      double tMCs   = tMC + shiftZ / (pzMC * phys::SpeedOfLight) * sqrt(mass * mass + pMC * pMC);

      fHitQaData.SetPointTime(tMCs);
      fHitQaData.SetPointX(xMCs);
      fHitQaData.SetPointY(yMCs);
      fHitQaData.SetPointZ(fHitQaData.GetHitZ());

      double zRes = kNAN;
      if constexpr (ca::EDetectorID::kTof == DetID) {
        zRes = fHitQaData.GetHitZ() - pMCPoint->GetZ();
      }
      else {
        zRes = fHitQaData.GetHitZ() - 0.5 * (pMCPoint->GetZ() + pMCPoint->GetZOut());
      }
      fvph_point_hit_delta_z[iSt]->Fill(zRes);

      double xRes = fHitQaData.GetResidualX();
      double yRes = fHitQaData.GetResidualY();
      double uRes = fHitQaData.GetResidualU();
      double vRes = fHitQaData.GetResidualV();
      double tRes = fHitQaData.GetResidualTime();

      double xPull = fHitQaData.GetPullX();
      double yPull = fHitQaData.GetPullY();
      double uPull = fHitQaData.GetPullU();
      double vPull = fHitQaData.GetPullV();
      double tPull = fHitQaData.GetPullTime();

      fvph_res_x[iSt]->Fill(xRes);
      fvph_res_y[iSt]->Fill(yRes);
      fvph_res_u[iSt]->Fill(uRes);
      fvph_res_v[iSt]->Fill(vRes);
      fvph_res_t[iSt]->Fill(tRes);

      fvph_pull_x[iSt]->Fill(xPull);
      fvph_pull_y[iSt]->Fill(yPull);
      fvph_pull_u[iSt]->Fill(uPull);
      fvph_pull_v[iSt]->Fill(vPull);
      fvph_pull_t[iSt]->Fill(tPull);

      fvph_res_x_vs_x[iSt]->Fill(fHitQaData.GetPointX(), xRes);
      fvph_res_y_vs_y[iSt]->Fill(fHitQaData.GetPointY(), yRes);
      fvph_res_u_vs_u[iSt]->Fill(fHitQaData.GetPointU(), uRes);
      fvph_res_v_vs_v[iSt]->Fill(fHitQaData.GetPointV(), vRes);
      fvph_res_t_vs_t[iSt]->Fill(fHitQaData.GetPointTime(), tRes);

      fvph_pull_x_vs_x[iSt]->Fill(fHitQaData.GetPointX(), xPull);
      fvph_pull_y_vs_y[iSt]->Fill(fHitQaData.GetPointY(), yPull);
      fvph_pull_u_vs_u[iSt]->Fill(fHitQaData.GetPointU(), uPull);
      fvph_pull_v_vs_v[iSt]->Fill(fHitQaData.GetPointV(), vPull);
      fvph_pull_t_vs_t[iSt]->Fill(fHitQaData.GetPointTime(), tPull);

      // fill summary histos

      fvph_point_hit_delta_z[nSt]->Fill(zRes);

      fvph_res_x[nSt]->Fill(xRes);
      fvph_res_y[nSt]->Fill(yRes);
      fvph_res_u[nSt]->Fill(uRes);
      fvph_res_v[nSt]->Fill(vRes);
      fvph_res_t[nSt]->Fill(tRes);

      fvph_pull_x[nSt]->Fill(xPull);
      fvph_pull_y[nSt]->Fill(yPull);
      fvph_pull_u[nSt]->Fill(uPull);
      fvph_pull_v[nSt]->Fill(vPull);
      fvph_pull_t[nSt]->Fill(tPull);

      fvph_res_x_vs_x[nSt]->Fill(fHitQaData.GetPointX(), xRes);
      fvph_res_y_vs_y[nSt]->Fill(fHitQaData.GetPointY(), yRes);
      fvph_res_u_vs_u[nSt]->Fill(fHitQaData.GetPointU(), uRes);
      fvph_res_v_vs_v[nSt]->Fill(fHitQaData.GetPointV(), vRes);
      fvph_res_t_vs_t[nSt]->Fill(fHitQaData.GetPointTime(), tRes);

      fvph_pull_x_vs_x[nSt]->Fill(fHitQaData.GetPointX(), xPull);
      fvph_pull_y_vs_y[nSt]->Fill(fHitQaData.GetPointY(), yPull);
      fvph_pull_u_vs_u[nSt]->Fill(fHitQaData.GetPointU(), uPull);
      fvph_pull_v_vs_v[nSt]->Fill(fHitQaData.GetPointV(), vPull);
      fvph_pull_t_vs_t[nSt]->Fill(fHitQaData.GetPointTime(), tPull);
    }
    FillHistogramsPerHit();
  }  // loop over hits: end

  // Fill hit reconstruction efficiencies
  if (IsMCUsed()) {
    for (int iE = 0; iE < nMCevents; ++iE) {
      int iFile   = fpMCEventList->GetFileIdByIndex(iE);
      int iEvent  = fpMCEventList->GetEventIdByIndex(iE);
      int nPoints = fpMCPoints->Size(iFile, iEvent);
      int nTracks = fpMCTracks->Size(iFile, iEvent);

      // If efficiency for the track at the station is already evaluated
      std::vector<std::vector<bool>> vIsTrackProcessed(nSt);
      for (int iSt = 0; iSt < nSt; iSt++) {
        vIsTrackProcessed[iSt].resize(nTracks, 0);
      }

      for (int iP = 0; iP < nPoints; ++iP) {
        fHitQaData.Reset();
        fHitQaData.SetPointID(iP, iEvent, iFile);
        fMonitor.IncrementCounter(EMonitorKey::kMcPoint);

        const auto* pMCPoint = dynamic_cast<const Point_t*>(fpMCPoints->Get(iFile, iEvent, iP));
        if (!pMCPoint) {
          LOG(error) << fName << ": MC point does not exist for iFile = " << iFile << ", iEvent = " << iEvent
                     << ", iP = " << iP;
          continue;
        }

        int address = pMCPoint->GetDetectorID();
        int iSt     = fpDetInterface->GetTrackingStationIndex(pMCPoint);
        if (iSt < 0) {
          continue;  // NOTE: legit, such sensors must be ignored in tracking
        }

        if (iSt >= nSt) {
          LOG(error) << fName << ": MC point for FEI = " << iFile << ", " << iEvent << ", " << iP << " and address "
                     << address << " has wrong station index: iSt = " << iSt;
          fMonitor.IncrementCounter(EMonitorKey::kMcPointWrongStation);
          continue;
        }

        // NOTE: SZh 04.09.2023: Methods GetX(), GetY(), GetZ() for MVD, STS, MUCH, TRD and TOF always return
        //                       positions of track at entrance to the active volume.
        fHitQaData.SetPointX(pMCPoint->FairMCPoint::GetX());
        fHitQaData.SetPointY(pMCPoint->FairMCPoint::GetY());
        fHitQaData.SetPointZ(pMCPoint->FairMCPoint::GetZ());

        fvph_point_xy[iSt]->Fill(fHitQaData.GetPointX(), fHitQaData.GetPointY());
        fvph_point_zx[iSt]->Fill(fHitQaData.GetPointZ(), fHitQaData.GetPointX());
        fvph_point_zy[iSt]->Fill(fHitQaData.GetPointZ(), fHitQaData.GetPointY());

        fvph_point_xy[nSt]->Fill(fHitQaData.GetPointX(), fHitQaData.GetPointY());
        fvph_point_zx[nSt]->Fill(fHitQaData.GetPointZ(), fHitQaData.GetPointX());
        fvph_point_zy[nSt]->Fill(fHitQaData.GetPointZ(), fHitQaData.GetPointY());

        int iTr = pMCPoint->GetTrackID();

        if (iTr >= nTracks) {
          LOG(error) << fName << ": index of MC track is out of range (point id = " << iP << ", event id = " << iE
                     << ", track id = " << iTr << ')';
          continue;
        }
        const auto* pMCTrack = dynamic_cast<const CbmMCTrack*>(fpMCTracks->Get(iFile, iEvent, iTr));

        fHitQaData.SetIfTrackHasHits(vNofHitsPerMcTrack[iE][iSt][iTr] > 0);
        fHitQaData.SetIfTrackSelected(IsTrackSelected(pMCTrack, pMCPoint));

        if (!pMCTrack) {
          LOG(error) << fName << ": null MC track pointer for file id = " << iFile << ", event id = " << iEvent
                     << ", track id = " << iTr;
          continue;
        }

        // check efficiency only once per mc track per station
        if (vIsTrackProcessed[iSt][iTr]) {
          continue;
        }

        vIsTrackProcessed[iSt][iTr] = true;

        // cut on the mc track quality
        if (!fHitQaData.GetIfTrackSelected()) {
          continue;
        }

        // Conditions under which point is accounted as reconstructed: point
        bool ifTrackHasHits = fHitQaData.GetIfTrackHasHits();

        fvpe_reco_eff_vs_xy[iSt]->Fill(fHitQaData.GetPointX(), fHitQaData.GetPointY(), ifTrackHasHits);
        fvpe_reco_eff_vs_xy[nSt]->Fill(fHitQaData.GetPointX(), fHitQaData.GetPointY(), ifTrackHasHits);
      }  // loop over MC-points: end
    }    // loop over MC-events: end
  }      // MC is used: end
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
InitStatus CbmCaInputQaBase<DetID>::InitQa()
{
  // ----- Specific configuration initialization
  fConfig = ReadSpecificConfig<CbmCaInputQaBase<DetID>::Config>().value_or(Config{});


  LOG_IF(fatal, !fpDetInterface) << "\033[1;31m" << fName << ": tracking detector interface is undefined\033[0m";

  // FairRootManager
  auto* pFairRootManager = FairRootManager::Instance();
  LOG_IF(fatal, !pFairRootManager) << "\033[1;31m" << fName << ": FairRootManager instance is a null pointer\033[0m";

  // Time-slice
  fpTimeSlice = dynamic_cast<CbmTimeSlice*>(pFairRootManager->GetObject("TimeSlice."));
  LOG_IF(fatal, !fpTimeSlice) << "\033[1;31m" << fName << ": time-slice branch is not found\033[0m";

  // ----- Reconstructed data-branches initialization

  // Hits container
  if constexpr (DetID == EDetectorID::kTof) {
    fpHits = dynamic_cast<TClonesArray*>(pFairRootManager->GetObject("TofCalHit"));
    if (!fpHits) {
      fpHits = dynamic_cast<TClonesArray*>(pFairRootManager->GetObject("TofHit"));
    }
  }
  else {
    fpHits = dynamic_cast<TClonesArray*>(pFairRootManager->GetObject(cbm::ca::kDetHitBrName[DetID]));
  }

  LOG_IF(fatal, !fpHits) << "\033[1;31m" << fName << ": container of reconstructed hits is not found\033[0m";

  // ----- MC-information branches initialization
  if (IsMCUsed()) {
    // MC manager
    fpMCDataManager = dynamic_cast<CbmMCDataManager*>(pFairRootManager->GetObject("MCDataManager"));
    LOG_IF(fatal, !fpMCDataManager) << "\033[1;31m" << fName << ": MC data manager branch is not found\033[0m";

    // MC event list
    fpMCEventList = dynamic_cast<CbmMCEventList*>(pFairRootManager->GetObject("MCEventList."));
    LOG_IF(fatal, !fpMCEventList) << "\033[1;31m" << fName << ": MC event list branch is not found\033[0m";

    // MC tracks
    fpMCTracks = fpMCDataManager->InitBranch("MCTrack");
    LOG_IF(fatal, !fpMCTracks) << "\033[1;31m" << fName << ": MC track branch is not found\033[0m";

    // MC points
    fpMCPoints = fpMCDataManager->InitBranch(cbm::ca::kDetPointBrName[DetID]);
    LOG_IF(fatal, !fpMCTracks) << "\033[1;31m" << fName << ": MC point branch is not found\033[0m";

    // Hit matches
    const char* hitMatchName = Form("%sMatch", cbm::ca::kDetHitBrName[DetID]);
    fpHitMatches             = dynamic_cast<TClonesArray*>(pFairRootManager->GetObject(hitMatchName));
    LOG_IF(fatal, !fpHitMatches) << "\033[1;31m]" << fName << ": hit match branch is not found\033[0m";
  }

  // get hit distribution ranges from the geometry
  int nSt = fpDetInterface->GetNtrackingStations();

  frXmin.resize(nSt + 1, 0.);
  frXmax.resize(nSt + 1, 0.);

  frYmin.resize(nSt + 1, 0.);
  frYmax.resize(nSt + 1, 0.);

  frZmin.resize(nSt + 1, 0.);
  frZmax.resize(nSt + 1, 0.);

  for (int i = nSt; i >= 0; --i) {
    {  // read the ranges for station i
      int j = (i == nSt ? 0 : i);

      frXmin[i] = fpDetInterface->GetXmin(j);
      frXmax[i] = fpDetInterface->GetXmax(j);

      frYmin[i] = fpDetInterface->GetYmin(j);
      frYmax[i] = fpDetInterface->GetYmax(j);

      frZmin[i] = fpDetInterface->GetZmin(j);
      frZmax[i] = fpDetInterface->GetZmax(j);
    }

    // update overall ranges

    frXmin[nSt] = std::min(frXmin[nSt], frXmin[i]);
    frXmax[nSt] = std::max(frXmax[nSt], frXmax[i]);

    frYmin[nSt] = std::min(frYmin[nSt], frYmin[i]);
    frYmax[nSt] = std::max(frYmax[nSt], frYmax[i]);

    frZmin[nSt] = std::min(frZmin[nSt], frZmin[i]);
    frZmax[nSt] = std::max(frZmax[nSt], frZmax[i]);
  }

  // add 5% margins for a better view

  for (int i = 0; i <= nSt; ++i) {
    double dx = 0.05 * fabs(frXmax[i] - frXmin[i]);
    frXmin[i] -= dx;
    frXmax[i] += dx;

    double dy = 0.05 * fabs(frYmax[i] - frYmin[i]);
    frYmin[i] -= dy;
    frYmax[i] += dy;

    if constexpr (ca::EDetectorID::kMuch == DetID) {
      frZmin[i] -= 40;
      frZmax[i] += 40;
    }
    else {
      double dz = 0.05 * fabs(frZmax[i] - frZmin[i]);
      frZmin[i] -= dz;
      frZmax[i] += dz;
    }
  }

  // ----- Monitor initialization
  //
  fMonitor.SetName(Form("Monitor for %s", fName.Data()));
  fMonitor.SetCounterName(EMonitorKey::kEvent, "N events");
  fMonitor.SetCounterName(EMonitorKey::kHit, "N hits total");
  fMonitor.SetCounterName(EMonitorKey::kHitAccepted, "N hits accepted");
  fMonitor.SetCounterName(EMonitorKey::kMcPoint, "N MC points total");
  fMonitor.SetCounterName(EMonitorKey::kMcPointWrongStation, "N MC points total");
  fMonitor.SetRatioKeys({EMonitorKey::kEvent});


  // ----- Histograms initialization
  //
  //SetStoringMode(EStoringMode::kSAMEDIR);
  MakeQaDirectory("Summary");
  MakeQaDirectory("Summary/vs Station");
  if constexpr (ca::EDetectorID::kSts == DetID) {
    MakeQaDirectory("Summary/vs N digi");
  }
  MakeQaDirectory("All stations");

  fvph_hit_xy.resize(nSt + 1, nullptr);
  fvph_hit_zy.resize(nSt + 1, nullptr);
  fvph_hit_zx.resize(nSt + 1, nullptr);

  fvph_hit_station_delta_z.resize(nSt + 1, nullptr);

  fvph_hit_dx.resize(nSt + 1, nullptr);
  fvph_hit_dy.resize(nSt + 1, nullptr);
  fvph_hit_dt.resize(nSt + 1, nullptr);
  fvph_hit_dv.resize(nSt + 1, nullptr);
  fvph_hit_kuv.resize(nSt + 1, nullptr);
  fvph_hit_du.resize(nSt + 1, nullptr);

  std::string detName = fpDetInterface->GetDetectorName();

  for (int iSt = 0; iSt <= nSt; ++iSt) {
    TString sF = "";  // Subfolder
    sF += (iSt == nSt) ? "All stations/" : Form("Station %d/", iSt);
    TString nsuff = (iSt == nSt) ? "" : Form("_st%d", iSt);                               // Histogram name suffix
    TString tsuff = (iSt == nSt) ? "" : Form(" in %s station %d", detName.c_str(), iSt);  // Histogram title suffix
    TString sN    = "";
    TString sT    = "";

    // place directories in a prefered order
    MakeQaDirectory(sF + "occup/");
    if (IsMCUsed()) {
      MakeQaDirectory(sF + "res/");
      MakeQaDirectory(sF + "pull/");
      MakeQaDirectory(sF + "eff/");
    }
    MakeQaDirectory(sF + "err/");

    int nBinsZ = (iSt < nSt) ? fNbins : fNbinsZ;

    // Hit occupancy
    sN = (TString) "hit_xy" + nsuff;
    sT = (TString) "Hit occupancy in xy-Plane" + tsuff + ";x_{hit} [cm];y_{hit} [cm]";
    fvph_hit_xy[iSt] =
      MakeQaObject<TH2F>(sF + "occup/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], fNbins, frYmin[iSt], frYmax[iSt]);

    sN = (TString) "hit_zx" + nsuff;
    sT = (TString) "Hit occupancy in xz-Plane" + tsuff + ";z_{hit} [cm];x_{hit} [cm]";
    fvph_hit_zx[iSt] =
      MakeQaObject<TH2F>(sF + "occup/" + sN, sT, nBinsZ, frZmin[iSt], frZmax[iSt], fNbins, frXmin[iSt], frXmax[iSt]);

    sN = (TString) "hit_zy" + nsuff;
    sT = (TString) "Hit occupancy in yz-plane" + tsuff + ";z_{hit} [cm];y_{hit} [cm]";
    fvph_hit_zy[iSt] =
      MakeQaObject<TH2F>(sF + "occup/" + sN, sT, nBinsZ, frZmin[iSt], frZmax[iSt], fNbins, frYmin[iSt], frYmax[iSt]);

    // Hit errors
    sN               = (TString) "hit_dx" + nsuff;
    sT               = (TString) "Hit position error along x-axis" + tsuff + ";dx_{hit} [cm]";
    fvph_hit_dx[iSt] = MakeQaObject<TH1F>(sF + "err/" + sN, sT, fNbins, fRHitDx[0], fRHitDx[1]);

    sN               = (TString) "hit_dy" + nsuff;
    sT               = (TString) "Hit position error along y-axis" + tsuff + ";dy_{hit} [cm]";
    fvph_hit_dy[iSt] = MakeQaObject<TH1F>(sF + "err/" + sN, sT, fNbins, fRHitDy[0], fRHitDy[1]);

    sN               = (TString) "hit_du" + nsuff;
    sT               = (TString) "Hit position error along the major detector coordinate U" + tsuff + ";du_{hit} [cm]";
    fvph_hit_du[iSt] = MakeQaObject<TH1F>(sF + "err/" + sN, sT, fNbins, fRHitDu[0], fRHitDu[1]);

    sN               = (TString) "hit_dv" + nsuff;
    sT               = (TString) "Hit position error along the minor detector coordinate V" + tsuff + ";dv_{hit} [cm]";
    fvph_hit_dv[iSt] = MakeQaObject<TH1F>(sF + "err/" + sN, sT, fNbins, fRHitDv[0], fRHitDv[1]);

    sN = (TString) "hit_kuv" + nsuff;
    sT = (TString) "Hit error correlation between the major (U) and the minor detector coordinate Vs" + tsuff
         + ";kuv_{hit} [unitless]";
    fvph_hit_kuv[iSt] = MakeQaObject<TH1F>(sF + "err/" + sN, sT, fNbins / 2 * 2 + 1, -1.1, 1.1);

    sN               = (TString) "hit_dt" + nsuff;
    sT               = (TString) "Hit time error" + tsuff + ";dt_{hit} [ns]";
    fvph_hit_dt[iSt] = MakeQaObject<TH1F>(sF + "err/" + sN, sT, fNbins, fRHitDt[0], fRHitDt[1]);

    sN = (TString) "hit_station_delta_z" + nsuff;
    sT = (TString) "Different between hit and station z-positions" + tsuff + ";z_{hit} - z_{st} [cm]";
    fvph_hit_station_delta_z[iSt] = MakeQaObject<TH1F>(sF + sN, sT, fNbins, -5., 5.);

  }  // loop over station index: end

  // ----- Initialize histograms, which are use MC-information
  if (IsMCUsed()) {
    // Resize histogram vectors
    fvph_n_points_per_hit.resize(nSt + 1, nullptr);
    fvph_point_xy.resize(nSt + 1, nullptr);
    fvph_point_zx.resize(nSt + 1, nullptr);
    fvph_point_zy.resize(nSt + 1, nullptr);
    fvph_point_hit_delta_z.resize(nSt + 1, nullptr);
    fvph_res_x.resize(nSt + 1, nullptr);
    fvph_res_y.resize(nSt + 1, nullptr);
    fvph_res_u.resize(nSt + 1, nullptr);
    fvph_res_v.resize(nSt + 1, nullptr);
    fvph_res_t.resize(nSt + 1, nullptr);
    fvph_pull_x.resize(nSt + 1, nullptr);
    fvph_pull_y.resize(nSt + 1, nullptr);
    fvph_pull_u.resize(nSt + 1, nullptr);
    fvph_pull_v.resize(nSt + 1, nullptr);
    fvph_pull_t.resize(nSt + 1, nullptr);
    fvph_res_x_vs_x.resize(nSt + 1, nullptr);
    fvph_res_y_vs_y.resize(nSt + 1, nullptr);
    fvph_res_u_vs_u.resize(nSt + 1, nullptr);
    fvph_res_v_vs_v.resize(nSt + 1, nullptr);
    fvph_res_t_vs_t.resize(nSt + 1, nullptr);
    fvph_pull_x_vs_x.resize(nSt + 1, nullptr);
    fvph_pull_y_vs_y.resize(nSt + 1, nullptr);
    fvph_pull_u_vs_u.resize(nSt + 1, nullptr);
    fvph_pull_v_vs_v.resize(nSt + 1, nullptr);
    fvph_pull_t_vs_t.resize(nSt + 1, nullptr);
    fvpe_reco_eff_vs_xy.resize(nSt + 1, nullptr);
    fvph_reco_eff.resize(nSt + 1, nullptr);

    for (int iSt = 0; iSt <= nSt; ++iSt) {
      TString sF = "";  // Subfolder
      sF += (iSt == nSt) ? "All stations/" : Form("Station %d/", iSt);
      TString nsuff = (iSt == nSt) ? "" : Form("_st%d", iSt);                               // Histogram name suffix
      TString tsuff = (iSt == nSt) ? "" : Form(" in %s station %d", detName.c_str(), iSt);  // Histogram title suffix
      TString sN    = "";
      TString sT    = "";

      sN                         = (TString) "n_points_per_hit" + nsuff;
      sT                         = (TString) "Number of points per hit" + tsuff + ";N_{point}/hit";
      fvph_n_points_per_hit[iSt] = MakeQaObject<TH1F>(sF + sN, sT, 10, -0.5, 9.5);

      // Point occupancy
      sN = (TString) "point_xy" + nsuff;
      sT = (TString) "Point occupancy in XY plane" + tsuff + ";x_{MC} [cm];y_{MC} [cm]";
      fvph_point_xy[iSt] =
        MakeQaObject<TH2F>(sF + "occup/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], fNbins, frYmin[iSt], frYmax[iSt]);

      sN = (TString) "point_zx" + nsuff;
      sT = (TString) "Point Occupancy in XZ plane" + tsuff + ";z_{MC} [cm];x_{MC} [cm]";
      fvph_point_zx[iSt] =
        MakeQaObject<TH2F>(sF + "occup/" + sN, sT, fNbinsZ, frZmin[iSt], frZmax[iSt], fNbins, frXmin[iSt], frXmax[iSt]);

      sN = (TString) "point_zy" + nsuff;
      sT = (TString) "Point Occupancy in YZ Plane" + tsuff + ";z_{MC} [cm];y_{MC} [cm]";
      fvph_point_zy[iSt] =
        MakeQaObject<TH2F>(sF + "occup/" + sN, sT, fNbinsZ, frZmin[iSt], frZmax[iSt], fNbins, frYmin[iSt], frYmax[iSt]);

      // Difference between MC input z and hit z coordinates
      sN = (TString) "point_hit_delta_z" + nsuff;
      sT = (TString) "Distance between " + detName + " point and hit along z axis" + tsuff + ";z_{reco} - z_{MC} [cm]";
      fvph_point_hit_delta_z[iSt] = MakeQaObject<TH1F>(sF + sN, sT, fNbins, fRangeDzHitPoint[0], fRangeDzHitPoint[1]);

      sN              = (TString) "res_x" + nsuff;
      sT              = (TString) "Residuals for X" + tsuff + ";x_{reco} - x_{MC} [cm]";
      fvph_res_x[iSt] = MakeQaObject<TH1F>(sF + "res/" + sN, sT, fNbins, fRResX[0], fRResX[1]);

      sN              = (TString) "res_y" + nsuff;
      sT              = (TString) "Residuals for Y" + tsuff + ";y_{reco} - y_{MC} [cm]";
      fvph_res_y[iSt] = MakeQaObject<TH1F>(sF + "res/" + sN, sT, fNbins, fRResY[0], fRResY[1]);

      sN              = (TString) "res_u" + nsuff;
      sT              = (TString) "Residuals for the major detector coordinate U" + tsuff + ";u_{reco} - u_{MC} [cm]";
      fvph_res_u[iSt] = MakeQaObject<TH1F>(sF + "res/" + sN, sT, fNbins, fRResU[0], fRResU[1]);

      sN              = (TString) "res_v" + nsuff;
      sT              = (TString) "Residuals for the minor detector coordinate V" + tsuff + ";v_{reco} - v_{MC} [cm]";
      fvph_res_v[iSt] = MakeQaObject<TH1F>(sF + "res/" + sN, sT, fNbins, fRResV[0], fRResV[1]);

      sN              = (TString) "res_t" + nsuff;
      sT              = (TString) "Residuals for Time" + tsuff + ";t_{reco} - t_{MC} [ns]";
      fvph_res_t[iSt] = MakeQaObject<TH1F>(sF + "res/" + sN, sT, fNbins, fRResT[0], fRResT[1]);

      sN               = (TString) "pull_x" + nsuff;
      sT               = (TString) "Pulls for X" + tsuff + ";(x_{reco} - x_{MC}) / #sigma_{x}^{reco}";
      fvph_pull_x[iSt] = MakeQaObject<TH1F>(sF + "pull/" + sN, sT, kNbinsPull, kRPull[0], kRPull[1]);

      sN               = (TString) "pull_y" + nsuff;
      sT               = (TString) "Pulls for Y" + tsuff + ";(y_{reco} - y_{MC}) / #sigma_{y}^{reco}";
      fvph_pull_y[iSt] = MakeQaObject<TH1F>(sF + "pull/" + sN, sT, kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "pull_u" + nsuff;
      sT = (TString) "Pulls for the major detector coordinate U" + tsuff + ";(u_{reco} - u_{MC}) / #sigma_{u}^{reco}";
      fvph_pull_u[iSt] = MakeQaObject<TH1F>(sF + "pull/" + sN, sT, kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "pull_v" + nsuff;
      sT = (TString) "Pulls for the minor detector coordinate V" + tsuff + ";(v_{reco} - v_{MC}) / #sigma_{v}^{reco}";
      fvph_pull_v[iSt] = MakeQaObject<TH1F>(sF + "pull/" + sN, sT, kNbinsPull, kRPull[0], kRPull[1]);

      sN               = (TString) "pull_t" + nsuff;
      sT               = (TString) "Pulls for Time" + tsuff + ";(t_{reco} - t_{MC}) / #sigma_{t}^{reco}";
      fvph_pull_t[iSt] = MakeQaObject<TH1F>(sF + "pull/" + sN, sT, kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "res_x_vs_x" + nsuff;
      sT = (TString) "Residuals for X" + tsuff + ";x_{MC} [cm];x_{reco} - x_{MC} [cm]";
      fvph_res_x_vs_x[iSt] =
        MakeQaObject<TH2F>(sF + "res/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], fNbins, fRResX[0], fRResX[1]);

      sN = (TString) "res_y_vs_y" + nsuff;
      sT = (TString) "Residuals for Y" + tsuff + ";y_{MC} [cm];y_{reco} - y_{MC} [cm]";
      fvph_res_y_vs_y[iSt] =
        MakeQaObject<TH2F>(sF + "res/" + sN, sT, fNbins, frYmin[iSt], frYmax[iSt], fNbins, fRResY[0], fRResY[1]);

      sN = (TString) "res_u_vs_u" + nsuff;
      sT = (TString) "Residuals for the major detector coordinate U" + tsuff + ";u_{MC} [cm];u_{reco} - u_{MC} [cm]";
      fvph_res_u_vs_u[iSt] =
        MakeQaObject<TH2F>(sF + "res/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], fNbins, fRResU[0], fRResU[1]);

      sN = (TString) "res_v_vs_v" + nsuff;
      sT = (TString) "Residuals for the minor detector coordinate V" + tsuff + ";v_{MC} [cm];v_{reco} - v_{MC} [cm]";
      fvph_res_v_vs_v[iSt] =
        MakeQaObject<TH2F>(sF + "res/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], fNbins, fRResV[0], fRResV[1]);

      sN                   = (TString) "res_t_vs_t" + nsuff;
      sT                   = (TString) "Residuals for Time" + tsuff + ";t_{MC} [ns];t_{reco} - t_{MC} [ns]";
      fvph_res_t_vs_t[iSt] = MakeQaObject<TH2F>(sF + "res/" + sN, sT, fNbins, 0, 0, fNbins, fRResT[0], fRResT[1]);

      sN = (TString) "pull_x_vs_x" + nsuff;
      sT = (TString) "Pulls for X" + tsuff + ";x_{MC} [cm];(x_{reco} - x_{MC}) / #sigma_{x}^{reco}";
      fvph_pull_x_vs_x[iSt] =
        MakeQaObject<TH2F>(sF + "pull/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "pull_y_vs_y" + nsuff;
      sT = (TString) "Pulls for Y" + tsuff + ";y_{MC} [cm];(y_{reco} - y_{MC}) / #sigma_{y}^{reco}";
      fvph_pull_y_vs_y[iSt] =
        MakeQaObject<TH2F>(sF + "pull/" + sN, sT, fNbins, frYmin[iSt], frYmax[iSt], kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "pull_u_vs_u" + nsuff;
      sT = (TString) "Pulls for the major detector coordinate U" + tsuff
           + ";u_{MC} [cm];(u_{reco} - u_{MC}) / #sigma_{u}^{reco}";
      fvph_pull_u_vs_u[iSt] =
        MakeQaObject<TH2F>(sF + "pull/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "pull_v_vs_v" + nsuff;
      sT = (TString) "Pulls for the minor detector coordinate V" + tsuff
           + ";v_{MC} [cm];(v_{reco} - v_{MC}) / #sigma_{v}^{reco}";
      fvph_pull_v_vs_v[iSt] =
        MakeQaObject<TH2F>(sF + "pull/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt], kNbinsPull, kRPull[0], kRPull[1]);

      sN = (TString) "pull_t_vs_t" + nsuff;
      sT = (TString) "Pulls for Time" + tsuff + ";t_{MC} [ns];(t_{reco} - t_{MC}) / #sigma_{t}^{reco}";
      fvph_pull_t_vs_t[iSt] = MakeQaObject<TH2F>(sF + "pull/" + sN, sT, fNbins, 0, 0, fNbins, kRPull[0], kRPull[1]);

      sN                       = (TString) "reco_eff_vs_xy" + nsuff;
      sT                       = (TString) "Hit rec. efficiency in XY" + tsuff + ";x_{MC} [cm];y_{MC} [cm];#epsilon";
      fvpe_reco_eff_vs_xy[iSt] = MakeQaObject<TProfile2D>(sF + "eff/" + sN, sT, fNbins, frXmin[iSt], frXmax[iSt],
                                                          fNbins, frYmin[iSt], frYmax[iSt]);

      sN                 = (TString) "reco_eff" + nsuff;
      sT                 = (TString) "Hit rec. efficiency in XY bins" + tsuff + ";eff";
      fvph_reco_eff[iSt] = MakeQaObject<TH1F>(sF + "eff/" + sN, sT, 130, -0.005, 1.30 - 0.005);
    }  // stations
  }
  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
void CbmCaInputQaBase<DetID>::CreateSummary()
{
  /// TODO: Move this fuction somewhere on top of the class
  gStyle->SetOptFit(1);
  int nSt = fpDetInterface->GetNtrackingStations();

  // ********************
  // ** Drawing options

  // Contours
  constexpr auto contColor = kOrange + 7;
  constexpr auto contWidth = 2;  // Line width
  constexpr auto contStyle = 2;  // Line style
  constexpr auto contFill  = 0;  // Fill style

  // **********************
  // ** summary for all stations

  {
    auto* canv = MakeQaObject<TCanvas>("occ_hit", "Hit Occupancy", 1600, 800);
    canv->Divide(1, 3);
    canv->cd(1);
    fvph_hit_xy[nSt]->DrawCopy("colz", "");
    canv->cd(2);
    fvph_hit_zx[nSt]->DrawCopy("colz", "");
    canv->cd(3);
    fvph_hit_zy[nSt]->DrawCopy("colz", "");
  }

  if (IsMCUsed()) {
    auto* canv = MakeQaObject<TCanvas>("occ_point", "Point Occupancy", 1600, 800);
    canv->Divide(1, 3);
    canv->cd(1);
    fvph_point_xy[nSt]->DrawCopy("colz", "");
    canv->cd(2);
    fvph_point_zx[nSt]->DrawCopy("colz", "");
    canv->cd(3);
    fvph_point_zy[nSt]->DrawCopy("colz", "");
  }

  if (IsMCUsed()) {
    auto* canv = MakeQaObject<TCanvas>("residual", "Hit Residuals", 1600, 800);
    canv->Divide(1, 5);
    canv->cd(1);
    fvph_res_x[nSt]->DrawCopy("colz", "");
    canv->cd(2);
    fvph_res_y[nSt]->DrawCopy("colz", "");
    canv->cd(3);
    fvph_res_t[nSt]->DrawCopy("colz", "");
    canv->cd(4);
    fvph_res_u[nSt]->DrawCopy("colz", "");
    canv->cd(5);
    fvph_res_v[nSt]->DrawCopy("colz", "");
  }

  if (IsMCUsed()) {
    auto* canv = MakeQaObject<TCanvas>("pull", "Hit Pulls", 1600, 800);
    canv->Divide(1, 5);
    canv->cd(1);
    fvph_pull_x[nSt]->DrawCopy("colz", "");
    canv->cd(2);
    fvph_pull_y[nSt]->DrawCopy("colz", "");
    canv->cd(3);
    fvph_pull_t[nSt]->DrawCopy("colz", "");
    canv->cd(4);
    fvph_pull_u[nSt]->DrawCopy("colz", "");
    canv->cd(5);
    fvph_pull_v[nSt]->DrawCopy("colz", "");
  }

  if (IsMCUsed()) {
    auto* canv = MakeQaObject<TCanvas>("eff", "Hit Reconstruction Efficiency", 1600, 800);
    canv->Divide(1, 2);
    canv->cd(1);
    fvpe_reco_eff_vs_xy[nSt]->DrawCopy("colz", "");
    canv->cd(2);
    fvph_reco_eff[nSt]->DrawCopy("colz", "");
  }

  {
    auto* canv = MakeQaObject<TCanvas>("err", "Hit Errors", 1600, 800);
    canv->Divide(2, 3);
    canv->cd(1);
    fvph_hit_dx[nSt]->DrawCopy();
    canv->cd(2);
    fvph_hit_dy[nSt]->DrawCopy();
    canv->cd(3);
    fvph_hit_dt[nSt]->DrawCopy();
    canv->cd(4);
    fvph_hit_du[nSt]->DrawCopy();
    canv->cd(5);
    fvph_hit_dv[nSt]->DrawCopy();
    canv->cd(6);
    fvph_hit_kuv[nSt]->DrawCopy();
  }

  // TODO: Split the histograms for MC and real data
  if (IsMCUsed()) {
    auto* canv = MakeQaObject<TCanvas>("other", "Other histograms", 1600, 800);
    canv->Divide(1, 3);
    canv->cd(1);
    fvph_n_points_per_hit[nSt]->DrawCopy("colz", "");
    canv->cd(2);
    fvph_hit_station_delta_z[nSt]->DrawCopy("colz", "");
    canv->cd(3);
    fvph_point_hit_delta_z[nSt]->DrawCopy("colz", "");
  }


  // *********************************
  // ** Hit occupancies

  {  // ** Occupancy in XY plane
    auto* canv = MakeQaObject<TCanvas>("vs Station/hit_xy", "", 1600, 800);
    canv->DivideSquare(nSt);
    for (int iSt = 0; iSt < nSt; ++iSt) {
      canv->cd(iSt + 1);
      fvph_hit_xy[iSt]->DrawCopy("colz", "");

      // Square boarders of the active area of the station
      double stXmin = fpDetInterface->GetXmin(iSt);
      double stXmax = fpDetInterface->GetXmax(iSt);
      double stYmin = fpDetInterface->GetYmin(iSt);
      double stYmax = fpDetInterface->GetYmax(iSt);

      auto* pBox = new TBox(stXmin, stYmin, stXmax, stYmax);
      pBox->SetLineWidth(contWidth);
      pBox->SetLineStyle(contStyle);
      pBox->SetLineColor(contColor);
      pBox->SetFillStyle(contFill);
      pBox->Draw("SAME");
    }
  }

  {  // ** Occupancy in XZ plane
    auto* canv = MakeQaObject<TCanvas>("vs Station/hit_zx", "", 1600, 800);
    canv->cd();
    fvph_hit_zx[nSt]->DrawCopy("colz", "");
    for (int iSt = 0; iSt < nSt; ++iSt) {
      // Station positions in detector IFS
      double stZmin = fpDetInterface->GetZmin(iSt);
      double stZmax = fpDetInterface->GetZmax(iSt);
      double stHmin = fpDetInterface->GetXmin(iSt);
      double stHmax = fpDetInterface->GetXmax(iSt);

      auto* pBox = new TBox(stZmin, stHmin, stZmax, stHmax);
      pBox->SetLineWidth(contWidth);
      pBox->SetLineStyle(contStyle);
      pBox->SetLineColor(contColor);
      pBox->SetFillStyle(contFill);
      pBox->Draw("SAME");
    }
  }

  {  // ** Occupancy in YZ plane
    auto* canv = MakeQaObject<TCanvas>("vs Station/hit_zy", "", 1600, 800);
    canv->cd();
    fvph_hit_zy[nSt]->DrawCopy("colz", "");
    for (int iSt = 0; iSt < nSt; ++iSt) {
      // Station positions in detector IFS
      double stZmin = fpDetInterface->GetZmin(iSt);
      double stZmax = fpDetInterface->GetZmax(iSt);
      double stHmin = fpDetInterface->GetYmin(iSt);
      double stHmax = fpDetInterface->GetYmax(iSt);

      auto* pBox = new TBox(stZmin, stHmin, stZmax, stHmax);
      pBox->SetLineWidth(contWidth);
      pBox->SetLineStyle(contStyle);
      pBox->SetLineColor(contColor);
      pBox->SetFillStyle(contFill);
      pBox->Draw("SAME");
    }
  }

  // ************
  // ************

  if (IsMCUsed()) {

    // **********************
    // ** Point occupancies

    {  // ** Occupancy in XY plane
      auto* canv = MakeQaObject<TCanvas>("vs Station/point_xy", "", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_point_xy[iSt]->DrawCopy("colz", "");

        // Square boarders of the active area of the station
        double stXmin = fpDetInterface->GetXmin(iSt);
        double stXmax = fpDetInterface->GetXmax(iSt);
        double stYmin = fpDetInterface->GetYmin(iSt);
        double stYmax = fpDetInterface->GetYmax(iSt);

        auto* pBox = new TBox(stXmin, stYmin, stXmax, stYmax);
        pBox->SetLineWidth(contWidth);
        pBox->SetLineStyle(contStyle);
        pBox->SetLineColor(contColor);
        pBox->SetFillStyle(contFill);
        pBox->Draw("SAME");
      }
    }

    {  // ** Occupancy in XZ plane
      auto* canv = MakeQaObject<TCanvas>("vs Station/point_zx", "", 1600, 800);
      canv->cd();
      fvph_point_zx[nSt]->DrawCopy("colz", "");
      for (int iSt = 0; iSt < nSt; ++iSt) {
        // Station positions in detector IFS
        double stZmin = fpDetInterface->GetZmin(iSt);
        double stZmax = fpDetInterface->GetZmax(iSt);
        double stHmin = fpDetInterface->GetXmin(iSt);
        double stHmax = fpDetInterface->GetXmax(iSt);

        auto* pBox = new TBox(stZmin, stHmin, stZmax, stHmax);
        pBox->SetLineWidth(contWidth);
        pBox->SetLineStyle(contStyle);
        pBox->SetLineColor(contColor);
        pBox->SetFillStyle(contFill);
        pBox->Draw("SAME");
      }
    }

    {  // ** Occupancy in YZ plane
      auto* canv = MakeQaObject<TCanvas>("vs Station/point_zy", "", 1600, 800);
      canv->cd();
      fvph_point_zy[nSt]->DrawCopy("colz", "");
      for (int iSt = 0; iSt < nSt; ++iSt) {
        // Station positions in detector IFS
        double stZmin = fpDetInterface->GetZmin(iSt);
        double stZmax = fpDetInterface->GetZmax(iSt);
        double stHmin = fpDetInterface->GetYmin(iSt);
        double stHmax = fpDetInterface->GetYmax(iSt);

        auto* pBox = new TBox(stZmin, stHmin, stZmax, stHmax);
        pBox->SetLineWidth(contWidth);
        pBox->SetLineStyle(contStyle);
        pBox->SetLineColor(contColor);
        pBox->SetFillStyle(contFill);
        pBox->Draw("SAME");
      }
    }

    // **************
    // ** Residuals

    {  // x-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/res_x", "Residuals for x coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_res_x[iSt]->DrawCopy("", "");
      }
    }

    {  // y-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/res_y", "Residuals for y coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_res_y[iSt]->DrawCopy("", "");
      }
    }

    {  // u-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/res_u", "Residuals for u coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_res_u[iSt]->DrawCopy("", "");
      }
    }

    {  // v-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/res_v", "Residuals for v coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_res_v[iSt]->DrawCopy("", "");
      }
    }

    {  // t-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/res_t", "Residuals for time", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_res_t[iSt]->DrawCopy("", "");
      }
    }


    // **********
    // ** Pulls

    {  // x-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/pull_x", "Pulls for x coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_pull_x[iSt]->DrawCopy("", "");
      }
    }

    {  // y-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/pull_y", "Pulls for y coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_pull_y[iSt]->DrawCopy("", "");
      }
    }

    {  // u-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/pull_u", "Pulls for u coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_pull_u[iSt]->DrawCopy("", "");
      }
    }

    {  // v-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/pull_v", "Pulls for v coordinate", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_pull_v[iSt]->DrawCopy("", "");
      }
    }

    {  // t-coordinate
      auto* canv = MakeQaObject<TCanvas>("vs Station/pull_t", "Pulls for time", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvph_pull_t[iSt]->DrawCopy("", "");
      }
    }

    // ************************************
    // ** Hit reconstruction efficiencies

    {
      auto* canv = MakeQaObject<TCanvas>("vs Station/reco_eff", "Hit efficiencies in xy bins", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        //fvph_reco_eff[iSt]->SetMarkerStyle(20);
        fvph_reco_eff[iSt]->DrawCopy();
      }
    }

    {
      auto* canv = MakeQaObject<TCanvas>("vs Station/reco_eff_vs_xy", "Hit efficiencies wrt x and y", 1600, 800);
      canv->DivideSquare(nSt);
      for (int iSt = 0; iSt < nSt; ++iSt) {
        canv->cd(iSt + 1);
        fvpe_reco_eff_vs_xy[iSt]->DrawCopy("colz", "");
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
bool CbmCaInputQaBase<DetID>::IsTrackSelected(const CbmMCTrack* track, const Point_t* point) const
{
  assert(point);

  if (fConfig.fMcTrackCuts.fbPrimary && track->GetMotherId() >= 0) {
    return false;
  }

  double px = std::numeric_limits<double>::signaling_NaN();
  double py = std::numeric_limits<double>::signaling_NaN();
  double pz = std::numeric_limits<double>::signaling_NaN();
  if constexpr (ca::EDetectorID::kTof == DetID) {
    px = point->GetPx();
    py = point->GetPy();
    pz = point->GetPz();
  }
  else {
    px = point->GetPxOut();
    py = point->GetPyOut();
    pz = point->GetPzOut();
  }
  double p = sqrt(px * px + py * py + pz * pz);

  if (pz <= 0.) {
    return false;
  }

  if (p < fConfig.fMcTrackCuts.fMinMom) {
    return false;
  }

  if (TMath::ATan2(sqrt(px * px + py * py), pz) * TMath::RadToDeg() > fConfig.fMcTrackCuts.fMaxTheta) {
    return false;
  }

  return true;
}

template class CbmCaInputQaBase<ca::EDetectorID::kMvd>;
template class CbmCaInputQaBase<ca::EDetectorID::kSts>;
template class CbmCaInputQaBase<ca::EDetectorID::kMuch>;
template class CbmCaInputQaBase<ca::EDetectorID::kTrd>;
template class CbmCaInputQaBase<ca::EDetectorID::kTof>;
