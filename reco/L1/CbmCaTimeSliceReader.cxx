/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaTimeSliceReader.cxx
/// @brief  Time-slice/event reader for CA tracker in CBM (implementation)
/// @since  24.02.2023
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "CbmCaTimeSliceReader.h"

#include "CaDefs.h"
#include "CaInputData.h"
#include "CaParameters.h"
#include "CbmGlobalTrack.h"
#include "CbmKfUtil.h"  // for CopyTrackParam2TC
#include "CbmL1.h"
#include "CbmMuchTrack.h"
#include "CbmStsTrack.h"
#include "CbmTofTrack.h"
#include "CbmTrdTrack.h"
#include "FairRootManager.h"
#include "Logger.h"

#include <algorithm>
#include <numeric>

using cbm::ca::tools::HitRecord;

using namespace cbm::algo::ca::constants;
using namespace cbm::algo;

using cbm::ca::TimeSliceReader;

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::Clear()
{
  // Detector used
  fvbUseDet.fill(false);

  // Input branches
  // fpBrTimeSlice = nullptr;
  fpParameters = nullptr;

  fvpBrHits.fill(nullptr);

  fpBrRecoTracks = nullptr;
  fpBrStsTracks  = nullptr;
  fpBrMuchTracks = nullptr;
  fpBrTrdTracks  = nullptr;
  fpBrTofTracks  = nullptr;

  // Pointers to output containers
  fpvHitIds       = nullptr;
  fpvQaHits       = nullptr;
  fpIODataManager = nullptr;
  fpvTracks       = nullptr;

  fNofHits     = 0;
  fNofHitKeys  = 0;
  fFirstHitKey = 0;

  std::fill(fvHitFirstIndexDet.begin(), fvHitFirstIndexDet.end(), 0);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::CheckInit() const
{
  // Check parameters
  if (!fpParameters.get()) {
    throw std::logic_error("Tracking parameters object was not defined");
  }
  if (!fpvHitIds) {
    throw std::logic_error("Hit index container was not defined");
  }

  //if (!fpBrTimeSlice) {
  //throw std::logic_error("Time slice branch is unavailable");
  //}

  for (int iDet = 0; iDet < static_cast<int>(ca::EDetectorID::END); ++iDet) {
    if (fvbUseDet[iDet] && !fvpBrHits[iDet]) {
      throw std::logic_error(std::string(kDetName[iDet]) + " hits branch is not found");
    }
  }

  if (fpvTracks) {
    if (ECbmCaTrackingMode::kSTS == fTrackingMode) {
      if (!fpBrRecoTracks) {
        throw std::logic_error("StsTrack branch is unavailable");
      }
    }
    else if (ECbmCaTrackingMode::kMCBM == fTrackingMode) {
      if (!fpBrRecoTracks) {
        throw std::logic_error("GlobalTrack branch is unavailable");
      }
      if (fvbUseDet[ca::EDetectorID::kSts] && !fpBrStsTracks) {
        throw std::logic_error("StsTrack branch is not found");
      }
      if (fvbUseDet[ca::EDetectorID::kMuch] && !fpBrMuchTracks) {
        throw std::logic_error("MuchTrack branch is not found");
      }
      if (fvbUseDet[ca::EDetectorID::kTrd] && !fpBrTrdTracks) {
        throw std::logic_error("TrdTrack branch is not found");
      }
      if (fvbUseDet[ca::EDetectorID::kTof] && !fpBrTofTracks) {
        throw std::logic_error("TofTrack branch is not found");
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool TimeSliceReader::InitRun()
try {
  LOG(info) << "TimeSliceReader: initializing run ... ";

  // Init tracking detector interfaces
  fvpDetInterface[ca::EDetectorID::kMvd]  = CbmMvdTrackingInterface::Instance();
  fvpDetInterface[ca::EDetectorID::kSts]  = CbmStsTrackingInterface::Instance();
  fvpDetInterface[ca::EDetectorID::kMuch] = CbmMuchTrackingInterface::Instance();
  fvpDetInterface[ca::EDetectorID::kTrd]  = CbmTrdTrackingInterface::Instance();
  fvpDetInterface[ca::EDetectorID::kTof]  = CbmTofTrackingInterface::Instance();

  // ** Init data branches **

  auto* pFairManager = FairRootManager::Instance();
  LOG_IF(fatal, !pFairManager) << "TimeSliceReader: FairRootManager was not defined";

  // fpBrTimeSlice = dynamic_cast<CbmTimeSlice*>(pFairManager->GetObject("TimeSlice."));

  // Init branches
  auto InitHitBranch = [&](ca::EDetectorID detID, const char* branchName) -> bool {
    if (fvbUseDet[detID]) {
      fvpBrHits[detID] = dynamic_cast<TClonesArray*>(pFairManager->GetObject(branchName));
    }
    return static_cast<bool>(fvpBrHits[detID]);
  };

  InitHitBranch(ca::EDetectorID::kMvd, "MvdHit");
  InitHitBranch(ca::EDetectorID::kSts, "StsHit");
  InitHitBranch(ca::EDetectorID::kMuch, "MuchPixelHit");
  InitHitBranch(ca::EDetectorID::kTrd, "TrdHit");
  if (!InitHitBranch(ca::EDetectorID::kTof, "TofCalHit")) {
    LOG(warn) << "TimeSliceReader: TofCalHit branch not found, trying to get TofHit branch of uncalibrated hits";
    InitHitBranch(ca::EDetectorID::kTof, "TofHit");
  }

  // Init track branches

  if (fpvTracks) {
    switch (fTrackingMode) {
      case ECbmCaTrackingMode::kSTS:
        fpBrRecoTracks = dynamic_cast<TClonesArray*>(pFairManager->GetObject("StsTrack"));
        break;
      case ECbmCaTrackingMode::kMCBM:
        fpBrRecoTracks = dynamic_cast<TClonesArray*>(pFairManager->GetObject("GlobalTrack"));
        if (fvbUseDet[ca::EDetectorID::kSts]) {
          fpBrStsTracks = dynamic_cast<TClonesArray*>(pFairManager->GetObject("StsTrack"));
        }
        if (fvbUseDet[ca::EDetectorID::kMuch]) {
          fpBrMuchTracks = dynamic_cast<TClonesArray*>(pFairManager->GetObject("MuchTrack"));
        }
        if (fvbUseDet[ca::EDetectorID::kTrd]) {
          fpBrTrdTracks = dynamic_cast<TClonesArray*>(pFairManager->GetObject("TrdTrack"));
        }
        if (fvbUseDet[ca::EDetectorID::kTof]) {
          fpBrTofTracks = dynamic_cast<TClonesArray*>(pFairManager->GetObject("TofTrack"));
        }
        break;
    }
  }

  // Check initialization
  this->CheckInit();

  //clrs::CL - clear log
  //clrs::GNb - green bold log
  //clrs::RDb - red bold log

  LOG(info) << "TimeSliceReader: initializing run ... " << clrs::GNb << "done" << clrs::CL;
  return true;
}
catch (const std::logic_error& error) {
  LOG(info) << "TimeSliceReader: initializing run ... " << clrs::RDb << "failed" << clrs::CL
            << "\nReason: " << error.what();
  return false;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::ReadEvent(CbmEvent* pEvent)
{
  fpEvent = pEvent;
  this->ReadHits();
  if (fpvTracks) {
    this->ReadRecoTracks();
  }
}


// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::ReadRecoTracks()
{
  assert(fpBrRecoTracks);
  int nTracks = 0;
  switch (fTrackingMode) {
    case ECbmCaTrackingMode::kSTS:
      nTracks = fpEvent ? fpEvent->GetNofData(ECbmDataType::kStsTrack) : fpBrRecoTracks->GetEntriesFast();
      fpvTracks->reset(nTracks);
      // Fill tracks from StsTrack branch
      for (int iT = 0; iT < nTracks; ++iT) {
        int iText         = fpEvent ? fpEvent->GetIndex(ECbmDataType::kStsTrack, iT) : iT;
        auto* pInputTrack = static_cast<CbmStsTrack*>(fpBrRecoTracks->At(iText));
        auto& track       = (*fpvTracks)[iT];

        track.Set(cbm::kf::ConvertTrackParam(*pInputTrack->GetParamFirst()));
        track.TLast        = cbm::kf::ConvertTrackParam(*pInputTrack->GetParamLast());
        track.ChiSq()      = pInputTrack->GetChiSq();
        track.Ndf()        = pInputTrack->GetNDF();
        track.Tpv.Time()   = pInputTrack->GetStartTime();
        track.Tpv.C55()    = pInputTrack->GetStartTimeError();
        track.Time()       = pInputTrack->GetFirstHitTime();
        track.C55()        = pInputTrack->GetFirstHitTimeError();
        track.TLast.Time() = pInputTrack->GetLastHitTime();
        track.TLast.C55()  = pInputTrack->GetLastHitTimeError();
        track.Hits.clear();
        track.Hits.reserve(pInputTrack->GetTotalNofHits());
        for (int iH = 0; iH < pInputTrack->GetNofMvdHits(); ++iH) {
          int iHext = pInputTrack->GetMvdHitIndex(iH);
          int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kMvd][iHext];
          track.Hits.push_back(iHint);
        }  // iH
        for (int iH = 0; iH < pInputTrack->GetNofStsHits(); ++iH) {
          int iHext = pInputTrack->GetStsHitIndex(iH);
          int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kSts][iHext];
          track.Hits.push_back(iHint);
        }  // iH
      }    // iT
      break;

    case ECbmCaTrackingMode::kMCBM:
      // Fill tracks from GlobalTrack branch
      nTracks = fpEvent ? fpEvent->GetNofData(ECbmDataType::kGlobalTrack) : fpBrRecoTracks->GetEntriesFast();
      fpvTracks->reset(nTracks);
      for (int iT = 0; iT < nTracks; ++iT) {
        int iText         = fpEvent ? fpEvent->GetIndex(ECbmDataType::kGlobalTrack, iT) : iT;
        auto* pInputTrack = static_cast<CbmGlobalTrack*>(fpBrRecoTracks->At(iText));
        auto& track       = (*fpvTracks)[iT];
        track.Set(cbm::kf::ConvertTrackParam(*pInputTrack->GetParamFirst()));
        track.TLast   = cbm::kf::ConvertTrackParam(*pInputTrack->GetParamLast());
        track.ChiSq() = pInputTrack->GetChi2();
        track.Ndf()   = pInputTrack->GetNDF();

        // ** Fill information from local tracks **
        // STS tracks (+ MVD)
        if (fvbUseDet[ca::EDetectorID::kSts]) {
          int iStsTrkId = pInputTrack->GetStsTrackIndex();
          if (iStsTrkId > -1) {
            auto* pStsTrack = static_cast<CbmStsTrack*>(fpBrStsTracks->At(iStsTrkId));
            if (fvbUseDet[ca::EDetectorID::kMvd]) {
              for (int iH = 0; iH < pStsTrack->GetNofMvdHits(); ++iH) {
                int iHext = pStsTrack->GetMvdHitIndex(iH);
                int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kMvd][iHext];
                track.Hits.push_back(iHint);
              }
            }
            for (int iH = 0; iH < pStsTrack->GetNofStsHits(); ++iH) {
              int iHext = pStsTrack->GetStsHitIndex(iH);
              int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kSts][iHext];
              track.Hits.push_back(iHint);
            }
          }
        }

        // MUCH tracks
        if (fvbUseDet[ca::EDetectorID::kMuch]) {
          int iMuchTrkId = pInputTrack->GetMuchTrackIndex();
          if (iMuchTrkId > -1) {
            auto* pMuchTrack = static_cast<CbmMuchTrack*>(fpBrMuchTracks->At(iMuchTrkId));
            for (int iH = 0; iH < pMuchTrack->GetNofHits(); ++iH) {
              int iHext = pMuchTrack->GetHitIndex(iH);
              int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kMuch][iHext];
              track.Hits.push_back(iHint);
            }
          }
        }

        // TRD tracks
        if (fvbUseDet[ca::EDetectorID::kTrd]) {
          int iTrdTrkId = pInputTrack->GetTrdTrackIndex();
          if (iTrdTrkId > -1) {
            const auto* pTrdTrack = static_cast<const CbmTrdTrack*>(fpBrTrdTracks->At(iTrdTrkId));
            for (int iH = 0; iH < pTrdTrack->GetNofHits(); ++iH) {
              int iHext = pTrdTrack->GetHitIndex(iH);
              int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kTrd][iHext];
              track.Hits.push_back(iHint);
            }  // iH
          }
        }

        // TOF tracks
        if (fvbUseDet[ca::EDetectorID::kTof]) {
          int iTofTrkId = pInputTrack->GetTofTrackIndex();
          if (iTofTrkId > -1) {
            const auto* pTofTrack = static_cast<const CbmTofTrack*>(fpBrTofTracks->At(iTofTrkId));
            for (int iH = 0; iH < pTofTrack->GetNofHits(); ++iH) {
              int iHext = pTofTrack->GetHitIndex(iH);
              int iHint = fvmHitExtToIntIndexMap[ca::EDetectorID::kTof][iHext];
              track.Hits.push_back(iHint);
            }  // iH
          }    // if iTofTrkId > -1
        }      // if fvbUseDet[ca::EDetectorID::kTof]
      }        // iT
      break;
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::RegisterIODataManager(std::shared_ptr<ca::DataManager>& pIODataManager)
{
  LOG_IF(fatal, !pIODataManager.get()) << "TimeSliceReader: passed null pointer as a ca::DataManager instance";
  fpIODataManager = pIODataManager;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::SortQaHits()
{
  int nStationsActive = fpParameters->GetNstationsActive();
  ca::Vector<CbmL1HitDebugInfo> vNewHits{"TimeSliceReader::SortQaHits(): vNewHits", fpvQaHits->size()};
  std::vector<int> vHitFstIndexes(nStationsActive + 1, 0);
  std::vector<int> vNofHitsStored(nStationsActive, 0);

  // Count number of hits in each station (NOTE: we could use here boarders from the IO data manager, but we would keep
  // these two procedures independent)
  std::for_each(fpvQaHits->begin(), fpvQaHits->end(), [&](const auto& h) { ++vHitFstIndexes[h.GetStationId() + 1]; });
  for (int iSt = 0; iSt < nStationsActive; ++iSt) {
    vHitFstIndexes[iSt + 1] += vHitFstIndexes[iSt];
  }
  for (const auto& hit : (*fpvQaHits)) {
    int iSt                                                 = hit.GetStationId();
    vNewHits[vHitFstIndexes[iSt] + (vNofHitsStored[iSt]++)] = hit;
  }

  auto name = fpvQaHits->GetName();
  std::swap(vNewHits, (*fpvQaHits));
  fpvQaHits->SetName(name);
}


// ---------------------------------------------------------------------------------------------------------------------
//
template<ca::EDetectorID DetID>
int cbm::ca::TimeSliceReader::ReadHitsForDetector()
{
  using Hit_t = HitTypes_t::at<DetID>;

  if (!fvbUseDet[DetID]) {
    return 0;
  }  // Detector is entirelly not used

  const auto* pDetInterface = fvpDetInterface[DetID];
  int nHitsTot              = fpEvent ? fpEvent->GetNofData(kCbmHitType[DetID]) : fvpBrHits[DetID]->GetEntriesFast();
  int nHitsStored           = 0;  // number of hits used in tracking

  fFirstHitKey = fNofHitKeys;

  for (int iH = 0; iH < nHitsTot; ++iH) {
    int iHext = fpEvent ? fpEvent->GetIndex(kCbmHitType[DetID], iH) : iH;
    if (iHext < 0) {
      LOG(warn) << "TimeSliceReader: hit index stored in the event is negative: " << iHext;
      continue;
    }
    tools::HitRecord hitRecord;

    auto* pHit  = static_cast<Hit_t*>(fvpBrHits[DetID]->At(iHext));
    int iStGeom = pDetInterface->GetTrackingStationIndex(pHit);
    if (iStGeom < 0) {
      continue;  // NOTE: sensors with iStGeom = -1 are ignored in tracking
    }

    // Fill out detector specific data
    if constexpr (ca::EDetectorID::kSts == DetID) {
      hitRecord.fStripF = fFirstHitKey + pHit->GetFrontClusterId();
      hitRecord.fStripB = fFirstHitKey + pHit->GetBackClusterId();
    }
    else if constexpr (ca::EDetectorID::kTof == DetID) {
      // *** Additional cuts for TOF ***
      // Skip Bmon hits
      if (5 == CbmTofAddress::GetSmType(pHit->GetAddress())) {
        continue;
      }
    }

    int iStActive = fpParameters->GetStationIndexActive(iStGeom, DetID);
    if (iStActive < 0) {
      continue;
    }  // Cut off inactive stations

    // Fill out data common for all the detectors
    hitRecord.fStaId = iStActive;
    hitRecord.fX     = pHit->GetX();
    hitRecord.fY     = pHit->GetY();
    hitRecord.fZ     = pHit->GetZ();
    hitRecord.fDx2   = pHit->GetDx() * pHit->GetDx();
    hitRecord.fDy2   = pHit->GetDy() * pHit->GetDy();
    hitRecord.fDxy   = pHit->GetDxy();
    hitRecord.fT     = pHit->GetTime();
    hitRecord.fDt2   = pHit->GetTimeError() * pHit->GetTimeError();

    // Apply hit error smearing according to misalignment
    hitRecord.fDx2 += fpParameters->GetMisalignmentXsq(DetID);
    hitRecord.fDy2 += fpParameters->GetMisalignmentYsq(DetID);
    hitRecord.fDt2 += fpParameters->GetMisalignmentTsq(DetID);

    std::tie(hitRecord.fRangeX, hitRecord.fRangeY, hitRecord.fRangeT) = pDetInterface->GetHitRanges(*pHit);

    hitRecord.fDet        = static_cast<int>(DetID);
    hitRecord.fDataStream = (static_cast<int64_t>(hitRecord.fDet) << 60) | pHit->GetAddress();
    hitRecord.fExtId      = iHext;

    // Check if hit values are reasonable
    {
      const ca::Station<fvec>& station = fpParameters->GetStation(iStActive);
      if (pHit->GetX() < station.GetXmin<fscal>() * 1.2 || pHit->GetX() > station.GetXmax<fscal>() * 1.2
          || pHit->GetY() < station.GetYmin<fscal>() * 1.2 || pHit->GetY() > station.GetYmax<fscal>() * 1.2
          || fabs(pHit->GetZ() - station.GetZ<fscal>()) > 150) {
        LOG(error) << "Ca:TimeSliceReader: " << CbmL1::GetDetectorName(DetID)
                   << " hit is outside of the station boundaries: " << hitRecord.ToString() << ";  station X "
                   << station.GetXmin<fscal>() << ".." << station.GetXmax<fscal>() << " Y " << station.GetYmin<fscal>()
                   << ".." << station.GetYmax<fscal>() << " Z " << station.GetZ<fscal>();
        continue;
      }
    }

    // Update number of hit keys
    if constexpr (ca::EDetectorID::kSts == DetID) {
      if (fNofHitKeys <= hitRecord.fStripF) {
        fNofHitKeys = hitRecord.fStripF + 1;
      }
      if (fNofHitKeys <= hitRecord.fStripB) {
        fNofHitKeys = hitRecord.fStripB + 1;
      }
    }
    else {
      hitRecord.fStripF = fFirstHitKey + iHext;
      hitRecord.fStripB = hitRecord.fStripF;
      if (fNofHitKeys <= hitRecord.fStripF) {
        fNofHitKeys = hitRecord.fStripF + 1;
      }
    }

    // Save hit to data structures
    if (hitRecord.Accept()) {
      this->StoreHitRecord(hitRecord);
      ++nHitsStored;
    }
  }  // iH

  return nHitsStored;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::ReadHits()
{
  fNofHits     = 0;
  fNofHitKeys  = 0;
  fFirstHitKey = 0;

  // TODO: Address case with CbmEvent != nullptr
  for (int iDet = 0; iDet < static_cast<int>(ca::EDetectorID::END); ++iDet) {
    if (fvbUseDet[iDet]) {
      fvNofHitsTotal[iDet] = fpEvent ? fpEvent->GetNofData(kCbmHitType[iDet]) : fvpBrHits[iDet]->GetEntriesFast();
    }
  }

  int nHitsTot = std::accumulate(fvNofHitsTotal.begin(), fvNofHitsTotal.end(), 0);

  // Resize the containers
  if (fpvHitIds) {
    fpvHitIds->clear();
    fpvHitIds->reserve(nHitsTot);
  }
  if (fpvQaHits) {
    fpvQaHits->clear();
    fpvQaHits->reserve(nHitsTot);
  }
  if (fpIODataManager) {
    fpIODataManager->ResetInputData(nHitsTot);
  }

  std::fill(fvHitFirstIndexDet.begin(), fvHitFirstIndexDet.end(), 0);

  // Read hits for different detectors
  fvNofHitsUsed[ca::EDetectorID::kMvd]  = ReadHitsForDetector<ca::EDetectorID::kMvd>();
  fvNofHitsUsed[ca::EDetectorID::kSts]  = ReadHitsForDetector<ca::EDetectorID::kSts>();
  fvNofHitsUsed[ca::EDetectorID::kMuch] = ReadHitsForDetector<ca::EDetectorID::kMuch>();
  fvNofHitsUsed[ca::EDetectorID::kTrd]  = ReadHitsForDetector<ca::EDetectorID::kTrd>();
  fvNofHitsUsed[ca::EDetectorID::kTof]  = ReadHitsForDetector<ca::EDetectorID::kTof>();

  // Save first hit index for different detector subsystems
  for (uint32_t iDet = 0; iDet < fvNofHitsUsed.size(); ++iDet) {
    fvHitFirstIndexDet[iDet + 1] = fvHitFirstIndexDet[iDet] + fvNofHitsUsed[iDet];
  }

  fNofHits = std::accumulate(fvNofHitsUsed.cbegin(), fvNofHitsUsed.cend(), 0);

  LOG(debug) << "CA: N hits used/tot = " << fNofHits << "/" << nHitsTot;

  // Update number of hit keys in input data object
  if (fpIODataManager) {
    fpIODataManager->SetNhitKeys(fNofHitKeys);
  }

  // Sort debug hits
  if (fpvQaHits && fbSortQaHits) {
    this->SortQaHits();
  }

  // Update maps of ext->int hit indexes
  // NOTE: fvpHitIds must be initialized, if we want to read tracks from the file
  if (fpvHitIds) {
    auto ResetIndexMap = [&](auto& m) mutable { m.clear(); };
    std::for_each(fvmHitExtToIntIndexMap.begin(), fvmHitExtToIntIndexMap.end(), ResetIndexMap);
    for (int iH = 0; iH < fNofHits; ++iH) {
      const auto& hit                               = (*fpvQaHits)[iH];
      fvmHitExtToIntIndexMap[hit.Det][hit.ExtIndex] = iH;
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TimeSliceReader::StoreHitRecord(const HitRecord& hitRecord)
{
  // Save the algo hit
  if (fpIODataManager.get()) {
    ca::Hit aHit;
    aHit.SetFrontKey(hitRecord.fStripF);
    aHit.SetBackKey(hitRecord.fStripB);
    aHit.SetX(hitRecord.fX);
    aHit.SetY(hitRecord.fY);
    aHit.SetZ(hitRecord.fZ);
    aHit.SetT(hitRecord.fT);
    aHit.SetDx2(hitRecord.fDx2);
    aHit.SetDy2(hitRecord.fDy2);
    aHit.SetDxy(hitRecord.fDxy);
    aHit.SetDt2(hitRecord.fDt2);
    aHit.SetRangeX(hitRecord.fRangeX);
    aHit.SetRangeY(hitRecord.fRangeY);
    aHit.SetRangeT(hitRecord.fRangeT);
    aHit.SetId(static_cast<int>(fpIODataManager->GetNofHits()));
    aHit.SetStation(hitRecord.fStaId);

    fpIODataManager->PushBackHit(aHit, hitRecord.fDataStream);
  }

  // Save hit ID information
  if (fpvHitIds) {
    fpvHitIds->emplace_back(hitRecord.fDet, hitRecord.fExtId);
  }

  // Save debug information
  if (fpvQaHits) {
    CbmL1HitDebugInfo aHitQa;
    aHitQa.Det      = hitRecord.fDet;
    aHitQa.ExtIndex = hitRecord.fExtId;
    aHitQa.IntIndex = static_cast<int>(fpvQaHits->size());
    aHitQa.iStation = hitRecord.fStaId;
    aHitQa.x        = hitRecord.fX;
    aHitQa.y        = hitRecord.fY;
    aHitQa.z        = hitRecord.fZ;
    aHitQa.dx       = sqrt(hitRecord.fDx2);
    aHitQa.dy       = sqrt(hitRecord.fDy2);
    aHitQa.dxy      = hitRecord.fDxy;
    aHitQa.time     = hitRecord.fT;
    aHitQa.dt       = sqrt(hitRecord.fDt2);
    fpvQaHits->push_back(aHitQa);
  }
}
