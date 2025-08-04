/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaInputQaSetup.cxx
/// @brief  QA task for tracking detector interfaces (implementation)
/// @since  28.08.2023
/// @author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaSetup.h"

#include "CbmCaParametersHandler.h"
#include "CbmL1DetectorID.h"
#include "CbmMCDataManager.h"
#include "CbmSetup.h"
#include "FairRootManager.h"
#include "TAxis.h"

#include <Logger.h>

using cbm::algo::ca::InitManager;
using cbm::algo::ca::Parameters;
using cbm::ca::InputQaSetup;

// ---------------------------------------------------------------------------------------------------------------------
//
InputQaSetup::InputQaSetup(int verbose, bool isMCUsed) : CbmQaTask("CbmCaInputQaSetup", verbose, isMCUsed) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void InputQaSetup::Check() {}


// ---------------------------------------------------------------------------------------------------------------------
//
void InputQaSetup::CheckInit() const
{
  if (IsMCUsed() && !fpMCEventHeader) {
    throw std::logic_error("MC event header branch is unavailable");
  }
  for (int iD = 0; iD < static_cast<int>(ca::EDetectorID::END); ++iD) {
    if (fvbUseDet[iD]) {
      if (!fvpBrHits[iD]) {
        throw std::logic_error(Form("Hit branch is unavailable for %s", kDetName[iD]));
      }
      if (IsMCUsed() && !fvpBrPoints[iD]) {
        throw std::logic_error(Form("MC point branch is unavailable for %s", kDetName[iD]));
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void InputQaSetup::CheckoutDetectors()
{
  auto CheckDetector = [&](ca::EDetectorID detID) {
    if (CbmSetup::Instance()->IsActive(ToCbmModuleId(detID))) {
      fvbUseDet[detID] = true;
    }
    L_(info) << fName << ": " << ToString(ToCbmModuleId(detID)) << " " << fvbUseDet[detID];
  };
  CheckDetector(ca::EDetectorID::kMvd);
  CheckDetector(ca::EDetectorID::kSts);
  CheckDetector(ca::EDetectorID::kMuch);
  CheckDetector(ca::EDetectorID::kTrd);
  CheckDetector(ca::EDetectorID::kTof);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void InputQaSetup::CreateSummary()
{
  // Tracking setup by hits
  {
    auto* canv = MakeQaObject<TCanvas>("c_setup_hits", "Setup by hits", 1000, 1000);
    canv->Divide(1, 2, 0.000001, 0.000001);
    canv->cd(1);
    gPad->SetLogz();
    gPad->SetBottomMargin(0.000001);
    gPad->SetRightMargin(0.05);
    gPad->SetTitle("");
    auto* ph_hit_xz = dynamic_cast<TH2F*>(fph_hit_xz.back()->Clone("h_hit_xz_clone"));
    ph_hit_xz->SetTitle(";z [cm];x [cm]");
    ph_hit_xz->Draw("col");
    this->PutSetupNameOnPad(0.08, 0.83, 0.50, 0.89);
    canv->cd(2);
    gPad->SetLogz();
    gPad->SetTopMargin(0.000001);
    gPad->SetRightMargin(0.05);
    gPad->SetTitle("");
    auto* ph_hit_yz = dynamic_cast<TH2F*>(fph_hit_yz.back()->Clone("h_hit_yz_clone"));
    ph_hit_yz->SetTitle(";z [cm];y [cm]");
    ph_hit_yz->Draw("col");
    this->PutSetupNameOnPad(0.08, 0.93, 0.50, 0.99);


    auto LoBinEdge = [](TAxis* pAxis, double val) { return pAxis->GetBinLowEdge(pAxis->FindBin(val)); };
    auto UpBinEdge = [](TAxis* pAxis, double val) { return pAxis->GetBinUpEdge(pAxis->FindBin(val)); };

    for (int iDet = 0; iDet < static_cast<int>(fvpDetInterface.size()); ++iDet) {
      if (!fvbUseDet[iDet]) {
        continue;
      }
      int nSt = fvpDetInterface[iDet]->GetNtrackingStations();
      for (int iSt = 0; iSt < nSt; ++iSt) {
        int iStActive       = fpParameters->GetStationIndexActive(iSt, static_cast<ca::EDetectorID>(iDet));
        Color_t boxColor    = (iStActive < 0) ? kGray + 1 : kOrange + 2;
        const char* detName = Form("%s-%d", fvpDetInterface[iDet]->GetDetectorName().c_str(), iSt);
        {
          double zMin = LoBinEdge(fph_hit_xz.back()->GetXaxis(), fvZmin[iDet][iSt]);
          double zMax = UpBinEdge(fph_hit_xz.back()->GetXaxis(), fvZmax[iDet][iSt]);
          double xMin = LoBinEdge(fph_hit_xz.back()->GetYaxis(), fvXmin[iDet][iSt]);
          double xMax = UpBinEdge(fph_hit_xz.back()->GetYaxis(), fvXmax[iDet][iSt]);
          canv->cd(1);
          auto* pBox = new TBox(zMin, xMin, zMax, xMax);
          pBox->SetFillStyle(0);
          pBox->SetLineWidth(2);
          pBox->SetLineColor(boxColor);
          pBox->Draw("same");
          auto* pText = new TText(zMin, xMax, detName);
          pText->SetTextColor(boxColor);
          pText->SetTextSize(0.035);
          pText->SetTextAngle(45);
          pText->Draw("same");
        }
        {
          double zMin = LoBinEdge(fph_hit_yz.back()->GetXaxis(), fvZmin[iDet][iSt]);
          double zMax = UpBinEdge(fph_hit_yz.back()->GetXaxis(), fvZmax[iDet][iSt]);
          double yMin = LoBinEdge(fph_hit_yz.back()->GetYaxis(), fvYmin[iDet][iSt]);
          double yMax = UpBinEdge(fph_hit_yz.back()->GetYaxis(), fvYmax[iDet][iSt]);
          canv->cd(2);
          auto* pBox = new TBox(zMin, yMin, zMax, yMax);
          pBox->SetFillStyle(0);
          pBox->SetLineWidth(2);
          pBox->SetLineColor(boxColor);
          pBox->Draw("same");
          auto* pText = new TText(zMin, yMax, detName);
          pText->SetTextSize(0.035);
          pText->SetTextColor(boxColor);
          pText->SetTextAngle(45);
          pText->Draw("same");
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------------------------------------------
//
template<cbm::algo::ca::EDetectorID DetID>
void InputQaSetup::FillHistogramsDet()
{
  using Hit_t     = HitTypes_t::at<DetID>;
  using McPoint_t = PointTypes_t::at<DetID>;
  int nHits       = fvpBrHits[DetID]->GetEntriesFast();

  for (int iH = 0; iH < nHits; ++iH) {
    const auto* pHit = dynamic_cast<const Hit_t*>(fvpBrHits[DetID]->At(iH));
    if (!pHit) {
      LOG(warn) << fName << ": hit with iH = " << iH << " for detector " << kDetName[DetID] << " is not found";
    }
    auto address = pHit->GetAddress();

    // skip Bmon hits (rule?)
    if constexpr (ca::EDetectorID::kTof == DetID) {
      if (5 == CbmTofAddress::GetSmType(address)) {
        continue;
      }
    }

    int iStLoc = fvpDetInterface[DetID]->GetTrackingStationIndex(address);
    if (iStLoc < 0) {
      continue;
    }

    int iStGeo = fpParameters->GetStationIndexGeometry(iStLoc, DetID);
    auto xHit  = pHit->GetX();
    auto yHit  = pHit->GetY();
    auto zHit  = pHit->GetZ();
    if (fvXmin[DetID][iStLoc] > xHit) {
      fvXmin[DetID][iStLoc] = xHit;
    }
    if (fvXmax[DetID][iStLoc] < xHit) {
      fvXmax[DetID][iStLoc] = xHit;
    }
    if (fvYmin[DetID][iStLoc] > yHit) {
      fvYmin[DetID][iStLoc] = yHit;
    }
    if (fvYmax[DetID][iStLoc] < yHit) {
      fvYmax[DetID][iStLoc] = yHit;
    }
    if (fvZmin[DetID][iStLoc] > zHit) {
      fvZmin[DetID][iStLoc] = zHit;
    }
    if (fvZmax[DetID][iStLoc] < zHit) {
      fvZmax[DetID][iStLoc] = zHit;
    }
    if (iStGeo >= 0) {
      fph_hit_xy[iStGeo]->Fill(xHit, yHit);
      fph_hit_xz[iStGeo]->Fill(zHit, xHit);
      fph_hit_yz[iStGeo]->Fill(zHit, yHit);
    }
    fph_hit_xz.back()->Fill(zHit, xHit);
    fph_hit_yz.back()->Fill(zHit, yHit);
  }  // iH

  if (IsMCUsed()) {
    int nMCevents = fpMCEventList->GetNofEvents();
    for (int iE = 0; iE < nMCevents; ++iE) {
      int iFile   = fpMCEventList->GetFileIdByIndex(iE);
      int iEvent  = fpMCEventList->GetEventIdByIndex(iE);
      int nPoints = fvpBrPoints[DetID]->Size(iFile, iEvent);

      for (int iP = 0; iP < nPoints; ++iP) {
        const auto* pPoint = dynamic_cast<const McPoint_t*>(fvpBrPoints[DetID]->Get(iFile, iEvent, iP));
        if (!pPoint) {
          LOG(error) << fName << ": point with iFile=" << iFile << ", iEvent=" << iEvent << ", iP=" << iP
                     << " for detector " << kDetName[DetID] << " is not found";
          continue;
        }
        auto address = pPoint->GetDetectorID();

        int iStLoc = fvpDetInterface[DetID]->GetTrackingStationIndex(address);
        if (iStLoc < 0) {
          continue;
        }

        int iStGeo  = fpParameters->GetStationIndexGeometry(iStLoc, DetID);
        auto xPoint = pPoint->FairMCPoint::GetX();
        auto yPoint = pPoint->FairMCPoint::GetY();
        auto zPoint = pPoint->FairMCPoint::GetZ();

        if (iStGeo >= 0) {
          fph_point_xy[iStGeo]->Fill(xPoint, yPoint);
          fph_point_xz[iStGeo]->Fill(zPoint, xPoint);
          fph_point_yz[iStGeo]->Fill(zPoint, yPoint);
        }
        fph_point_xz.back()->Fill(zPoint, xPoint);
        fph_point_yz.back()->Fill(zPoint, yPoint);
      }  //  iP
    }    // iE
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void InputQaSetup::ExecQa()
{
  if (fvbUseDet[ca::EDetectorID::kMvd]) {
    this->FillHistogramsDet<ca::EDetectorID::kMvd>();
  }
  if (fvbUseDet[ca::EDetectorID::kSts]) {
    this->FillHistogramsDet<ca::EDetectorID::kSts>();
  }
  if (fvbUseDet[ca::EDetectorID::kMuch]) {
    this->FillHistogramsDet<ca::EDetectorID::kMuch>();
  }
  if (fvbUseDet[ca::EDetectorID::kTrd]) {
    this->FillHistogramsDet<ca::EDetectorID::kTrd>();
  }
  if (fvbUseDet[ca::EDetectorID::kTof]) {
    this->FillHistogramsDet<ca::EDetectorID::kTof>();
  }
}


// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus InputQaSetup::InitQa()
try {
  LOG(info) << fName << ": initializing... ";
  CheckoutDetectors();

  // Tracking parameters
  fpParameters = cbm::ca::ParametersHandler::Instance()->Get(fsParametersFilename);
  LOG(info) << fName << ": parameters instance, reference: " << fpParameters.use_count();

  auto pFairManager = FairRootManager::Instance();
  assert(pFairManager);

  auto pMcManager = IsMCUsed() ? dynamic_cast<CbmMCDataManager*>(pFairManager->GetObject("MCDataManager")) : nullptr;
  if (IsMCUsed()) {
    fpMCEventList   = dynamic_cast<CbmMCEventList*>(pFairManager->GetObject("MCEventList."));
    fpMCEventHeader = pMcManager->GetObject("MCEventHeader.");
  }

  fvpBrPoints.fill(nullptr);
  fvpBrHits.fill(nullptr);

  auto InitDetInterface = [&](CbmTrackingDetectorInterfaceBase* ifs, ca::EDetectorID detID) {
    if (fvbUseDet[detID]) {
      fvpDetInterface[detID] = ifs;
      int nSt                = ifs->GetNtrackingStations();
      fvXmin[detID].resize(nSt, std::numeric_limits<double>::max());
      fvXmax[detID].resize(nSt, std::numeric_limits<double>::lowest());
      fvYmin[detID].resize(nSt, std::numeric_limits<double>::max());
      fvYmax[detID].resize(nSt, std::numeric_limits<double>::lowest());
      fvZmin[detID].resize(nSt, std::numeric_limits<double>::max());
      fvZmax[detID].resize(nSt, std::numeric_limits<double>::lowest());
    }
  };

  auto InitHitBranch = [&](const char* brName, ca::EDetectorID detID) {
    if (fvbUseDet[detID]) {
      fvpBrHits[detID] = dynamic_cast<TClonesArray*>(pFairManager->GetObject(brName));
      if (!fvpBrHits[detID]) {
        fvbUseDet[detID] = false;  // Disable detectors without hits
      }
    }
    return static_cast<bool>(fvpBrHits[detID]);
  };

  auto InitPointBranch = [&](const char* brName, ca::EDetectorID detID) {
    if (IsMCUsed() && fvbUseDet[detID]) {
      fvpBrPoints[detID] = pMcManager->InitBranch(brName);
    }
  };

  InitDetInterface(CbmMvdTrackingInterface::Instance(), ca::EDetectorID::kMvd);
  InitDetInterface(CbmStsTrackingInterface::Instance(), ca::EDetectorID::kSts);
  InitDetInterface(CbmMuchTrackingInterface::Instance(), ca::EDetectorID::kMuch);
  InitDetInterface(CbmTrdTrackingInterface::Instance(), ca::EDetectorID::kTrd);
  InitDetInterface(CbmTofTrackingInterface::Instance(), ca::EDetectorID::kTof);

  InitHitBranch("MvdHit", ca::EDetectorID::kMvd);
  InitHitBranch("StsHit", ca::EDetectorID::kSts);
  InitHitBranch("MuchPixelHit", ca::EDetectorID::kMuch);
  InitHitBranch("TrdHit", ca::EDetectorID::kTrd);
  if (!InitHitBranch("TofCalHit", ca::EDetectorID::kTof)) {
    InitHitBranch("TofHit", ca::EDetectorID::kTof);
  }

  InitPointBranch("MvdPoint", ca::EDetectorID::kMvd);
  InitPointBranch("StsPoint", ca::EDetectorID::kSts);
  InitPointBranch("MuchPoint", ca::EDetectorID::kMuch);
  InitPointBranch("TrdPoint", ca::EDetectorID::kTrd);
  InitPointBranch("TofPoint", ca::EDetectorID::kTof);

  this->CheckInit();


  int nStGeo = fpParameters->GetNstationsGeometry();

  MakeQaDirectory("hit_occupancy");
  for (int iD = 0; iD < static_cast<int>(ca::EDetectorID::END); ++iD) {
    if (fvbUseDet[iD]) {
      MakeQaDirectory(Form("hit_occupancy/%s", kDetName[iD]));
    }
  }
  MakeQaDirectory("point_occupancy");
  for (int iD = 0; iD < static_cast<int>(ca::EDetectorID::END); ++iD) {
    if (fvbUseDet[iD]) {
      MakeQaDirectory(Form("point_occupancy/%s", kDetName[iD]));
    }
  }

  fph_hit_xy.resize(nStGeo);
  fph_hit_xz.resize(nStGeo + 1);
  fph_hit_yz.resize(nStGeo + 1);
  fph_point_xy.resize(nStGeo);
  fph_point_xz.resize(nStGeo + 1);
  fph_point_yz.resize(nStGeo + 1);

  /// TEMPORARY
  constexpr int kNbinsZ = 300;
  constexpr int kNbinsX = 200;
  constexpr int kNbinsY = 200;
  constexpr float kMinZ = -5.;
  constexpr float kMaxZ = 350.;
  constexpr float kMinX = -100.;
  constexpr float kMaxX = 100.;
  constexpr float kMinY = -100.;
  constexpr float kMaxY = 100.;

  for (int iStGeo = 0; iStGeo <= nStGeo; ++iStGeo) {
    auto [detID, iStLoc] = fpParameters->GetStationIndexLocal(iStGeo);
    TString sF           = "";
    TString sN           = "";
    TString sT           = "";
    TString sNsuff       = (iStGeo == nStGeo) ? "" : Form("_st_%s%d", kDetName[detID], iStLoc);
    TString sTsuff       = (iStGeo == nStGeo) ? "" : Form(" in %s station %d", kDetName[detID], iStLoc);

    // Hit occupancy
    sF                 = Form("hit_occupancy%s", ((iStGeo == nStGeo) ? "" : Form("/%s", kDetName[detID])));
    sN                 = Form("hit_xz%s", sNsuff.Data());
    sT                 = Form("hit occupancy in xz-plane%s;z_{hit} [cm];x_{hit} [cm]", sTsuff.Data());
    fph_hit_xz[iStGeo] = MakeQaObject<TH2F>(sF + "/" + sN, sT, kNbinsZ, kMinZ, kMaxZ, kNbinsX, kMinX, kMaxX);

    sN                 = Form("hit_yz%s", sNsuff.Data());
    sT                 = Form("hit occupancy in yz-plane%s;z_{hit} [cm];y_{hit} [cm]", sTsuff.Data());
    fph_hit_yz[iStGeo] = MakeQaObject<TH2F>(sF + "/" + sN, sT, kNbinsZ, kMinZ, kMaxZ, kNbinsY, kMinY, kMaxY);

    if (iStGeo < nStGeo) {
      sN                 = Form("hit_xy%s", sNsuff.Data());
      sT                 = Form("hit occupancy in xy-plane%s;x_{hit} [cm];y_{hit} [cm]", sTsuff.Data());
      fph_hit_xy[iStGeo] = MakeQaObject<TH2F>(sF + "/" + sN, sT, kNbinsX, kMinX, kMaxX, kNbinsY, kMinY, kMaxY);
    }

    if (IsMCUsed()) {
      // Point occupancy
      sF                   = Form("point_occupancy%s", ((iStGeo == nStGeo) ? "" : Form("/%s", kDetName[detID])));
      sN                   = Form("point_xz%s", sNsuff.Data());
      sT                   = Form("point occupancy in xz-plane%s;z_{point} [cm];x_{point} [cm]", sTsuff.Data());
      fph_point_xz[iStGeo] = MakeQaObject<TH2F>(sF + "/" + sN, sT, kNbinsZ, kMinZ, kMaxZ, kNbinsX, kMinX, kMaxX);

      sN                   = Form("point_yz%s", sNsuff.Data());
      sT                   = Form("point occupancy in yz-plane%s;z_{point} [cm];y_{point} [cm]", sTsuff.Data());
      fph_point_yz[iStGeo] = MakeQaObject<TH2F>(sF + "/" + sN, sT, kNbinsZ, kMinZ, kMaxZ, kNbinsY, kMinY, kMaxY);

      if (iStGeo < nStGeo) {
        sN                   = Form("point_xy%s", sNsuff.Data());
        sT                   = Form("point occupancy in xy-plane%s;x_{point} [cm];y_{point} [cm]", sTsuff.Data());
        fph_point_xy[iStGeo] = MakeQaObject<TH2F>(sF + "/" + sN, sT, kNbinsX, kMinX, kMaxX, kNbinsY, kMinY, kMaxY);
      }
    }
  }


  auto CreateMaterialBudgetHistograms = [&](const auto& kfSetup, const TString& dir) {
    for (int iLayer = 0; iLayer < kfSetup.GetNofLayers(); ++iLayer) {
      const auto& matMap{kfSetup.GetMaterial(iLayer)};
      auto [detID, stationID] = kfSetup.GetIndexMap().GlobalToLocal(iLayer);
      TString sN              = Form("%s/mat_budget_%s_st%d", dir.Data(), kDetName[detID], stationID);
      TString sT =
        Form("Material budget map for %s station %d;x [cm];y [cm]; X/X_{0} [%%]", kDetName[detID], stationID);
      auto nBins{matMap.GetNbins()};
      auto xMin{-matMap.GetXYmax()};
      auto xMax{+matMap.GetXYmax()};
      auto* pHist = MakeQaObject<TH2F>(sN, sT, nBins, xMin, xMax, nBins, xMin, xMax);
      for (int iX = 0; iX < nBins; ++iX) {
        for (int iY = 0; iY < nBins; ++iY) {
          pHist->SetBinContent(iX + 1, iY + 1, 100. * matMap.template GetBinThicknessX0<float>(iX, iY));
        }
      }
    }
  };

  MakeQaDirectory("TrackingKFSetup");
  MakeQaDirectory("TrackingKFSetup/geometry");
  CreateMaterialBudgetHistograms(fpParameters->GetGeometrySetup(), "TrackingKFSetup/geometry");
  MakeQaDirectory("TrackingKFSetup/active");
  CreateMaterialBudgetHistograms(fpParameters->GetActiveSetup(), "TrackingKFSetup/active");

  LOG(info) << fName << ": initializing... \033[1;32mDone\033[0m";
  return kSUCCESS;
}
catch (const std::exception& e) {
  LOG(error) << fName << ": initializing... \033[1;31mFailed\033[0m\nReason: " << e.what();
  return kFATAL;
}
