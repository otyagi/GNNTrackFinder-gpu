/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaMCModule.cxx
/// \brief  CA Tracking performance interface for CBM (implementation)
/// \since  23.09.2022
/// \author S.Zharko <s.zharko@gsi.de>

#include "CbmCaMCModule.h"

#include "CbmEvent.h"
#include "CbmL1Constants.h"
#include "CbmL1DetectorID.h"
#include "CbmL1Hit.h"
#include "CbmMCDataManager.h"
#include "CbmMCDataObject.h"
#include "CbmMCEventList.h"
#include "CbmMCTrack.h"
#include "CbmStsHit.h"
#include "CbmTimeSlice.h"
#include "CbmTofPoint.h"
#include "FairEventHeader.h"
#include "FairMCEventHeader.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "Logger.h"
#include "TDatabasePDG.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <cassert>
#include <fstream>  // TODO: SZh 07.12.2022: For debug, should be removed!
#include <limits>
#include <map>
#include <stdexcept>  // for std::logic_error
#include <utility>

// *********************************
// ** Action definition functions **
// *********************************

using cbm::algo::ca::constants::clrs::CL;   // clear log
using cbm::algo::ca::constants::clrs::RDb;  // red bold log
using cbm::ca::MCModule;
using cbm::ca::tools::MCTrack;

// ---------------------------------------------------------------------------------------------------------------------
//
bool MCModule::InitRun()
try {

  if (fVerbose > 0) {
    LOG(info) << "CA MC Module: initializing CA tracking Monte-Carlo module... ";
  }

  // Detector interfaces
  if (fvbUseDet[ca::EDetectorID::kMvd]) {
    fvpDetInterface[ca::EDetectorID::kMvd] = CbmMvdTrackingInterface::Instance();
  }
  if (fvbUseDet[ca::EDetectorID::kSts]) {
    fvpDetInterface[ca::EDetectorID::kSts] = CbmStsTrackingInterface::Instance();
  }
  if (fvbUseDet[ca::EDetectorID::kMuch]) {
    fvpDetInterface[ca::EDetectorID::kMuch] = CbmMuchTrackingInterface::Instance();
  }
  if (fvbUseDet[ca::EDetectorID::kTrd]) {
    fvpDetInterface[ca::EDetectorID::kTrd] = CbmTrdTrackingInterface::Instance();
  }
  if (fvbUseDet[ca::EDetectorID::kTof]) {
    fvpDetInterface[ca::EDetectorID::kTof] = CbmTofTrackingInterface::Instance();
  }

  auto fairManager = FairRootManager::Instance();
  assert(fairManager);

  auto mcManager = dynamic_cast<CbmMCDataManager*>(fairManager->GetObject("MCDataManager"));
  assert(mcManager);

  fpTimeSlice     = dynamic_cast<CbmTimeSlice*>(fairManager->GetObject("TimeSlice."));
  fpMCEventList   = dynamic_cast<CbmMCEventList*>(fairManager->GetObject("MCEventList."));
  fpMCEventHeader = mcManager->GetObject("MCEventHeader.");
  fpMCTracks      = mcManager->InitBranch("MCTrack");

  fvpBrPoints.fill(nullptr);
  fvpBrHitMatches.fill(nullptr);

  fFileEventIDs.clear();

  auto InitPointBranch = [&](const char* brName, ca::EDetectorID detID) {
    if (fvbUseDet[detID]) {
      fvpBrPoints[detID] = mcManager->InitBranch(brName);
    }
  };

  auto InitMatchesBranch = [&](const char* brName, ca::EDetectorID detID) {
    if (fvbUseDet[detID]) {
      fvpBrHitMatches[detID] = dynamic_cast<TClonesArray*>(fairManager->GetObject(brName));
    }
  };

  InitPointBranch("MvdPoint", ca::EDetectorID::kMvd);
  InitPointBranch("StsPoint", ca::EDetectorID::kSts);
  InitPointBranch("MuchPoint", ca::EDetectorID::kMuch);
  InitPointBranch("TrdPoint", ca::EDetectorID::kTrd);
  InitPointBranch("TofPoint", ca::EDetectorID::kTof);

  InitMatchesBranch("MvdHitMatch", ca::EDetectorID::kMvd);
  InitMatchesBranch("StsHitMatch", ca::EDetectorID::kSts);
  InitMatchesBranch("MuchPixelHitMatch", ca::EDetectorID::kMuch);
  InitMatchesBranch("TrdHitMatch", ca::EDetectorID::kTrd);
  InitMatchesBranch("TofHitMatch", ca::EDetectorID::kTof);


  // Check initialization
  this->CheckInit();

  // Init monitor
  fMonitor.SetCounterName(EMonitorKey::kMcTrack, "N MC tracks");
  fMonitor.SetCounterName(EMonitorKey::kMcTrackReconstructable, "N MC tracks rec-able");
  fMonitor.SetCounterName(EMonitorKey::kMcPoint, "N MC points");
  fMonitor.SetCounterName(EMonitorKey::kRecoNevents, "N reco events");
  fMonitor.SetCounterName(EMonitorKey::kMissedMatchesMvd, "N missed MVD matches");
  fMonitor.SetCounterName(EMonitorKey::kMissedMatchesSts, "N missed STS matches");
  fMonitor.SetCounterName(EMonitorKey::kMissedMatchesMuch, "N missed MuCh matches");
  fMonitor.SetCounterName(EMonitorKey::kMissedMatchesTrd, "N missed TRD matches");
  fMonitor.SetCounterName(EMonitorKey::kMissedMatchesTof, "N missed TOF matches");
  fMonitor.SetRatioKeys({EMonitorKey::kRecoNevents});


  if (fVerbose > 0) {
    LOG(info) << "CA MC Module: initializing CA tracking Monte-Carlo module... \033[1;32mDone!\033[0m";
  }
  return true;
}
catch (const std::logic_error& error) {
  LOG(error) << "CA MC Module: initializing CA tracking Monte-Carlo module... \033[1;31mFailed\033[0m\nReason: "
             << error.what();
  return false;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::InitEvent(CbmEvent* pEvent)
{
  // Fill a set of file and event indexes
  fFileEventIDs.clear();
  fBestMcFile  = -1;
  fBestMcEvent = -1;
  if (pEvent) {
    CbmMatch* pEvtMatch = pEvent->GetMatch();
    assert(pEvtMatch);
    int nLinks = pEvtMatch->GetNofLinks();
    LOG(info) << "DEBUG: nof linked mc events " << nLinks << " of total " << fpMCEventList->GetNofEvents();
    for (int iLink = 0; iLink < nLinks; ++iLink) {
      const auto& link = pEvtMatch->GetLink(iLink);
      fFileEventIDs.emplace(link.GetFile(), link.GetEntry());
    }
    if (nLinks > 1) {
      const auto& bestLink = pEvtMatch->GetMatchedLink();
      fBestMcFile          = bestLink.GetFile();
      fBestMcEvent         = bestLink.GetEntry();
    }
  }
  else {
    int nEvents = fpMCEventList->GetNofEvents();
    for (int iE = 0; iE < nEvents; ++iE) {
      int iFile  = fpMCEventList->GetFileIdByIndex(iE);
      int iEvent = fpMCEventList->GetEventIdByIndex(iE);
      fFileEventIDs.emplace(iFile, iEvent);
    }
  }

  // Read data
  fpMCData->Clear();
  this->ReadMCTracks();
  this->ReadMCPoints();
  fMonitor.IncrementCounter(EMonitorKey::kMcTrack, fpMCData->GetNofPoints());
  fMonitor.IncrementCounter(EMonitorKey::kMcPoint, fpMCData->GetNofTracks());

  // Prepare tracks: set point indexes and redefine indexes from external to internal containers
  for (auto& aTrk : fpMCData->GetTrackContainer()) {
    aTrk.SortPointIndexes(
      [&](const int& iPl, const int& iPr) { return fpMCData->GetPoint(iPl).GetZ() < fpMCData->GetPoint(iPr).GetZ(); });
  }

  fMonitor.IncrementCounter(EMonitorKey::kRecoNevents);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::ProcessEvent(CbmEvent*) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::InitTrackInfo()
{
  // FIXME: Reconstructable criteria initialization should be verified!
  // ----- Initialize stations arrangement and hit indexes
  fpMCData->InitTrackInfo(*fpvQaHits);

  // ----- Define reconstructable and additional flags
  for (auto& aTrk : fpMCData->GetTrackContainer()) {
    bool isRec = true;  // the track can be reconstructed

    // Cut on momentum
    isRec &= aTrk.GetP() > CbmL1Constants::MinRecoMom;

    // Cut on max number of points on station
    isRec &= aTrk.GetMaxNofPointsOnStation() <= 3;

    // Suppress MC tracks from complementary MC events
    if (fBestMcFile >= 0 && (aTrk.GetFileId() != fBestMcFile || aTrk.GetEventId() != fBestMcEvent)) {
      isRec = false;
    }

    bool isAdd = isRec;  // is track additional

    // Cut on number of stations
    switch (fPerformanceMode) {
      case 1: isRec &= aTrk.GetNofConsStationsWithHit() >= CbmL1Constants::MinNStations; break;
      case 2: isRec &= aTrk.GetTotNofStationsWithHit() >= CbmL1Constants::MinNStations; break;
      case 3: isRec &= aTrk.GetNofConsStationsWithPoint() >= CbmL1Constants::MinNStations; break;
      default: LOG(fatal) << "CA MC Module: invalid performance mode " << fPerformanceMode;
    }

    if (aTrk.GetNofPoints() > 0) {
      isAdd &= aTrk.GetNofConsStationsWithHit() == aTrk.GetTotNofStationsWithHit();
      isAdd &= aTrk.GetNofConsStationsWithPoint() == aTrk.GetTotNofStationsWithHit();
      isAdd &= aTrk.GetTotNofStationsWithHit() == aTrk.GetTotNofStationsWithPoint();
      isAdd &= aTrk.GetNofConsStationsWithHit() >= 3;
      isAdd &= fpMCData->GetPoint(aTrk.GetPointIndexes()[0]).GetStationId() == 0;
      isAdd &= !isRec;
    }
    else {
      isAdd = false;
    }
    aTrk.SetFlagReconstructable(isRec);
    aTrk.SetFlagAdditional(isAdd);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::Finish() { LOG(info) << '\n' << fMonitor.ToString(); }


// **********************************
// **     Reco and MC matching     **
// **********************************


// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::MatchHits()
{
  this->MatchPointsAndHits<ca::EDetectorID::kMvd>();
  this->MatchPointsAndHits<ca::EDetectorID::kSts>();
  this->MatchPointsAndHits<ca::EDetectorID::kMuch>();
  this->MatchPointsAndHits<ca::EDetectorID::kTrd>();
  this->MatchPointsAndHits<ca::EDetectorID::kTof>();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::MatchTracks()
{
  this->MatchRecoAndMCTracks();
  this->InitTrackInfo();
  for (const auto& trkMC : fpMCData->GetTrackContainer()) {
    if (trkMC.IsReconstructable()) {
      fMonitor.IncrementCounter(EMonitorKey::kMcTrackReconstructable);
    }
  }
}


// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::MatchRecoAndMCTracks()
{
  for (int iTre = 0; iTre < int(fpvRecoTracks->size()); ++iTre) {
    auto& trkRe = (*fpvRecoTracks)[iTre];

    // ----- Count number of hits from each MC track belonging to this reconstructed track
    auto& mNofHitsVsMCTrkID = trkRe.hitMap;
    mNofHitsVsMCTrkID.clear();
    for (int iH : trkRe.Hits) {
      auto& vP = (*fpvQaHits)[iH].GetMcPointIds();
      for (int iP : vP) {
        if (iP < 0) {
          continue;
        }
        int iTmc = fpMCData->GetPoint(iP).GetTrackId();
        mNofHitsVsMCTrkID[iTmc]++;
      }
    }  // loop over hit ids stored for a reconstructed track: end


    // ----- Calculate track max purity
    // NOTE: Maximal purity is a maximum fraction of hits from a single MC track. A reconstructed track can be matched
    //       to several MC tracks, because it uses hits from different particle. Purity shows, how fully the true track
    //       was reconstructed.
    int nHitsTrkRe   = trkRe.Hits.size();  // number of hits in a given reconstructed track
    double maxPurity = 0.;                 // [0, 1]
    for (auto& item : mNofHitsVsMCTrkID) {
      int iTmc       = item.first;
      int nHitsTrkMc = item.second;

      if (iTmc < 0) {
        continue;
      }

      if (double(nHitsTrkMc) > double(nHitsTrkRe) * maxPurity) {
        maxPurity = double(nHitsTrkMc) / double(nHitsTrkRe);
      }

      auto& trkMc = fpMCData->GetTrack(iTmc);

      // Match reconstructed and MC tracks, if purity with this MC track passes the threshold
      if (double(nHitsTrkMc) >= CbmL1Constants::MinPurity * double(nHitsTrkRe)) {
        trkMc.AddRecoTrackIndex(iTre);
        trkRe.AddMCTrackIndex(iTmc);
      }
      // If purity is low, but some hits of a given MC track are presented in the reconstructed track
      else {
        trkMc.AddTouchTrackIndex(iTre);
      }

      // Update max purity of the reconstructed track
      trkRe.SetMaxPurity(maxPurity);
    }  // loop over hit map: end
  }    // loop over reconstructed tracks: end
}

// *******************************
// **     Utility functions     **
// *******************************

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::CheckInit() const
{
  // Check parameters
  if (!fpParameters.get()) {
    throw std::logic_error("Tracking parameters object was not defined");
  }

  // Check output data containers
  if (!fpMCData) {
    throw std::logic_error("MC data object was not registered");
  }
  if (!fpvRecoTracks) {
    throw std::logic_error("Reconstructed track container was not registered");
  }
  if (!fpvHitIds) {
    throw std::logic_error("Hit index container was not registered");
  }
  if (!fpvQaHits) {
    throw std::logic_error("QA hit container was not registered");
  }
  if (!fpvFstHitId) {
    throw std::logic_error("Array of first hit indexes in each detector was not registered");
  }

  // Check event list
  if (!fpMCEventList) {
    throw std::logic_error("MC event list was not found");
  }
  if (!fpTimeSlice) {
    throw std::logic_error("Time slice was not found");
  }

  // Tracks branch
  if (!fpMCTracks) {
    throw std::logic_error("MC tracks branch is unavailable");
  }

  // Event header
  if (!fpMCEventHeader) {
    throw std::logic_error("MC event header is unavailable");
  }

  // Check detectors initialization
  for (int iD = 0; iD < static_cast<int>(ca::EDetectorID::END); ++iD) {
    if (fvbUseDet[iD]) {
      if (!fvpBrPoints[iD]) {
        throw std::logic_error(Form("MC points are unavailable for %s", kDetName[iD]));
      }
      if (!fvpBrHitMatches[iD]) {
        throw std::logic_error(Form("Hit matches are unavailable for %s", kDetName[iD]));
      }
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<>
void MCModule::ReadMCPointsForDetector<ca::EDetectorID::kTof>()
{
  if (!fvbUseDet[ca::EDetectorID::kTof]) {
    return;
  }

  auto* pBrHitMatches = fvpBrHitMatches[ca::EDetectorID::kTof];
  auto* pBrPoints     = fvpBrPoints[ca::EDetectorID::kTof];

  // FIXME: Use mask from CbmTofAddress (but test before!!)
  constexpr int kNofBitsRpcAddress = 11;

  for (const auto& [iFile, iEvent] : fFileEventIDs) {
    // Fill map of flags: the external index of the matched point vs. iTrExt and the RPC address
    int nPoints = pBrPoints->Size(iFile, iEvent);
    std::map<std::pair<int, int>, int> mMatchedPointId;  // map (iTr, addr) -> is matched
    for (int iH = 0; iH < pBrHitMatches->GetEntriesFast(); ++iH) {
      auto* pHitMatch = dynamic_cast<CbmMatch*>(pBrHitMatches->At(iH));
      LOG_IF(fatal, !pHitMatch) << "CA MC Module: hit match was not found for TOF hit " << iH;
      double bestWeight = 0;
      for (int iLink = 0; iLink < pHitMatch->GetNofLinks(); ++iLink) {
        const auto& link = pHitMatch->GetLink(iLink);
        if (link.GetFile() != iFile || link.GetEntry() != iEvent) {
          continue;
        }
        int iPext = link.GetIndex();
        if (iPext < 0) {
          continue;
        }
        auto* pExtPoint = dynamic_cast<CbmTofPoint*>(pBrPoints->Get(link));
        if (!pExtPoint) {
          LOG(error) << "ca::MCModule: MC Point with link=" << link.ToString() << " does not exist in branch";
          continue;
        }

        int trkId   = pExtPoint->GetTrackID();
        int rpcAddr = pExtPoint->GetDetectorID() << kNofBitsRpcAddress;  // FIXME:
        auto key    = std::make_pair(trkId, rpcAddr);
        auto prev   = mMatchedPointId.find(key);
        if (prev == mMatchedPointId.end()) {
          bestWeight           = link.GetWeight();
          mMatchedPointId[key] = iPext;
        }
        else {  // If we find two links for the same interaction, we select the one with the largest weight
          if (bestWeight < link.GetWeight()) {
            mMatchedPointId[key] = iPext;
          }
        }
      }
    }  // iH

    // Select one point for the given track ID and RPC address. If at least one of the points for (trackID, RPC address)
    // produced a hit, the point from the best link is selected. Otherwise the closest point to the RPC z-center is selected
    {
      int iPointSelected = -1;
      int iTmcCurr       = -1;
      int rpcAddrCurr    = -1;
      bool bTrkHasHits   = false;
      double zDist       = std::numeric_limits<double>::max();
      double zCell       = std::numeric_limits<double>::signaling_NaN();
      for (int iP = 0; iP < nPoints; ++iP) {
        auto* pExtPoint = dynamic_cast<CbmTofPoint*>(pBrPoints->Get(iFile, iEvent, iP));
        LOG_IF(fatal, !pExtPoint) << "NO POINT: TOF: file, event, index = " << iFile << ' ' << iEvent << ' ' << iP;

        int iTmc    = pExtPoint->GetTrackID();
        int rpcAddr = pExtPoint->GetDetectorID() << kNofBitsRpcAddress;
        // New point
        if (rpcAddrCurr != rpcAddr || iTmcCurr != iTmc) {  // The new interaction of the MC track with the TOF RPC
          if (iPointSelected != -1) {
            auto oPoint = FillMCPoint<ca::EDetectorID::kTof>(iPointSelected, iEvent, iFile);
            if (oPoint) {
              fpMCData->AddPoint(*oPoint);
            }
          }
          iTmcCurr    = iTmc;
          rpcAddrCurr = rpcAddr;
          auto key    = std::make_pair(iTmc, rpcAddr);
          auto found  = mMatchedPointId.find(key);
          bTrkHasHits = found != mMatchedPointId.end();
          if (bTrkHasHits) {
            iPointSelected = found->second;
          }
          else {
            // First iteration
            zCell          = fvpDetInterface[ca::EDetectorID::kTof]->GetZrefModule(pExtPoint->GetDetectorID());
            zDist          = std::fabs(zCell - pExtPoint->GetZ());
            iPointSelected = iP;
          }
        }
        else {
          if (!bTrkHasHits) {
            auto newDist = std::fabs(pExtPoint->GetZ() - zCell);
            if (newDist < zDist) {
              zDist          = newDist;
              iPointSelected = iP;
            }
          }
        }
      }
      // Add the last point
      if (iPointSelected != -1) {
        auto oPoint = FillMCPoint<ca::EDetectorID::kTof>(iPointSelected, iEvent, iFile);
        if (oPoint) {
          fpMCData->AddPoint(*oPoint);
        }
      }
    }
  }  // [iFile, iEvent]
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::ReadMCPoints()
{
  int nPointsEstimated = 5 * fpMCData->GetNofTracks() * fpParameters->GetNstationsActive();
  fpMCData->ReserveNofPoints(nPointsEstimated);

  DetIdArr_t<int> vNofPointsDet = {{0}};
  for (const auto& [iFile, iEvent] : fFileEventIDs) {
    for (int iD = 0; iD < static_cast<int>(vNofPointsDet.size()); ++iD) {
      if (fvbUseDet[iD]) {
        vNofPointsDet[iD] = fvpBrPoints[iD]->Size(iFile, iEvent);
      }
      fpMCData->SetNofPointsOrig(static_cast<ca::EDetectorID>(iD), vNofPointsDet[iD]);
    }
  }

  // ----- Read MC points in MVD, STS, MuCh, TRD and TOF
  ReadMCPointsForDetector<ca::EDetectorID::kMvd>();
  ReadMCPointsForDetector<ca::EDetectorID::kSts>();
  ReadMCPointsForDetector<ca::EDetectorID::kMuch>();
  ReadMCPointsForDetector<ca::EDetectorID::kTrd>();
  ReadMCPointsForDetector<ca::EDetectorID::kTof>();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void MCModule::ReadMCTracks()
{
  // ----- Total number of tracks
  int nTracksTot = 0;
  for (const auto& [iFile, iEvent] : fFileEventIDs) {
    if (iFile < 0 || iEvent < 0) {
      continue;
    }
    nTracksTot += fpMCTracks->Size(iFile, iEvent);  /// iFile, iEvent
  }
  fpMCData->ReserveNofTracks(nTracksTot);

  // ----- Loop over MC events
  for (const auto& [iFile, iEvent] : fFileEventIDs) {
    if (iFile < 0 || iEvent < 0) {
      continue;
    }
    auto pEvtHeader = dynamic_cast<FairMCEventHeader*>(fpMCEventHeader->Get(iFile, iEvent));
    if (!pEvtHeader) {
      LOG(fatal) << "cbm::ca::MCModule: event header is not found for file " << iFile << " and event " << iEvent;
    }
    double eventTime = fpMCEventList->GetEventTime(iEvent, iFile);

    // ----- Loop over MC tracks
    int nTracks = fpMCTracks->Size(iFile, iEvent);
    for (int iTrkExt = 0; iTrkExt < nTracks; ++iTrkExt) {
      CbmMCTrack* pExtMCTrk = dynamic_cast<CbmMCTrack*>(fpMCTracks->Get(iFile, iEvent, iTrkExt));
      if (!pExtMCTrk) {
        LOG(warn) << "cbm::ca::MCModule: track with (iF, iE, iT) = " << iFile << ' ' << iEvent << ' ' << iTrkExt
                  << " not found";
      }
      // Create a CbmL1MCTrack
      auto aTrk = MCTrack{};

      aTrk.SetId(fpMCData->GetNofTracks());  // assign current number of tracks read so far as an ID of a new track
      aTrk.SetExternalId(iTrkExt);           // external index of track is its index from CbmMCTrack objects container
      aTrk.SetEventId(iEvent);
      aTrk.SetFileId(iFile);

      aTrk.SetStartT(pExtMCTrk->GetStartT() + eventTime);
      aTrk.SetStartX(pExtMCTrk->GetStartX());
      aTrk.SetStartY(pExtMCTrk->GetStartY());
      aTrk.SetStartZ(pExtMCTrk->GetStartZ());

      aTrk.SetPx(pExtMCTrk->GetPx());
      aTrk.SetPy(pExtMCTrk->GetPy());
      aTrk.SetPz(pExtMCTrk->GetPz());

      aTrk.SetFlagSignal(aTrk.IsPrimary() && (aTrk.GetStartZ() > (pEvtHeader->GetZ() + 1.e-10)));

      // In CbmMCTrack mass is defined from ROOT PDG data base. If track is left from ion, and its pdg is not registered
      // in the data base, its mass is calculated as A times proton mass.
      aTrk.SetMass(pExtMCTrk->GetMass());

      // The charge in CbmMCTrack is given in the units of e
      aTrk.SetCharge(pExtMCTrk->GetCharge());

      // Set index of mother track. We assume, that mother track was recorded into the internal array before
      int extMotherId    = pExtMCTrk->GetMotherId();
      int extChainId     = iTrkExt;
      int extChainParent = extMotherId;
      while (extChainParent >= 0) {
        extChainId                = extChainParent;
        const auto* pExtParentTrk = dynamic_cast<CbmMCTrack*>(fpMCTracks->Get(iFile, iEvent, extChainParent));
        extChainParent            = pExtParentTrk->GetMotherId();
      }

      if (extMotherId < 0) {
        // This is a primary track, and it's mother ID equals -1 or -2. This value is taken also as an internal mother
        // ID. The same rules here and below are applied to the chainId
        aTrk.SetMotherId(extMotherId);
        aTrk.SetChainId(extChainId);
      }
      else {
        // This is a secondary track, mother ID should be recalculated for the internal track array.
        int motherId = fpMCData->FindInternalTrackIndex(extMotherId, iEvent, iFile);
        int chainId  = fpMCData->FindInternalTrackIndex(extChainId, iEvent, iFile);
        if (motherId == -1) {
          motherId = -3;
        }  // Mother is neutral particle, which is rejected
        aTrk.SetMotherId(motherId);
        aTrk.SetChainId(chainId);
      }

      aTrk.SetProcessId(pExtMCTrk->GetGeantProcessId());
      aTrk.SetPdgCode(pExtMCTrk->GetPdgCode());

      fpMCData->AddTrack(aTrk);
    }  // Loop over MC tracks: end
  }    // Loop over MC events: end
}
