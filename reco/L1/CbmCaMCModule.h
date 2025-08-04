/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaMCModule.h
/// @brief  CA Tracking performance interface for CBM (header)
/// @since  23.09.2022
/// @author S.Zharko <s.zharko@gsi.de>

#ifndef CbmCaMCModule_h
#define CbmCaMCModule_h 1

#include "CaMonitor.h"
#include "CaParameters.h"
#include "CaToolsMCData.h"
#include "CaToolsMCPoint.h"
#include "CaVector.h"
#include "CbmL1DetectorID.h"
#include "CbmL1Hit.h"
#include "CbmL1Track.h"
#include "CbmLink.h"
#include "CbmMCDataArray.h"
#include "CbmMCEventList.h"
#include "CbmMCTrack.h"
#include "CbmMatch.h"
#include "CbmMuchPoint.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdPoint.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmStsPoint.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTimeSlice.h"
#include "CbmTofPoint.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdPoint.h"
#include "CbmTrdTrackingInterface.h"
#include "TClonesArray.h"
#include "TDatabasePDG.h"

#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

class CbmEvent;
class CbmMCDataObject;
class CbmL1HitDebugInfo;
class CbmL1Track;

namespace cbm::ca
{
  namespace ca = cbm::algo::ca;

  /// @brief Class CbmCaPerformance is an interface to communicate between
  ///
  class MCModule {


   public:
    /// @brief Constructor
    /// @param verbosity  Verbosity level
    /// @param perfMode   Performance mode (defines cut on number of consecutive stations with hit or point)
    MCModule(int verb = 1, int perfMode = 1) : fVerbose(verb), fPerformanceMode(perfMode)
    {
      LOG(info) << "cbm::ca::MCModule: performance mode = " << fPerformanceMode;
    }

    /// @brief Destructor
    ~MCModule() = default;

    /// @brief Copy constructor
    MCModule(const MCModule&) = delete;

    /// @brief Move constructor
    MCModule(MCModule&&) = delete;

    /// @brief Copy assignment operator
    MCModule& operator=(const MCModule&) = delete;

    /// @brief Move assignment operator
    MCModule& operator=(MCModule&&) = delete;


    /// @brief Defines performance action in the end of the run
    void Finish();

    /// @brief Gets a pointer to MC data object
    const tools::MCData* GetMCData() const { return fpMCData; }

    /// @brief Defines performance action in the beginning of each event or time slice
    /// @note This function should be called before hits initialization
    /// @param  pEvent Pointer to a current CbmEvent
    void InitEvent(CbmEvent* pEvent);

    /// @brief Defines action on the module in the beginning of the run
    /// @return Success flag
    bool InitRun();

    /// @brief Matches hit with MC point
    /// @tparam  DetId Detector ID
    /// @param   iHitExt  External index of hit
    /// @return           MC-point index in fvMCPoints array
    ///
    /// This method finds a match for a given hit or matches for hits clusters (in case of STS), finds the best
    /// link in the match and returns the corresponding global ID of the MC points.
    template<ca::EDetectorID DetId>
    std::tuple<int, std::vector<int>> MatchHitWithMc(int iHitExt);

    /// @brief Match reconstructed hits and MC points
    void MatchHits();

    /// @brief Match reconstructed and MC data
    ///
    /// Runs matching of reconstructed tracks with MC ones. Reconstructed tracks are updated with true information.
    void MatchTracks();

    /// @brief Processes event
    ///
    /// Fills histograms and tables, should be called after the tracking done
    void ProcessEvent(CbmEvent* pEvent);

    /// @brief Sets first hit indexes container in different detectors
    /// @param source Array of indexes
    void RegisterFirstHitIndexes(const std::array<int, constants::size::MaxNdetectors + 1>& source)
    {
      fpvFstHitId = &source;
    }

    /// @brief Sets used detector subsystems
    /// @param  detID  Id of detector
    /// @param  flag   Flag: true - detector is used
    /// @note Should be called before this->Init()
    void SetDetector(ca::EDetectorID detID, bool flag) { fvbUseDet[detID] = flag; }

    /// @brief Registers MC data object
    /// @param mcData  Instance of MC data
    void RegisterMCData(tools::MCData& mcData) { fpMCData = &mcData; }

    /// @brief Registers reconstructed track container
    /// @param vRecoTrack Reference to reconstructed track container
    void RegisterRecoTrackContainer(ca::Vector<CbmL1Track>& vRecoTracks) { fpvRecoTracks = &vRecoTracks; }

    /// @brief Registers hit index container
    /// @param vHitIds  Reference to hit index container
    void RegisterHitIndexContainer(ca::Vector<CbmL1HitId>& vHitIds) { fpvHitIds = &vHitIds; }

    /// @brief Registers CA parameters object
    /// @param pParameters  A shared pointer to the parameters object
    void RegisterParameters(std::shared_ptr<ca::Parameters<float>>& pParameters) { fpParameters = pParameters; }

    /// @brief Registers debug hit container
    /// @param vQaHits  Reference to debug hit container
    void RegisterQaHitContainer(ca::Vector<CbmL1HitDebugInfo>& vQaHits) { fpvQaHits = &vQaHits; }

    /// @brief Gets verbosity level
    int GetVerbosity() const { return fVerbose; }

    /// @brief Check class initialization
    /// @note The function throws std::logic_error, if initialization is incomplete
    void CheckInit() const;

    /// @brief  Initializes MC track
    ///
    /// Initializes information about arrangement of points and hits of MC tracks within stations and the status
    /// of track ability to be reconstructed, calculates max number of points and hits on a station, number of
    /// consecutive stations containing a hit or point and number of stations and points with hits.
    void InitTrackInfo();

    /// @brief  Match sets of MC points and reconstructed hits for a given detector
    /// @tparam  DetID  Index of the detector
    ///
    /// Writes indexes of MC points to each hit and indexes of hits to each MC point.
    template<ca::EDetectorID DetID>
    void MatchPointsAndHits();

    /// @brief Matches reconstructed tracks with MC tracks
    ///
    /// In the procedure, the maps of associated MC track indexes to corresponding number of hits are filled out and the
    /// max purity is calculated for each reconstructed track in the TS/event. If the value of purity for a given MC track
    /// is larger then a threshold, the corresponding track index is saved to the reconstructed track object, in the same
    /// time the index of the reconstructed track object is saved to this MC track object. If purity for the MC track does
    /// not pass the threshold, its index will not be accounted as a matched track, and the index of reconstructed track
    /// will be added as an index of "touched" track.
    void MatchRecoAndMCTracks();

   private:
    /// @brief Reads MC tracks from external trees and saves them to MCDataObject
    void ReadMCTracks();

    /// @brief Reads MC points from external trees and saves them to MCDataObject
    void ReadMCPoints();

    /// @brief Reads MC points in particular detector
    template<ca::EDetectorID DetID>
    void ReadMCPointsForDetector();

    /// @brief Fills a single detector-specific MC point
    /// @tparam     DetID      Detector subsystem ID
    /// @param[in]  iExtId     Index of point in the external points container
    /// @param[in]  iEvent     EventID of point in the external points container
    /// @param[in]  iFile      FileID of point int the external points container
    /// @param[out] intMCPoint Reference to the internal tracking MC point object (ca::tools::MCData)
    /// @return true   Point is correct and is to be saved to the MCData object
    /// @return false  Point is incorrect and will be ignored
    template<ca::EDetectorID DetID>
    std::optional<tools::MCPoint> FillMCPoint(int iExtId, int iEvent, int iFile);

    /// @enum  EMonitorKey
    /// @brief Monitor keys
    enum class EMonitorKey
    {
      kMcTrack,                 ///< Number of MC tracks
      kMcTrackReconstructable,  ///< Number of reconstructable MC tracks
      kMcPoint,                 ///< Number of MC points
      kRecoNevents,             ///< Number of events
      kMissedMatchesMvd,        ///< Number of missed matches in MVD
      kMissedMatchesSts,        ///< Number of missed matches in STS
      kMissedMatchesMuch,       ///< Number of missed matches in MuCh
      kMissedMatchesTrd,        ///< Number of missed matches in TRD
      kMissedMatchesTof,        ///< Number of missed TOF matches
      END
    };
    ca::Monitor<EMonitorKey> fMonitor{"CA MC Module"};  ///< Monitor

    // ------ Flags
    DetIdArr_t<bool> fvbUseDet = {{false}};  ///< Flag: is detector subsystem used
    int fVerbose               = 1;          ///< Verbosity level
    int fPerformanceMode       = -1;         ///< Mode of performance

    std::shared_ptr<ca::Parameters<float>> fpParameters = nullptr;  ///< Pointer to tracking parameters object
    // ------ Input data branches
    const CbmTimeSlice* fpTimeSlice  = nullptr;  ///< Current time slice
    CbmMCEventList* fpMCEventList    = nullptr;  ///< MC event list
    CbmMCDataObject* fpMCEventHeader = nullptr;  ///< MC event header
    CbmMCDataArray* fpMCTracks       = nullptr;  ///< MC tracks input

    DetIdArr_t<CbmTrackingDetectorInterfaceBase*> fvpDetInterface = {{nullptr}};  ///< Tracking detector interface

    DetIdArr_t<CbmMCDataArray*> fvpBrPoints   = {{nullptr}};  ///< Array of points vs. detector
    DetIdArr_t<TClonesArray*> fvpBrHitMatches = {{nullptr}};  ///< Array of hit match branches vs. detector

    // Matching information
    std::set<std::pair<int, int>> fFileEventIDs;  ///< Set of file and event indexes: first - iFile, second - iEvent
    int fBestMcFile  = -1;                        ///< Index of bestly matched MC file
    int fBestMcEvent = -1;                        ///< Index of bestly matched MC event


    // ----- Internal MC data
    tools::MCData* fpMCData = nullptr;  ///< MC information (hits and tracks) instance

    // ----- Internal reconstructed data
    ca::Vector<CbmL1Track>* fpvRecoTracks    = nullptr;  ///< Pointer to reconstructed track container
    ca::Vector<CbmL1HitId>* fpvHitIds        = nullptr;  ///< Pointer to hit index container
    ca::Vector<CbmL1HitDebugInfo>* fpvQaHits = nullptr;  ///< Pointer to QA hit container

    /// @brief Pointer to array of first hit indexes in the detector subsystem
    ///
    /// This array must be initialized in the run initialization function.
    const std::array<int, constants::size::MaxNdetectors + 1>* fpvFstHitId = nullptr;
  };


  // **********************************************
  // **     Template function implementation     **
  // **********************************************

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  std::tuple<int, std::vector<int>> MCModule::MatchHitWithMc(int iHitExt)
  {
    int iPoint = -1;
    std::vector<int> vPoints;
    const auto* pHitMatch = dynamic_cast<CbmMatch*>(fvpBrHitMatches[DetID]->At(iHitExt));
    if (!pHitMatch) {
      LOG(warn) << "Hit match with index " << iHitExt << " is missing for " << kDetName[DetID];
      if constexpr (ca::EDetectorID::kMvd == DetID) {
        fMonitor.IncrementCounter(EMonitorKey::kMissedMatchesMvd);
      }
      else if constexpr (ca::EDetectorID::kSts == DetID) {
        fMonitor.IncrementCounter(EMonitorKey::kMissedMatchesSts);
      }
      else if constexpr (ca::EDetectorID::kMuch == DetID) {
        fMonitor.IncrementCounter(EMonitorKey::kMissedMatchesMuch);
      }
      else if constexpr (ca::EDetectorID::kTrd == DetID) {
        fMonitor.IncrementCounter(EMonitorKey::kMissedMatchesTrd);
      }
      else if constexpr (ca::EDetectorID::kTof == DetID) {
        fMonitor.IncrementCounter(EMonitorKey::kMissedMatchesTof);
      }
      return std::tuple(iPoint, vPoints);
    }

    for (int iLink = 0; iLink < pHitMatch->GetNofLinks(); ++iLink) {
      const auto& link = pHitMatch->GetLink(iLink);
      int iPointExt    = link.GetIndex();
      int iEvent       = link.GetEntry();
      int iFile        = link.GetFile();
      if (iPointExt < 0) continue;
      int id = fpMCData->FindInternalPointIndex(DetID, iPointExt, iEvent, iFile);
      vPoints.push_back(id);
    }

    if (pHitMatch->GetNofLinks() > 0) {
      const auto& link = pHitMatch->GetMatchedLink();
      if (link.GetIndex() > -1) {
        int index = link.GetIndex();
        int event = link.GetEntry();
        int file  = link.GetFile();
        iPoint    = fpMCData->FindInternalPointIndex(DetID, index, event, file);
      }
    }

    return std::tuple(iPoint, vPoints);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  std::optional<tools::MCPoint> MCModule::FillMCPoint(int iExtId, int iEvent, int iFile)
  {
    auto oPoint = std::make_optional<tools::MCPoint>();

    // ----- Get positions, momenta, time and track ID
    TVector3 posIn;   // Position at entrance to station [cm]
    TVector3 posOut;  // Position at exist of station [cm]
    TVector3 momIn;   // 3-momentum at entrance to station [GeV/c]
    TVector3 momOut;  // 3-momentum at exit of station [GeV/c]

    auto* pBrPoints = fvpBrPoints[DetID];

    PointTypes_t::at<DetID>* pExtPoint = dynamic_cast<PointTypes_t::at<DetID>*>(pBrPoints->Get(iFile, iEvent, iExtId));
    if (!pExtPoint) {
      LOG(warn) << "CbmCaMCModule: " << kDetName[DetID] << " MC point with iExtId = " << iExtId
                << ", iEvent = " << iEvent << ", iFile = " << iFile << " does not exist";
      return std::nullopt;
    }
    if constexpr (ca::EDetectorID::kMvd == DetID) {
      pExtPoint->Position(posIn);
      pExtPoint->PositionOut(posOut);
      pExtPoint->Momentum(momIn);
      pExtPoint->MomentumOut(momOut);
    }
    // STS
    else if constexpr (ca::EDetectorID::kSts == DetID) {
      pExtPoint->Position(posIn);
      pExtPoint->PositionOut(posOut);
      pExtPoint->Momentum(momIn);
      pExtPoint->MomentumOut(momOut);
    }
    // MuCh
    else if constexpr (ca::EDetectorID::kMuch == DetID) {
      pExtPoint->Position(posIn);
      pExtPoint->PositionOut(posOut);
      pExtPoint->Momentum(momIn);
      pExtPoint->Momentum(momOut);
    }
    // TRD
    else if constexpr (ca::EDetectorID::kTrd == DetID) {
      pExtPoint->Position(posIn);
      pExtPoint->PositionOut(posOut);
      pExtPoint->Momentum(momIn);
      pExtPoint->MomentumOut(momOut);
    }
    // TOF
    else if constexpr (ca::EDetectorID::kTof == DetID) {
      pExtPoint->Position(posIn);
      pExtPoint->Position(posOut);
      pExtPoint->Momentum(momIn);
      pExtPoint->Momentum(momOut);
    }
    double time = pExtPoint->GetTime();
    int iTmcExt = pExtPoint->GetTrackID();

    if (iTmcExt < 0) {
      LOG(warn) << "CbmCaMCModule: For MC point with iExtId = " << iExtId << ", iEvent = " << iEvent
                << ", iFile = " << iFile << " MC track is undefined (has ID = " << iTmcExt << ')';
      return std::nullopt;
    }
    TVector3 posMid = 0.5 * (posIn + posOut);
    TVector3 momMid = 0.5 * (momIn + momOut);

    // // ----- Get station index
    int iStLoc = fvpDetInterface[DetID]->GetTrackingStationIndex(pExtPoint);
    if (iStLoc < 0) {
      return std::nullopt;
    }

    int stationID = fpParameters->GetStationIndexActive(iStLoc, DetID);
    if (stationID == -1) {
      return std::nullopt;
    }  // Skip points from inactive stations

    // Update point time with event time
    time += fpMCEventList->GetEventTime(iEvent, iFile);

    // ----- Reject MC points falling out of the time slice
    // STS, MuCh, TRD, TOF
    if constexpr (DetID != ca::EDetectorID::kMvd) {
      // TODO: SZh 18.06.2024: Avoid dependency from CbmTimeSlice, if possible (no TimeSlice branch in the online unpack)
      double startT = fpTimeSlice->GetStartTime();
      double endT   = fpTimeSlice->GetEndTime();

      if ((startT > 0. && time < startT) || (endT > 0. && time > endT)) {
        LOG(warn) << "CbmCaMCModule: MC point with iExtId = " << iExtId << ", iEvent = " << iEvent
                  << ", iFile = " << iFile << " and det id " << int(DetID) << " fell out of the TS duration [" << startT
                  << ", " << endT << "] with measured time = " << time << " [ns]";
        return std::nullopt;
      }
    }

    // ----- Fill MC point
    oPoint->SetExternalId(fpMCData->GetPointGlobExtIndex(DetID, iExtId));
    oPoint->SetEventId(iEvent);
    oPoint->SetFileId(iFile);
    oPoint->SetTime(time);
    oPoint->SetX(posMid.X());
    oPoint->SetY(posMid.Y());
    oPoint->SetZ(posMid.Z());
    oPoint->SetXIn(posIn.X());
    oPoint->SetYIn(posIn.Y());
    oPoint->SetZIn(posIn.Z());
    oPoint->SetXOut(posOut.X());
    oPoint->SetYOut(posOut.Y());
    oPoint->SetZOut(posOut.Z());
    oPoint->SetPx(momMid.X());
    oPoint->SetPy(momMid.Y());
    oPoint->SetPz(momMid.Z());
    oPoint->SetPxIn(momIn.X());
    oPoint->SetPyIn(momIn.Y());
    oPoint->SetPzIn(momIn.Z());
    oPoint->SetPxOut(momOut.X());
    oPoint->SetPyOut(momOut.Y());
    oPoint->SetPzOut(momOut.Z());

    // Select current number of points as a local id of points
    oPoint->SetId(fpMCData->GetNofPoints());

    // Match MC track and point to each other
    int iTmcInt = fpMCData->FindInternalTrackIndex(iTmcExt, iEvent, iFile);

    oPoint->SetTrackId(iTmcInt);
    if (iTmcInt > -1) {
      fpMCData->GetTrack(iTmcInt).AddPointIndex(oPoint->GetId());
    }

    oPoint->SetStationId(stationID);
    oPoint->SetDetectorId(DetID);

    auto* pExtTrk = dynamic_cast<CbmMCTrack*>(fpMCTracks->Get(iFile, iEvent, iTmcExt));
    if (!pExtTrk) {
      LOG(warn) << "CbmCaMCModule: MC track with iTmcExt = " << iTmcExt << ", iEvent = " << iEvent
                << ", iFile = " << iFile << " MC track is undefined (nullptr)";
      return std::nullopt;
    }
    oPoint->SetPdgCode(pExtTrk->GetPdgCode());
    oPoint->SetMotherId(pExtTrk->GetMotherId());

    auto* pPdgDB = TDatabasePDG::Instance()->GetParticle(oPoint->GetPdgCode());
    oPoint->SetMass(pPdgDB ? pPdgDB->Mass() : 0.);  /// TODO: Set from track
    oPoint->SetCharge(pPdgDB ? pPdgDB->Charge() / 3. : 0.);

    return oPoint;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  void MCModule::MatchPointsAndHits()
  {
    if (!fvbUseDet[DetID]) {
      return;
    }

    for (int iH = (*fpvFstHitId)[static_cast<int>(DetID)]; iH < (*fpvFstHitId)[static_cast<int>(DetID) + 1]; ++iH) {
      auto& hit            = (*fpvQaHits)[iH];
      auto [iBestP, vAllP] = MatchHitWithMc<DetID>(hit.ExtIndex);
      if (iBestP >= 0) {
        hit.SetBestMcPointId(iBestP);
      }
      for (auto iP : vAllP) {
        if (iP >= 0) {
          hit.AddMcPointId(iP);
          fpMCData->GetPoint(iP).AddHitID(iH);
        }
      }
    }
  }
  // -------------------------------------------------------------------------------------------------------------------
  // NOTE: template is used, because another template function FillMCPoint is used inside
  template<ca::EDetectorID DetID>
  void MCModule::ReadMCPointsForDetector()
  {
    if (!fvbUseDet[DetID]) {
      return;
    }
    for (const auto& [iFile, iEvent] : fFileEventIDs) {
      int nPointsEvent = fvpBrPoints[DetID]->Size(iFile, iEvent);
      for (int iP = 0; iP < nPointsEvent; ++iP) {
        std::optional<tools::MCPoint> oPoint = FillMCPoint<DetID>(iP, iEvent, iFile);
        if (oPoint) {
          fpMCData->AddPoint(*oPoint);
        }
      }  // iP: end
    }    // key: end
  }
}  // namespace cbm::ca

#endif  // CbmCaMCModule_h
