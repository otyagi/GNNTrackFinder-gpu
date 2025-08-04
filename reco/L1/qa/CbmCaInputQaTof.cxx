/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaTof.cxx
/// \date   30.01.2023
/// \brief  QA-task for CA tracking input from TOF detector (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "CbmCaInputQaTof.h"

#include "CaToolsLinkKey.h"
#include "CbmAddress.h"
#include "CbmMCDataArray.h"
#include "CbmMCEventList.h"
#include "CbmMatch.h"
#include "CbmTofHit.h"
#include "CbmTofPoint.h"
#include "CbmTofTrackingInterface.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "Logger.h"
#include "TBox.h"
#include "TClonesArray.h"
#include "TF1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMath.h"
#include "TStyle.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>

ClassImp(CbmCaInputQaTof);

// ---------------------------------------------------------------------------------------------------------------------
//
CbmCaInputQaTof::CbmCaInputQaTof(int verbose, bool isMCUsed) : CbmCaInputQaBase("CbmCaInputQaTof", verbose, isMCUsed)
{
  // Default parameters of task
  DefineParameters();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaTof::CreateSummary()
{
  // Base canvases
  CbmCaInputQaBase::CreateSummary();

  // Hit occupancy vs TOF cell
  {
    //auto DrawBox = [&](double xLo, double yLo, double xUp, double yUp) {
    //  auto* pBox = new TBox(xLo, yLo, xUp, yUp);
    //  pBox->SetLineWidth(kOrange + 7);
    //  pBox->SetLineStyle(2);
    //  pBox->SetLineColor(2);
    //  pBox->SetFillStyle(0);
    //  pBox->Draw("SAME");
    //};

    // NOTE: SZh 11.09.2023: Causes memory overconsumption
    //for (int iSt = 0; iSt < fpDetInterface->GetNtrackingStations(); ++iSt) {
    //  for (int iSmType = 0; iSmType < fDigiBdfPar->GetNbSmTypes(); ++iSmType) {
    //    if (iSmType == 5) { continue; }  // skip Bmon
    //    for (int iSm = 0; iSm < fDigiBdfPar->GetNbSm(iSmType); ++iSm) {
    //      for (int iRpc = 0; iRpc < fDigiBdfPar->GetNbRpc(iSmType); ++iRpc) {
    //        for (int iCh = 0; iCh < fDigiBdfPar->GetNbChan(iSmType, iRpc); ++iCh) {
    //          const char* dir = "occup_cell/";
    //          TString name    = Form("%s/occup_xy_smt%d_sm%d_rpc%d_ch%d", dir, iSmType, iSm, iRpc, iCh);

    //          auto address      = CbmTofAddress::GetUniqueAddress(iSm, iRpc, iCh, /*side = */ 0, iSmType);
    //          const auto* pCell = dynamic_cast<const CbmTofCell*>(fDigiPar->GetCell(address));
    //          auto xLo          = pCell->GetX() - 0.5 * pCell->GetSizex();
    //          auto xUp          = pCell->GetX() + 0.5 * pCell->GetSizex();
    //          auto yLo          = pCell->GetY() - 0.5 * pCell->GetSizey();
    //          auto yUp          = pCell->GetY() + 0.5 * pCell->GetSizey();
    //          auto zLo          = pCell->GetZ() - 1.0;
    //          auto zUp          = pCell->GetZ() + 1.0;

    //          auto* canv = MakeQaObject<CbmQaCanvas>(name, "", 1500, 800);
    //          canv->Divide(3, 1);
    //          {
    //            canv->cd(1);
    //            fvph_hit_xy_vs_cell[iSmType][iSm][iRpc][iCh]->DrawCopy("colz", "");
    //            DrawBox(xLo, yLo, xUp, yUp);

    //            canv->cd(2);
    //            fvph_hit_zx_vs_cell[iSmType][iSm][iRpc][iCh]->DrawCopy("colz", "");
    //            DrawBox(zLo, xLo, zUp, xUp);

    //            canv->cd(3);
    //            fvph_hit_zy_vs_cell[iSmType][iSm][iRpc][iCh]->DrawCopy("colz", "");
    //            DrawBox(zLo, yLo, zUp, yUp);
    //          }
    //        }
    //      }
    //    }
    //  }
    //}
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaTof::DeInit()
{
  CbmCaInputQaBase::DeInit();
  fvph_hit_xy_vs_cell.clear();
  fvph_hit_zx_vs_cell.clear();
  fvph_hit_zy_vs_cell.clear();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaTof::DefineParameters()
{
  auto SetRange = [](std::array<double, 2>& range, double min, double max) {
    range[0] = min;
    range[1] = max;
  };
  // Hit errors
  SetRange(fRHitDx, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDy, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDu, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDv, 0.0000, 5.00);  // [cm]
  SetRange(fRHitDt, 0.0000, 0.15);  // [ns]
  // Residuals
  SetRange(fRResX, -2.00, 2.00);
  SetRange(fRResY, -4.00, 4.00);
  SetRange(fRResU, -2.00, 2.00);
  SetRange(fRResV, -4.00, 4.00);
  SetRange(fRResT, -0.50, 0.50);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaTof::ExecQa()
{
  // Base QA execution
  CbmCaInputQaBase::ExecQa();

  // TOF-specific QA execution
  if constexpr (0) {  // DEBUG
    auto PrintPoint = [&](const CbmTofPoint* point, std::stringstream& m) {
      m << "point: trk=" << point->GetTrackID() << ", ";
      auto pointAddress  = point->GetDetectorID();
      int32_t rpcAddress = (pointAddress << 11);  // Address of RPC
      auto iSmType       = CbmTofAddress::GetSmType(pointAddress);
      auto iSm           = CbmTofAddress::GetSmId(pointAddress);
      auto iRpc          = CbmTofAddress::GetRpcId(pointAddress);
      m << "iSt=" << fpDetInterface->GetTrackingStationIndex(pointAddress) << ", ";
      m << "address=" << pointAddress << ", rpcAddress= " << rpcAddress;
      m << ", (" << iSmType << "," << iSm << "," << iRpc << "), ";
      m << "x=" << point->GetX() << ", y=" << point->GetY() << ", z=" << point->GetZ();
    };

    if (IsMCUsed()) {
      // Print hit sample
      LOG(info) << fName << ": ===== Hit Sample";
      for (int iHit = 0; iHit < fpHits->GetEntriesFast(); ++iHit) {
        const auto* pHit = dynamic_cast<const Hit_t*>(fpHits->At(iHit));
        if (!pHit) {
          LOG(error) << fName << "::FillHistogramsPerHit: hit with index " << iHit << " not found";
          continue;
        }

        int address    = pHit->GetAddress();
        int iHitSmType = CbmTofAddress::GetSmType(address);
        int iHitSm     = CbmTofAddress::GetSmId(address);
        int iHitRpc    = CbmTofAddress::GetRpcId(address);

        auto* pHitMatch = dynamic_cast<CbmMatch*>(fpHitMatches->At(iHit));
        std::stringstream msg;
        msg << fName << ": hit: id=" << iHit << ", NofLinks=" << pHitMatch->GetNofLinks() << ", ";
        msg << "iSt=" << fpDetInterface->GetTrackingStationIndex(address) << ", ";
        msg << "RPC=(" << iHitSmType << "," << iHitSm << "," << iHitRpc << ")";
        if (pHitMatch->GetNofLinks() == 0) {
          continue;
        }
        const auto& bestLink = pHitMatch->GetMatchedLink();
        for (int iLink = 0; iLink < pHitMatch->GetNofLinks(); ++iLink) {
          const auto& link = pHitMatch->GetLink(iLink);
          int iPointExt    = link.GetIndex();
          int iEvent       = link.GetEntry();
          int iFile        = link.GetFile();
          msg << "\n\tLink " << iLink << ": " << iPointExt << ", " << iEvent << ", " << iFile << " (best? ";
          msg << (bestLink == link) << ')';
          if (iPointExt < 0) {
            continue;
          }
          msg << ", ";
          auto* pPoint = dynamic_cast<CbmTofPoint*>(fpMCPoints->Get(link));
          PrintPoint(pPoint, msg);
        }
        LOG(info) << msg.str();
      }
      // Print point sample
      LOG(info) << fName << ": ===== Point Sample";
      if (IsMCUsed()) {
        for (int iE = 0; iE < fpMCEventList->GetNofEvents(); ++iE) {
          int iFile   = fpMCEventList->GetFileIdByIndex(iE);
          int iEvent  = fpMCEventList->GetEventIdByIndex(iE);
          int nPoints = fpMCPoints->Size(iFile, iEvent);
          for (int iPoint = 0; iPoint < nPoints; ++iPoint) {
            auto* pPoint = dynamic_cast<CbmTofPoint*>(fpMCPoints->Get(iFile, iEvent, iPoint));
            std::stringstream msg;
            msg << "link: " << iPoint << ", " << iEvent << ", " << iFile << ", ";
            PrintPoint(pPoint, msg);
            LOG(info) << '\t' << msg.str();
          }
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmCaInputQaTof::FillHistogramsPerHit()
{
  auto iHit        = GetHitQaData().GetHitIndex();
  const auto* pHit = dynamic_cast<const Hit_t*>(fpHits->At(iHit));
  if (!pHit) {
    LOG(error) << fName << "::FillHistogramsPerHit: hit with index " << iHit << " not found";
    return;
  }

  int address    = pHit->GetAddress();
  int iHitSmType = CbmTofAddress::GetSmType(address);
  int iHitSm     = CbmTofAddress::GetSmId(address);
  int iHitRpc    = CbmTofAddress::GetRpcId(address);

  {  // Check, if the hit is created on the one of the defined RPCs. If not, save to address into map
    double xHit = pHit->GetX();
    double yHit = pHit->GetY();
    double zHit = pHit->GetZ();
    fvph_hit_xy_vs_cell[iHitSmType][iHitSm][iHitRpc]->Fill(xHit, yHit);
    fvph_hit_zx_vs_cell[iHitSmType][iHitSm][iHitRpc]->Fill(zHit, xHit);
    fvph_hit_zy_vs_cell[iHitSmType][iHitSm][iHitRpc]->Fill(zHit, yHit);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmCaInputQaTof::InitQa()
{
  // TOF tracking detector interface
  fpDetInterface = CbmTofTrackingInterface::Instance();

  auto initStatus = CbmCaInputQaBase::InitQa();
  if (kSUCCESS != initStatus) {
    return initStatus;
  }

  // FIXME: Move to the parameters definition function
  auto* pRuntimeDb = FairRunAna::Instance()->GetRuntimeDb();
  fDigiPar         = dynamic_cast<CbmTofDigiPar*>(pRuntimeDb->getContainer("CbmTofDigiPar"));
  fDigiBdfPar      = dynamic_cast<CbmTofDigiBdfPar*>(pRuntimeDb->getContainer("CbmTofDigiBdfPar"));

  // ----- Histogram initialization
  // Hit occupancy vs. TOF cell
  MakeQaDirectory("occup_cell/");
  int nSmTypes = fDigiBdfPar->GetNbSmTypes();
  fvph_hit_xy_vs_cell.resize(nSmTypes);
  fvph_hit_zx_vs_cell.resize(nSmTypes);
  fvph_hit_zy_vs_cell.resize(nSmTypes);
  for (int iSmType = 0; iSmType < nSmTypes; ++iSmType) {
    if (iSmType == 5) {
      continue;
    }  // skip Bmon
    MakeQaDirectory(Form("occup_cell/sm_type_%d", iSmType));
    int nSm  = fDigiBdfPar->GetNbSm(iSmType);
    int nRpc = fDigiBdfPar->GetNbRpc(iSmType);
    fvph_hit_xy_vs_cell[iSmType].resize(nSm);
    fvph_hit_zx_vs_cell[iSmType].resize(nSm);
    fvph_hit_zy_vs_cell[iSmType].resize(nSm);
    for (int iSm = 0; iSm < fDigiBdfPar->GetNbSm(iSmType); ++iSm) {
      MakeQaDirectory(Form("occup_cell/sm_type_%d/sm_%d/", iSmType, iSm));
      fvph_hit_xy_vs_cell[iSmType][iSm].resize(nRpc, nullptr);
      fvph_hit_zx_vs_cell[iSmType][iSm].resize(nRpc, nullptr);
      fvph_hit_zy_vs_cell[iSmType][iSm].resize(nRpc, nullptr);
      for (int iRpc = 0; iRpc < nRpc; ++iRpc) {
        const char* dir = Form("occup_cell/sm_type_%d/sm_%d/rpc%d", iSmType, iSm, iRpc);
        TString name    = Form("%s/occup_xy_smt%d_sm%d_rpc%d", dir, iSmType, iSm, iRpc);
        TString title   = Form("Hit Occupancy in xy-Plane for iSmType = %d, iSm = %d, iRpc = %d", iSmType, iSm, iRpc);
        title += ";x_{hit} [cm];y_{hit} [cm]";
        fvph_hit_xy_vs_cell[iSmType][iSm][iRpc] =
          MakeQaObject<TH2F>(name, title, fNbXo, fLoXo, fUpXo, fNbYo, fLoYo, fUpYo);
        name  = Form("%s/occup_zx_smt%d_sm%d_rpc%d", dir, iSmType, iSm, iRpc);
        title = Form("Hit Occupancy in zx-Plane for iSmType = %d, iSm = %d, iRpc = %d", iSmType, iSm, iRpc);
        title += ";z_{hit} [cm];x_{hit} [cm]";
        fvph_hit_zx_vs_cell[iSmType][iSm][iRpc] =
          MakeQaObject<TH2F>(name, title, fNbinsZ, frZmin.back(), frZmax.back(), fNbXo, fLoXo, fUpXo);
        name  = Form("%s/occup_zy_smt%d_sm%d_rpc%d", dir, iSmType, iSm, iRpc);
        title = Form("Hit Occupancy in zy-Plane for iSmType = %d, iSm = %d, iRpc = %d", iSmType, iSm, iRpc);
        title += ";z_{hit} [cm];y_{hit} [cm]";
        fvph_hit_zy_vs_cell[iSmType][iSm][iRpc] =
          MakeQaObject<TH2F>(name, title, fNbinsZ, frZmin.back(), frZmax.back(), fNbYo, fLoYo, fUpYo);
      }
    }
  }

  return kSUCCESS;
}
