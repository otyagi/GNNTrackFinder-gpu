/* Copyright (C) 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaQa.cxx
/// \date   20.11.2023
/// \brief  A QA module for CA tracking (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaQa.h"

#include "CaDefs.h"
#include "CaInputData.h"
#include "CaParameters.h"
#include "CaTrack.h"
#include "TrackingDefs.h"

#include <algorithm>
#include <fstream>
#include <limits>

#include <fmt/format.h>

using cbm::algo::ca::Hit;
using cbm::algo::ca::Qa;
using cbm::algo::ca::constants::math::Pi;

// ---------------------------------------------------------------------------------------------------------------------
//
bool Qa::CheckInit() const
{
  bool res = true;
  if (!fpParameters) {
    L_(error) << "cbm::algo::ca::OutputQa::Build(): parameters object is undefined";
    res = false;
  }
  if (!fpInputData) {
    L_(error) << "cbm::algo::ca::OutputQa::Build(): input data object is undefined";
    res = false;
  }
  if (!fpvTracks) {
    L_(error) << "cbm::algo::ca::OutputQa::Build(): track vector is undefined";
    res = false;
  }
  if (!fpvRecoHits) {
    L_(error) << "cbm::algo::ca::OutputQa::Build(): used hit index vector is undefined";
    res = false;
  }
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Qa::Init()
{
  using cbm::algo::qa::CanvasConfig;
  using cbm::algo::qa::Data;
  using cbm::algo::qa::H1D;
  using cbm::algo::qa::H2D;
  using cbm::algo::qa::PadConfig;
  using cbm::algo::qa::Prof2D;
  using fmt::format;

  if (!IsActive()) {
    return;
  }

  // ********************
  // ** Hit distributions
  int nSt = fpParameters->GetNstationsActive();  // Number of active tracking stations
  {
    // Occupancy distributions
    fvphHitOccupXY.resize(nSt);
    fvphHitOccupZX.resize(nSt);
    if (kDebug) {
      fvphHitOccupZY.resize(nSt);
      fvphHitUsageXY.resize(nSt);
    }

    // Station sizes in transverse plane
    std::vector<double> vMinX(nSt);
    std::vector<double> vMaxX(nSt);
    std::vector<double> vMinY(nSt);
    std::vector<double> vMaxY(nSt);

    int nBinsXY = 200;
    int nBinsZ  = 400;

    for (int iSt = 0; iSt < nSt; ++iSt) {
      const auto& station = fpParameters->GetStation(iSt);
      vMaxX[iSt]          = station.GetXmax<double>();
      vMinX[iSt]          = station.GetXmin<double>();
      vMaxY[iSt]          = station.GetYmax<double>();
      vMinY[iSt]          = station.GetYmin<double>();
      double dy           = (vMaxY[iSt] - vMinY[iSt]) * kXYZMargin;
      double dx           = (vMaxX[iSt] - vMinX[iSt]) * kXYZMargin;
      vMaxX[iSt] += dx;
      vMinX[iSt] -= dx;
      vMaxY[iSt] += dy;
      vMinY[iSt] -= dy;
    }
    // Station max
    double xMinA = *std::min_element(vMinX.begin(), vMinX.end());
    double xMaxA = *std::max_element(vMaxX.begin(), vMaxX.end());
    double yMinA = *std::min_element(vMinY.begin(), vMinY.end());
    double yMaxA = *std::max_element(vMaxY.begin(), vMaxY.end());
    double zMinA = fpParameters->GetStation(0).GetZ<double>();
    double zMaxA = fpParameters->GetStation(nSt - 1).GetZ<double>();
    {
      double dz = (zMaxA - zMinA) * kXYZMargin;
      zMinA -= dz;
      zMaxA += dz;
    }
    for (int iSt = 0; iSt < nSt; ++iSt) {
      int iStGeo           = fpParameters->GetActiveToGeoMap()[iSt];
      auto [detID, iStLoc] = fpParameters->GetGeoToLocalIdMap()[iStGeo];
      for (auto hitSet : kHitSets) {
        auto setNm = EHitSet::Input == hitSet ? "input" : "used";
        auto setTl = EHitSet::Input == hitSet ? "Input" : "Used";
        {
          auto name = format("hit_{}_occup_xy_sta_{}", setNm, iSt);
          auto titl = format("{} hit occupancy in XY plane for station {} ({}{});x [cm];y [cm]", setTl, iSt,
                             kDetName[detID], iStLoc);
          fvphHitOccupXY[iSt][hitSet] =
            MakeObj<H2D>(name, titl, nBinsXY, vMinX[iSt], vMaxX[iSt], nBinsXY, vMinY[iSt], vMaxY[iSt]);
        }
        {
          auto name = format("hit_{}_occup_zx_sta_{}", setNm, iSt);
          auto titl = format("{} hit occupancy in ZX plane for station {} ({}{});z [cm];x [cm]", setTl, iSt,
                             kDetName[detID], iStLoc);
          fvphHitOccupZX[iSt][hitSet] = MakeObj<H2D>(name, titl, nBinsZ, zMinA, zMaxA, nBinsXY, xMinA, xMaxA);
        }
        if (kDebug) {
          auto name = format("hit_{}_occup_zy_sta_{}", setNm, iSt);
          auto titl = format("{} hit occupancy in ZY plane for station {} ({}{});z [cm];y [cm]", setTl, iSt,
                             kDetName[detID], iStLoc);
          fvphHitOccupZY[iSt][hitSet] = MakeObj<H2D>(name, titl, nBinsZ, zMinA, zMaxA, nBinsXY, yMinA, yMaxA);
        }
      }
      if (kDebug) {
        auto name = format("hit_usage_xy_sta_{}", iSt);
        auto titl = format("Hit usage in XY plane for station {} ({}{});x [cm];y [cm]", iSt, kDetName[detID], iStLoc);
        fvphHitUsageXY[iSt] =
          MakeObj<Prof2D>(name, titl, nBinsXY, vMinX[iSt], vMaxX[iSt], nBinsXY, vMinY[iSt], vMaxY[iSt], 0., 1.);
      }
    }
    if (kDebug) {
      for (auto hitSet : kHitSets) {
        constexpr int NBins = 1000000;
        auto setNm          = EHitSet::Input == hitSet ? "input" : "used";
        auto setTl          = EHitSet::Input == hitSet ? "Input" : "Used";
        {
          auto name                    = format("hit_{}_front_key_index", setNm);
          auto titl                    = format("{} hit front key index;ID_{{key}}/N_{{keys}};Count", setTl);
          fvphHitFrontKeyIndex[hitSet] = MakeObj<H1D>(name, titl, NBins, 0., 1.);
        }
        {
          auto name                   = format("hit_{}_back_key_index", setNm);
          auto titl                   = format("{} hit back key index;ID_{{key}}/N_{{keys}};Count", setTl);
          fvphHitBackKeyIndex[hitSet] = MakeObj<H1D>(name, titl, NBins, 0., 1.);
        }
      }
    }

    // Hit time distributions
    if (kDebug) {
      fvphHitTime.resize(nSt + 1);  // [nSt] - total over stations
      for (int iSt = 0; iSt < nSt + 1; ++iSt) {
        auto staNm = iSt == nSt ? "" : format("_sta_{}", iSt);
        auto staTl = iSt == nSt ? "" : format(" in station {}", iSt);
        for (auto hitSet : kHitSets) {
          auto setNm               = EHitSet::Input == hitSet ? "input" : "used";
          auto setTl               = EHitSet::Input == hitSet ? "Input" : "Used";
          auto name                = format("hit_{}_rel_time{}", setNm, staNm);
          auto titl                = format("{} hit relative time{}; #delta t_{{hit}};Count", setTl, staTl);
          fvphHitTime[iSt][hitSet] = MakeObj<H1D>(name, titl, 10000, -0.1, 1.1);
        }
      }
    }
  }

  // **********************
  // ** Track distributions
  std::vector<std::string> vsPointName = {"first", "last"};
  for (int i = 0; i < knTrkParPoints; ++i) {
    if (!kDebug && i > 0) {
      continue;
    }
    {
      auto sName      = format("{}_track_{}_theta", GetTaskName(), vsPointName[i]);
      auto sTitl      = format("#theta at {} hit; #theta", vsPointName[i]);
      fvphTrkTheta[i] = MakeObj<H1D>(sName, sTitl, 62, 0., 90.);
    }
    {
      auto sName    = format("{}_track_{}_phi", GetTaskName(), vsPointName[i]);
      auto sTitl    = format("#phi at {} hit; #phi", vsPointName[i]);
      fvphTrkPhi[i] = MakeObj<H1D>(sName, sTitl, 62, -180., 180.);
    }
    {
      auto sName         = format("{}_track_{}_thata_phi", GetTaskName(), vsPointName[i]);
      auto sTitl         = format("#theta vs #phi at {} hit; #phi; #theta", vsPointName[i]);
      fvphTrkPhiTheta[i] = MakeObj<H2D>(sName, sTitl, 62, -180., 180., 62, 0., 90.);
    }
    {
      auto sName        = format("{}_track_{}_chi2_ndf", GetTaskName(), vsPointName[i]);
      auto sTitl        = format("#chi^{{2}}/NDF at {} hit; #chi^{{2}}/NDF", vsPointName[i]);
      fvphTrkChi2Ndf[i] = MakeObj<H1D>(sName, sTitl, 100, 0., 20.);
    }
  }
  {
    int nBins   = knStaMax;
    double xMin = -0.5;
    double xMax = double(knStaMax) - 0.5;
    {
      auto sName      = format("{}_track_fst_lst_sta", GetTaskName());
      auto sTitl      = "First vs. last station index;ID^{last}_{station};ID^{first}_{station}";
      fphTrkFstLstSta = MakeObj<H2D>(sName, sTitl, nBins, xMin, xMax, nBins, xMin, xMax);
    }
    {
      auto sName     = format("{}_track_origin", GetTaskName());
      auto sTitl     = "Track origin;x [cm];y [cm]";
      fphTrkOriginXY = MakeObj<H2D>(sName, sTitl, kOriginB, kOriginL, kOriginU, kOriginB, kOriginL, kOriginU);
    }
    fphTrkNofHits = MakeObj<H1D>("n_hits", "Number of hits;N_{hit}", nBins, xMin, xMax);
  }

  // ---- Init canvases
  {
    // Input hits canvas
    {
      for (auto hitSet : kHitSets) {
        auto setNm = EHitSet::Input == hitSet ? "input" : "used";
        auto setTl = EHitSet::Input == hitSet ? "Input" : "Used";
        {  // XY
          auto name = format("{}_ca_hit_{}_occupancy_xy", GetTaskName(), setNm);
          auto titl = format("{} hit occupancy in different stations in XY plane", setNm);
          auto canv = CanvasConfig(name, titl);
          for (int iSt = 0; iSt < nSt; ++iSt) {
            auto pad = PadConfig(false, false, false, false, true);
            pad.RegisterHistogram(fvphHitOccupXY[iSt][hitSet], "colz");
            canv.AddPadConfig(pad);
          }
          AddCanvasConfig(canv);
        }
        {  // ZX and ZY
          auto name = format("{}_ca_hit_{}_occupancy_zx_zy", GetTaskName(), setNm);
          auto titl = format("{} hit occupancy in different stations in ZX and ZY planes", setTl);
          auto canv = CanvasConfig(name, titl);
          {  // ZX
            auto pad = PadConfig();
            for (int iSt = 0; iSt < nSt; ++iSt) {
              pad.RegisterHistogram(fvphHitOccupZX[iSt][hitSet], (iSt == 0 ? "colz" : "cols same"));
            }
            canv.AddPadConfig(pad);
          }
          if (kDebug) {  // ZY
            auto pad = PadConfig();
            for (int iSt = 0; iSt < nSt; ++iSt) {
              pad.RegisterHistogram(fvphHitOccupZY[iSt][hitSet], (iSt == 0 ? "colz" : "cols same"));
            }
            canv.AddPadConfig(pad);
          }
          AddCanvasConfig(canv);
        }
      }
      if (kDebug) {
        auto name = format("{}_ca_hit_usage_xy", GetTaskName());
        auto titl = format("Hit usage in different stations in XY plane");
        auto canv = CanvasConfig(name, titl);
        for (int iSt = 0; iSt < nSt; ++iSt) {
          auto pad = PadConfig();
          pad.RegisterHistogram(fvphHitUsageXY[iSt], "colz");
          canv.AddPadConfig(pad);
        }
        AddCanvasConfig(canv);
      }
    }

    // Tracks canvas
    {
      auto canv = CanvasConfig(format("{}_ca_tracks", GetTaskName()), "Tracking output QA");
      {
        auto pad = PadConfig(true, true, false, false, true);
        pad.RegisterHistogram(fvphTrkPhiTheta[0], "colz");
        canv.AddPadConfig(pad);
      }
      {
        auto pad = PadConfig(true, true, false, true, false);
        pad.RegisterHistogram(fvphTrkChi2Ndf[0], "hist");
        canv.AddPadConfig(pad);
      }
      {
        auto pad = PadConfig(true, true, false, true, false);
        pad.RegisterHistogram(fphTrkNofHits, "hist");
        canv.AddPadConfig(pad);
      }
      {
        auto pad = PadConfig(true, true, false, false, true);
        pad.RegisterHistogram(fphTrkFstLstSta, "colz");
        canv.AddPadConfig(pad);
      }
      {
        auto pad = PadConfig(true, true, false, false, false);
        pad.RegisterHistogram(fphTrkOriginXY, "colz");
        canv.AddPadConfig(pad);
      }
      AddCanvasConfig(canv);
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Qa::Exec()
{
  if (!IsActive()) {
    return;
  }

  if (!CheckInit()) {
    L_(fatal) << "ca::Qa: instance is not initialized";
    assert(false);
  }

  int nHitsInput = fpInputData->GetNhits();
  // Map: if hit used in tracking
  std::vector<unsigned char> vbHitUsed(nHitsInput, false);
  for (int iH : (*fpvRecoHits)) {
    vbHitUsed[iH] = true;
  }

  // Calculate max and min hit time
  if constexpr (kDebug) {
    const auto& hits = fpInputData->GetHits();
    auto [minTimeIt, maxTimeIt] =
      std::minmax_element(hits.cbegin(), hits.cend(), [](const auto& h1, const auto& h2) { return h1.T() < h2.T(); });
    fMinHitTime = minTimeIt->T();
    fMaxHitTime = maxTimeIt->T();
  }

  // Fill input hit histograms
  {
    for (int iH = 0; iH < nHitsInput; ++iH) {
      const auto& hit = fpInputData->GetHit(iH);
      FillHitDistributionsForHitSet(EHitSet::Input, hit);
      if (vbHitUsed[iH]) {
        FillHitDistributionsForHitSet(EHitSet::Used, hit);
      }
      if constexpr (kDebug) {
        int iSt  = hit.Station();
        double x = hit.X();
        double y = hit.Y();
        fvphHitUsageXY[iSt]->Fill(x, y, static_cast<double>(vbHitUsed[iH]));
      }
    }
  }

  // Fill track histograms
  {
    int trkFirstHit = 0;  // Index of hit in fpvRecoHits
    for (auto trkIt = fpvTracks->begin(); trkIt != fpvTracks->end(); ++trkIt) {
      auto& track = *trkIt;
      int nHits   = track.fNofHits;
      // Indices of hits in fpInputData->GetHits()
      int iFstHit = (*fpvRecoHits)[trkFirstHit];
      int iLstHit = (*fpvRecoHits)[trkFirstHit + nHits - 1];

      // Distributions in different track points
      for (int ip = 0; ip < knTrkParPoints; ++ip) {
        if (!kDebug && ip > 0) {
          continue;
        }
        //int iHit           = (ip == 0 ? iFstHit : iLstHit);
        //const auto& hit    = fpInputData->GetHit(iHit);
        const auto& trkPar = (ip == 0 ? track.fParFirst : track.fParLast);
        fvphTrkTheta[ip]->Fill(trkPar.GetTheta() * 180. / Pi);
        fvphTrkPhi[ip]->Fill(trkPar.GetPhi() * 180. / Pi);
        fvphTrkPhiTheta[ip]->Fill(trkPar.GetPhi() * 180. / Pi, trkPar.GetTheta() * 180. / Pi);
        fvphTrkChi2Ndf[ip]->Fill(trkPar.GetChiSq() / trkPar.GetNdf());
      }
      // Other distributions
      fphTrkFstLstSta->Fill(fpInputData->GetHit(iLstHit).Station(), fpInputData->GetHit(iFstHit).Station());
      fphTrkNofHits->Fill(nHits);
      fphTrkOriginXY->Fill(track.fParPV.X(), track.fParPV.Y());
      trkFirstHit += nHits;
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Qa::FillHitDistributionsForHitSet(Qa::EHitSet hitSet, const Hit& hit)
{
  int nSt  = fpParameters->GetNstationsActive();
  int iSt  = hit.Station();
  double x = hit.X();
  double y = hit.Y();
  double z = hit.Z();
  fvphHitOccupXY[iSt][hitSet]->Fill(x, y);
  fvphHitOccupZX[iSt][hitSet]->Fill(z, x);
  if constexpr (kDebug) {
    fvphHitOccupZY[iSt][hitSet]->Fill(z, y);
    auto nKeys = static_cast<double>(fpInputData->GetNhitKeys());
    fvphHitFrontKeyIndex[hitSet]->Fill(hit.FrontKey() / nKeys);
    fvphHitBackKeyIndex[hitSet]->Fill(hit.BackKey() / nKeys);
    double relTime = (hit.T() - fMinHitTime) / (fMaxHitTime - fMinHitTime);
    fvphHitTime[iSt][hitSet]->Fill(relTime);
    fvphHitTime[nSt][hitSet]->Fill(relTime);
  }
}
