/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaIdealHitProducerDetBase.h
/// @brief  Base class for the ideal hit producer task (header)
/// @author S.Zharko
/// @since  30.05.2023

#pragma once

#include "CaAlgoRandom.h"
#include "CaDefs.h"
#include "CaUvConverter.h"
#include "CbmEvent.h"
#include "CbmL1DetectorID.h"
#include "CbmMCDataArray.h"
#include "CbmMCDataManager.h"
#include "CbmMCDataObject.h"
#include "CbmMCEventList.h"
#include "CbmMatch.h"
#include "CbmMuchAddress.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchPoint.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdHit.h"
#include "CbmMvdPoint.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmStsHit.h"
#include "CbmStsPoint.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTimeSlice.h"
#include "CbmTofAddress.h"
#include "CbmTofHit.h"
#include "CbmTofPoint.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrackingDetectorInterfaceBase.h"
#include "CbmTrdHit.h"
#include "CbmTrdPoint.h"
#include "CbmTrdTrackingInterface.h"
#include "FairRootManager.h"
#include "FairTask.h"
#include "TClonesArray.h"
#include "TVector3.h"

#include <Logger.h>

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace cbm::ca
{
  namespace constants = cbm::algo::ca::constants;

  /// @brief Ideal hit producer class
  ///
  ///
  template<ca::EDetectorID DetID>
  class IdealHitProducerDet {
   public:
    using Hit_t   = HitTypes_t::at<DetID>;
    using Point_t = PointTypes_t::at<DetID>;

    /// @brief Constructor
    IdealHitProducerDet() = default;

    /// @brief Destructor
    ~IdealHitProducerDet();

    /// @brief Copy constructor
    IdealHitProducerDet(const IdealHitProducerDet&) = delete;

    /// @brief Move constructor
    IdealHitProducerDet(IdealHitProducerDet&&) = delete;

    /// @brief Copy assignment operator
    IdealHitProducerDet& operator=(const IdealHitProducerDet&) = delete;

    /// @brief Move assignment operator
    IdealHitProducerDet& operator=(IdealHitProducerDet&&) = delete;

    /// @brief Initialization of the task
    InitStatus Init();

    /// @brief Re-initialization of the task
    InitStatus ReInit() { return Init(); }

    /// @brief Execution of the task
    void Exec(Option_t* option);

    /// @brief Sets random seed (1 by default)
    /// @param seed  Random seed
    void SetRandomSeed(int seed) { fRandom.SetSeed(seed); }

    /// @brief Sets YAML configuration file name
    /// @param name  Name of the configuration file
    void SetConfigName(const char* name) { fsConfigName = name; }

   private:
    static constexpr double kHitWSize = 3.5;  ///< half-width for the bounded gaussian [sigma]

    /// @brief Gets pointer to matched MC point
    /// @param iH     Index of hit
    /// @return link to matched point
    std::optional<CbmLink> GetMatchedPointLink(int iH);

    /// @brief Parses the YAML configuration file
    void ParseConfig();

    /// @brief Pushes back a hit into the hits branch
    /// @param pHit  Pointer to source hit
    void PushBackHit(const Hit_t* pHit);

    /// @brief Smears the value by a given standard deviation with a selected function
    /// @param value  Value to be smeared
    /// @param sigma  Value of the standard deviation
    /// @param opt    Function (0 - BoundedGaus, 1 - Uniform)
    void SmearValue(double& value, double sigma, int opt);

    /// @brief Fills map of legit points (inside MC event!)
    void FillPointIsLegit(int iEvGlob);

    /// @brief A structure to keep hit adjustment/creation settings for each station
    struct HitParameters {

      double fPhiU{constants::Undef<double>};  ///< Angle of the first independent measured coordinate [rad]
      double fPhiV{constants::Undef<double>};  ///< Angle of the second independent measured coordinate [rad]

      double fDu{constants::Undef<double>};  ///< Error of u-coordinate measurement [cm]
      double fDv{constants::Undef<double>};  ///< Error of v-coordinate measurement [cm]
      double fDt{constants::Undef<double>};  ///< Error of time measurement [ns]

      /// PDFs selection for different quantities
      /// - 0: Bounded Gaussian distribution
      /// - 1: Uniform distribution
      short fPdfU = -1;
      short fPdfV = -1;
      short fPdfT = -1;

      /// @brief Mode:
      ///  - 0: real hits
      ///  - 1: existing hits are improved from MC points
      ///  - 2: new hits are created from MC points
      short fMode = 0;

      /// @brief Flag:
      ///  - true:  MC point quantities are smeared with gaussian using error
      ///  - false: Precise MC point quantities are used
      bool fbSmear = false;
    };

    CbmTrackingDetectorInterfaceBase* fpDetInterface = nullptr;  ///< Detector interface

    // ----- Input branches:
    CbmTimeSlice* fpTimeSlice        = nullptr;  ///< Current time slice
    TClonesArray* fpRecoEvents       = nullptr;  ///< Array of reconstructed events
    CbmMCEventList* fpMCEventList    = nullptr;  ///< MC event list
    CbmMCDataObject* fpMCEventHeader = nullptr;  ///< MC event header
    CbmMCDataArray* fpBrPoints       = nullptr;  ///< Branch: array of MC points
    CbmMCDataArray* fpBrMCTracks     = nullptr;  ///< Branch: array of MC tracks

    TClonesArray* fpBrHits       = nullptr;  ///< Branch: array of hits
    TClonesArray* fpBrHitMatches = nullptr;  ///< Branch: array of hit matches

    TClonesArray* fpBrHitsTmp       = nullptr;  ///< Temporary array of hits
    TClonesArray* fpBrHitMatchesTmp = nullptr;  ///< Temporary array of hit matches

    ECbmDataType fHitDataType = ECbmDataType::kUnknown;  ///< Hit data type

    std::string fsConfigName = "";  ///< Name of configuration file

    std::vector<HitParameters> fvStationPars;  ///< Parameters, stored for each station

    std::vector<unsigned char> fvbPointIsLegit;  ///< Map of used point index

    ca::Random fRandom{1};  ///< Random generator

    /// @brief Management flag, which does run the routine if there was a request
    bool fbRunTheRoutine = true;


    int fHitCounter = 0;  ///< Hit counter in a new branch
  };


  // **********************************************************
  // **     Template and inline function implementations     **
  // **********************************************************

  // ------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  IdealHitProducerDet<DetID>::~IdealHitProducerDet()
  {
    if (fpBrHitsTmp) {
      fpBrHitsTmp->Delete();
      delete fpBrHitsTmp;
      fpBrHitsTmp = nullptr;
    }

    if (fpBrHitMatchesTmp) {
      fpBrHitMatchesTmp->Delete();
      delete fpBrHitMatchesTmp;
      fpBrHitMatchesTmp = nullptr;
    }
  }

  // ------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  InitStatus IdealHitProducerDet<DetID>::Init()
  try {
    auto CheckBranch = [](auto pBranch, const char* brName) {
      if (!pBranch) {
        throw std::logic_error(Form("Branch %s is not available", brName));
      }
    };

    // ------ Input branch initialization
    auto* pFairManager = FairRootManager::Instance();
    auto* pMcManager   = dynamic_cast<CbmMCDataManager*>(pFairManager->GetObject("MCDataManager"));
    CheckBranch(pMcManager, "MCDataManager");

    fpTimeSlice   = dynamic_cast<CbmTimeSlice*>(pFairManager->GetObject("TimeSlice."));
    fpRecoEvents  = dynamic_cast<TClonesArray*>(pFairManager->GetObject("CbmEvent"));
    fpMCEventList = dynamic_cast<CbmMCEventList*>(pFairManager->GetObject("MCEventList."));

    CheckBranch(fpTimeSlice, "TimeSlice.");
    CheckBranch(fpMCEventList, "MCEventList.");
    fpMCEventHeader = pMcManager->GetObject("MCEventHeader.");
    fpBrMCTracks    = pMcManager->InitBranch("MCTrack");
    CheckBranch(fpBrMCTracks, "MCTrack");

    switch (DetID) {
      case ca::EDetectorID::kMvd:
        fpDetInterface = CbmMvdTrackingInterface::Instance();
        fpBrPoints     = pMcManager->InitBranch("MvdPoint");
        fpBrHits       = dynamic_cast<TClonesArray*>(pFairManager->GetObject("MvdHit"));
        fpBrHitMatches = dynamic_cast<TClonesArray*>(pFairManager->GetObject("MvdHitMatch"));
        fHitDataType   = ECbmDataType::kMvdHit;
        CheckBranch(fpBrPoints, "MvdPoint");
        CheckBranch(fpBrHits, "MvdHit");
        CheckBranch(fpBrHitMatches, "MvdHitMatch");
        break;
      case ca::EDetectorID::kSts:
        fpDetInterface = CbmStsTrackingInterface::Instance();
        fpBrPoints     = pMcManager->InitBranch("StsPoint");
        fpBrHits       = dynamic_cast<TClonesArray*>(pFairManager->GetObject("StsHit"));
        fpBrHitMatches = dynamic_cast<TClonesArray*>(pFairManager->GetObject("StsHitMatch"));
        fHitDataType   = ECbmDataType::kStsHit;
        CheckBranch(fpBrPoints, "StsPoint");
        CheckBranch(fpBrHits, "StsHit");
        CheckBranch(fpBrHitMatches, "StsHitMatch");
        break;
      case ca::EDetectorID::kMuch:
        fpDetInterface = CbmMuchTrackingInterface::Instance();
        fpBrPoints     = pMcManager->InitBranch("MuchPoint");
        fpBrHits       = dynamic_cast<TClonesArray*>(pFairManager->GetObject("MuchPixelHit"));
        fpBrHitMatches = dynamic_cast<TClonesArray*>(pFairManager->GetObject("MuchPixelHitMatch"));
        fHitDataType   = ECbmDataType::kMuchPixelHit;
        CheckBranch(fpBrPoints, "MuchPoint");
        CheckBranch(fpBrHits, "MuchPixelHit");
        CheckBranch(fpBrHitMatches, "MuchPixelHitMatch");
        break;
      case ca::EDetectorID::kTrd:
        fpDetInterface = CbmTrdTrackingInterface::Instance();
        fpBrPoints     = pMcManager->InitBranch("TrdPoint");
        fpBrHits       = dynamic_cast<TClonesArray*>(pFairManager->GetObject("TrdHit"));
        fpBrHitMatches = dynamic_cast<TClonesArray*>(pFairManager->GetObject("TrdHitMatch"));
        fHitDataType   = ECbmDataType::kTrdHit;
        CheckBranch(fpBrPoints, "TrdPoint");
        CheckBranch(fpBrHits, "TrdHit");
        CheckBranch(fpBrHitMatches, "TrdHitMatch");
        break;
      case ca::EDetectorID::kTof:
        fpDetInterface = CbmTofTrackingInterface::Instance();
        fpBrPoints     = pMcManager->InitBranch("TofPoint");
        fpBrHits       = dynamic_cast<TClonesArray*>(pFairManager->GetObject("TofHit"));
        fpBrHitMatches = dynamic_cast<TClonesArray*>(pFairManager->GetObject("TofHitMatch"));
        fHitDataType   = ECbmDataType::kTofHit;
        CheckBranch(fpBrPoints, "TofPoint");
        CheckBranch(fpBrHits, "TofHit");
        CheckBranch(fpBrHitMatches, "TofHitMatch");
        break;
      case ca::EDetectorID::END:
        LOG(fatal) << "CA ideal hit producer for " << kDetName[DetID]
                   << ": Wrong template argument is used: ca::EDetectorID::END";
        break;
    }

    // ------ Checks of the settings of hit creation/adjustment mode
    if (fsConfigName.size() != 0) {
      this->ParseConfig();
      // Skip event/time slice processing, if there are no requests for hit modification of the detector
      // (fbRunTheRoutine = true, if there is at least one station with fMode > 0)
      fbRunTheRoutine =
        std::any_of(fvStationPars.begin(), fvStationPars.end(), [](const auto& p) { return p.fMode > 0; });
    }
    else {
      fbRunTheRoutine = false;
    }

    std::stringstream msg;
    msg << "\033[1;31mATTENTION! IDEAL HIT PRODUCER IS USED FOR " << kDetName[DetID] << "\033[0m";
    LOG_IF(info, fbRunTheRoutine) << msg.str();

    return kSUCCESS;
  }
  catch (const std::logic_error& error) {
    LOG(error) << "CA ideal hit producer for " << kDetName[DetID]
               << ": initialization failed. Reason: " << error.what();
    return kERROR;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  void IdealHitProducerDet<DetID>::ParseConfig()
  {
    int nStations = fpDetInterface->GetNtrackingStations();
    fvStationPars.clear();
    fvStationPars.resize(nStations);

    // Read file
    YAML::Node config;
    try {
      config = YAML::LoadFile(fsConfigName)["ideal_hit_producer"];

      std::string detBranchName = "";
      if constexpr (ca::EDetectorID::kMvd == DetID) {
        detBranchName = "mvd";
      }
      else if constexpr (ca::EDetectorID::kSts == DetID) {
        detBranchName = "sts";
      }
      else if constexpr (ca::EDetectorID::kMuch == DetID) {
        detBranchName = "much";
      }
      else if constexpr (ca::EDetectorID::kTrd == DetID) {
        detBranchName = "trd";
      }
      else if constexpr (ca::EDetectorID::kTof == DetID) {
        detBranchName = "tof";
      }

      const auto& parNode  = config["parameters"][detBranchName];
      const auto& flagNode = config["flags"][detBranchName];

      for (int iSt = 0; iSt < nStations; ++iSt) {
        auto& par  = fvStationPars[iSt];
        auto& node = parNode[iSt];
        short mode = flagNode[iSt].as<int>(0);
        if (mode > 0) {
          par.fPhiU = node["du"]["angle"].as<double>() * 180. / M_PI;
          par.fDu   = node["du"]["value"].as<double>();
          switch (node["du"]["pdf"].as<char>()) {
            case 'g': par.fPdfU = 0; break;
            case 'u': par.fPdfU = 1; break;
            default: par.fPdfU = -1; break;
          }
          par.fPhiV = node["dv"]["angle"].as<double>() * 180. / M_PI;
          par.fDv   = node["dv"]["value"].as<double>();
          switch (node["dv"]["pdf"].as<char>()) {
            case 'g': par.fPdfV = 0; break;
            case 'u': par.fPdfV = 1; break;
            default: par.fPdfV = -1; break;
          }
          par.fDt = node["dt"]["value"].as<double>();
          switch (node["dt"]["pdf"].as<char>()) {
            case 'g': par.fPdfT = 0; break;
            case 'u': par.fPdfT = 1; break;
            default: par.fPdfT = -1; break;
          }
          switch (mode) {
            case 1:
              par.fMode   = 1;
              par.fbSmear = false;
              break;
            case 2:
              par.fMode   = 1;
              par.fbSmear = true;
              break;
            case 3:
              par.fMode   = 2;
              par.fbSmear = false;
              break;
            case 4:
              par.fMode   = 2;
              par.fbSmear = true;
              break;
          }
        }
      }
    }
    catch (const YAML::BadFile& err) {
      std::stringstream msg;
      msg << "YAML configuration file " << fsConfigName << " does not exist";
      throw std::runtime_error(msg.str());
    }
    catch (const YAML::ParserException& err) {
      std::stringstream msg;
      msg << "YAML configuration file " << fsConfigName << " is formatted improperly";
      throw std::runtime_error(msg.str());
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  void IdealHitProducerDet<DetID>::PushBackHit(const HitTypes_t::at<DetID>* pHit)
  {
    // Common fields for all the subsystems
    auto pos  = TVector3{};
    auto dpos = TVector3{};
    pHit->Position(pos);
    pHit->PositionError(dpos);

    // Fields specific to each detector type
    if constexpr (ca::EDetectorID::kMvd == DetID) {
      auto statNr  = pHit->GetStationNr();
      auto iCentrX = pHit->GetIndexCentralX();
      auto iCentrY = pHit->GetIndexCentralY();
      auto iClust  = pHit->GetClusterIndex();
      auto flag    = pHit->GetFlag();
      new ((*fpBrHits)[fHitCounter]) CbmMvdHit(statNr, pos, dpos, iCentrX, iCentrY, iClust, flag);
    }
    else {
      auto address = pHit->GetAddress();
      auto time    = pHit->GetTime();
      auto dtime   = pHit->GetTimeError();
      if constexpr (ca::EDetectorID::kSts == DetID) {
        auto dxy     = pHit->GetDxy();
        auto iClustF = pHit->GetFrontClusterId();
        auto iClustB = pHit->GetBackClusterId();
        auto du      = pHit->GetDu();
        auto dv      = pHit->GetDv();
        new ((*fpBrHits)[fHitCounter]) CbmStsHit(address, pos, dpos, dxy, iClustF, iClustB, time, dtime, du, dv);
      }
      else if constexpr (ca::EDetectorID::kMuch == DetID) {
        auto dxy     = pHit->GetDxy();
        auto refId   = pHit->GetRefId();
        auto planeId = pHit->GetPlaneId();
        LOG(info) << "MUCH: " << pHit->GetRefId() << ' ' << pHit->GetPlaneId();
        new ((*fpBrHits)[fHitCounter]) CbmMuchPixelHit(address, pos, dpos, dxy, refId, planeId, time, dtime);
      }
      else if constexpr (ca::EDetectorID::kTrd == DetID) {
        auto dxy   = pHit->GetDxy();
        auto refId = pHit->GetRefId();
        auto eLoss = pHit->GetELoss();
        new ((*fpBrHits)[fHitCounter]) CbmTrdHit(address, pos, dpos, dxy, refId, eLoss, time, dtime);
      }
      else if constexpr (ca::EDetectorID::kTof == DetID) {
        auto refId   = -1;  // pHit->GetRefId(); // -> deprecated
        auto flag    = pHit->GetFlag();
        auto channel = pHit->GetCh();
        new ((*fpBrHits)[fHitCounter]) CbmTofHit(address, pos, dpos, refId, time, dtime, flag, channel);
      }
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  std::optional<CbmLink> IdealHitProducerDet<DetID>::GetMatchedPointLink(int iH)
  {
    const auto* pHitMatch = dynamic_cast<CbmMatch*>(fpBrHitMatchesTmp->At(iH));
    assert(pHitMatch);
    if (pHitMatch->GetNofLinks() > 0) {
      const auto& link = pHitMatch->GetMatchedLink();
      if (link.GetIndex() > -1) {
        return std::make_optional(link);
      }
    }
    return std::nullopt;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  void IdealHitProducerDet<DetID>::SmearValue(double& value, double sigma, int opt)
  {
    switch (opt) {
      case 0: value += fRandom.BoundedGaus<double>(0., sigma, kHitWSize); break;
      case 1: value += fRandom.Uniform<double>(0., sigma); break;
      default: LOG(fatal) << "IdealHitProducerDet::SmearValue: illegal option: opt = " << opt;
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  void IdealHitProducerDet<DetID>::Exec(Option_t*)
  {
    // ------ Check, if there are any requirement to run the routine for this detector
    if (!fbRunTheRoutine) {
      return;
    }

    // ------ Copy hit and match branches to the temporary array and clear the main arrays
    if (fpBrHits) {
      fpBrHitsTmp = static_cast<TClonesArray*>(fpBrHits->Clone("tmpHit"));
      fpBrHits->Delete();
    }
    if (fpBrHitMatches) {
      fpBrHitMatchesTmp = static_cast<TClonesArray*>(fpBrHitMatches->Clone("tmpHitMatch"));
      fpBrHitMatches->Delete();
    }

    assert(fpBrHits);
    assert(fpBrHitMatches);
    assert(fpBrHitsTmp);
    assert(fpBrHitMatchesTmp);

    std::vector<double> vHitMcEventTime;

    // ------ Fill main hit array
    fHitCounter  = 0;
    int iCluster = 0;  ///< Current cluster index
    for (int iH = 0; iH < fpBrHitsTmp->GetEntriesFast(); ++iH) {
      auto* pHit = static_cast<Hit_t*>(fpBrHitsTmp->At(iH));
      double tMC = pHit->GetTime();

      // Get setting
      int iSt = fpDetInterface->GetTrackingStationIndex(pHit);
      if (iSt < 0) {
        continue;
      }
      const auto& setting = fvStationPars[iSt];

      // Skip hits from stations, where new hits are to be created from points
      if (setting.fMode == 2) {
        continue;
      }

      else {
        if (5 == CbmTofAddress::GetSmType(pHit->GetAddress())) {
          LOG(info) << "TOF is TZERO!";
        }
      }

      // ------ Adjust hit to matched MC point
      if (setting.fMode == 1) {
        // Access matched MC point
        auto oLinkToPoint = GetMatchedPointLink(iH);
        if (oLinkToPoint == std::nullopt) {
          continue;
        }  // Hit had no links, so it's fake
        auto& link   = oLinkToPoint.value();
        auto* pPoint = static_cast<Point_t*>(fpBrPoints->Get(link));

        // Get point position and time
        auto posIn  = TVector3{};
        auto posOut = TVector3{};

        if constexpr (ca::EDetectorID::kTof == DetID) {
          pPoint->Position(posIn);
          pPoint->Position(posOut);
        }
        else {
          pPoint->Position(posIn);
          pPoint->PositionOut(posOut);
        }
        auto pos = 0.5 * (posIn + posOut);
        double x = pos.X();
        double y = pos.Y();
        double z = pos.Z();
        double t = pPoint->GetTime() + fpMCEventList->GetEventTime(link);
        tMC      = fpMCEventList->GetEventTime(link);

        if (setting.fbSmear) {
          double dx2 = pHit->GetDx() * pHit->GetDx();
          double dxy = pHit->GetDxy();
          double dy2 = pHit->GetDy() * pHit->GetDy();

          CaUvConverter conv(setting.fPhiU, dx2, dxy, dy2);

          auto [u, v]          = conv.ConvertXYtoUV(x, y);
          auto [du2, duv, dv2] = conv.ConvertCovMatrixXYtoUV(dx2, dxy, dy2);

          double dt = pHit->GetTimeError();
          this->SmearValue(u, sqrt(fabs(du2)), setting.fPdfU);
          this->SmearValue(v, sqrt(fabs(dv2)), setting.fPdfV);
          this->SmearValue(t, dt, setting.fPdfT);

          std::tie(x, y) = conv.ConvertUVtoXY(u, v);
        }
        pHit->SetX(x);
        pHit->SetY(y);
        pHit->SetZ(z);
        pHit->SetTime(t);
      }  // IF setting.fMode == 1

      // Check out the index of cluster
      if constexpr (ca::EDetectorID::kSts == DetID) {
        iCluster = std::max(pHit->GetFrontClusterId(), iCluster);
        iCluster = std::max(pHit->GetBackClusterId(), iCluster);
      }

      PushBackHit(pHit);
      vHitMcEventTime.push_back(tMC);
      auto* pHitMatchNew = new ((*fpBrHitMatches)[fHitCounter]) CbmMatch();
      pHitMatchNew->AddLinks(*static_cast<CbmMatch*>(fpBrHitMatchesTmp->At(iH)));
      ++fHitCounter;
    }

    // ------ Create hits from points
    int nEvents = fpMCEventList->GetNofEvents();
    for (int iE = 0; iE < nEvents; ++iE) {
      int fileId  = fpMCEventList->GetFileIdByIndex(iE);
      int eventId = fpMCEventList->GetEventIdByIndex(iE);
      int nPoints = fpBrPoints->Size(fileId, eventId);

      // TODO: Write point selector for TOF
      // NOTE: does not implemented for MVD
      this->FillPointIsLegit(iE);
      for (int iP = 0; iP < nPoints; ++iP) {
        if (!fvbPointIsLegit[iP]) {
          continue;
        }
        auto* pPoint = static_cast<Point_t*>(fpBrPoints->Get(fileId, eventId, iP));
        int iSt      = fpDetInterface->GetTrackingStationIndex(pPoint->GetDetectorID());
        if (iSt < 0) {
          continue;
        }

        const auto& setting = fvStationPars[iSt];
        if (setting.fMode != 2) {
          continue;
        }
        double du = setting.fDu;
        double dv = setting.fDv;
        double dt = setting.fDt;
        double dz = 0.;

        CaUvConverter conv(setting.fPhiU, setting.fPhiV);
        auto [dx2, dxy, dy2] = conv.ConvertCovMatrixUVtoXY(du * du, 0., dv * dv);

        // Get point position and time
        auto posIn  = TVector3{};
        auto posOut = TVector3{};

        if constexpr (ca::EDetectorID::kTof == DetID) {
          pPoint->Position(posIn);
          pPoint->Position(posOut);
        }
        else {
          pPoint->Position(posIn);
          pPoint->PositionOut(posOut);
        }
        auto pos = 0.5 * (posIn + posOut);
        double x = pos.X();
        double y = pos.Y();
        double z = pos.Z();
        double t = pPoint->GetTime() + fpMCEventList->GetEventTime(eventId, fileId);

        double tMC = fpMCEventList->GetEventTime(eventId, fileId);

        if (setting.fbSmear) {  // TODO: Provide more realistic profiles for TRD
          auto [u, v] = conv.ConvertXYtoUV(x, y);
          this->SmearValue(u, du, setting.fPdfU);
          this->SmearValue(v, dv, setting.fPdfV);
          this->SmearValue(t, dt, setting.fPdfT);
          std::tie(x, y) = conv.ConvertUVtoXY(u, v);
        }
        pos.SetX(x);
        pos.SetY(y);
        pos.SetZ(z);
        auto dpos = TVector3{std::sqrt(dx2), std::sqrt(dy2), dz};

        // Push back a new artificial hit
        if constexpr (ca::EDetectorID::kMvd == DetID) {
          new ((*fpBrHits)[fHitCounter]) CbmMvdHit();
        }
        else {
          auto address = pPoint->GetDetectorID();
          if constexpr (ca::EDetectorID::kSts == DetID) {
            int32_t iClustF = iCluster;
            int32_t iClustB = iCluster + 1;
            new ((*fpBrHits)[fHitCounter]) CbmStsHit(address, pos, dpos, dxy, iClustF, iClustB, t, dt, du, dv);
            iCluster += 2;
          }
          else if constexpr (ca::EDetectorID::kMuch == DetID) {
            // TODO: What is planeID in MuCh?
            int32_t refId   = -1;
            int32_t planeId = -1;
            new ((*fpBrHits)[fHitCounter]) CbmMuchPixelHit(address, pos, dpos, dxy, refId, planeId, t, dt);
          }
          else if constexpr (ca::EDetectorID::kTrd == DetID) {
            int32_t refId = -1;
            auto eLoss    = pPoint->GetEnergyLoss();
            new ((*fpBrHits)[fHitCounter]) CbmTrdHit(address, pos, dpos, dxy, refId, eLoss, t, dt);
          }
          else if constexpr (ca::EDetectorID::kTof == DetID) {
            int32_t refId   = 0;
            int32_t flag    = 0;
            int32_t channel = 0;
            new ((*fpBrHits)[fHitCounter]) CbmTofHit(address, pos, dpos, refId, t, dt, flag, channel);
          }
        }

        // Update hit match
        auto* pHitMatchNew = new ((*fpBrHitMatches)[fHitCounter]) CbmMatch();
        pHitMatchNew->AddLink(1., iP, eventId, fileId);
        vHitMcEventTime.push_back(tMC);
        ++fHitCounter;
        assert(fpBrHitMatches->GetEntriesFast() == fHitCounter);
      }  // iP
    }

    // ------ Clear temporary hits array
    fpBrHitsTmp->Delete();
    delete fpBrHitsTmp;
    fpBrHitsTmp = nullptr;

    fpBrHitMatchesTmp->Delete();
    delete fpBrHitMatchesTmp;
    fpBrHitMatchesTmp = nullptr;

    // --- Set new hit data to the reconstructed events
    if (fpRecoEvents) {
      for (Int_t iEvent = 0; iEvent < fpRecoEvents->GetEntriesFast(); iEvent++) {
        CbmEvent* event = static_cast<CbmEvent*>(fpRecoEvents->At(iEvent));
        event->ClearData(fHitDataType);
        double tStart = event->GetStartTime();
        double tEnd   = event->GetEndTime();
        for (UInt_t iH = 0; iH < vHitMcEventTime.size(); iH++) {
          double t = vHitMcEventTime[iH];
          if ((t >= tStart && t <= tEnd) || tStart < 0 || tEnd < 0) {
            event->AddData(fHitDataType, iH);
          }
        }
      }
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<ca::EDetectorID DetID>
  void IdealHitProducerDet<DetID>::FillPointIsLegit(int iEvGlob)
  {
    int iFile   = fpMCEventList->GetFileIdByIndex(iEvGlob);
    int iEvent  = fpMCEventList->GetEventIdByIndex(iEvGlob);
    int nPoints = fpBrPoints->Size(iFile, iEvent);

    fvbPointIsLegit.clear();
    fvbPointIsLegit.resize(nPoints);
    if constexpr (ca::EDetectorID::kTof == DetID) {
      constexpr int kNofBitsRpcAddress = 11;
      std::map<std::pair<int, int>, int> mMatchedPointId;  // map (iTr, addr) -> is matched
      for (int iH = 0; iH < fpBrHitMatches->GetEntriesFast(); ++iH) {
        auto* pHitMatch = dynamic_cast<CbmMatch*>(fpBrHitMatches->At(iH));
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
          auto* pExtPoint = static_cast<CbmTofPoint*>(fpBrPoints->Get(link));
          int trkId       = pExtPoint->GetTrackID();
          int rpcAddr     = pExtPoint->GetDetectorID() << kNofBitsRpcAddress;  // FIXME:
          auto key        = std::make_pair(trkId, rpcAddr);
          auto prev       = mMatchedPointId.find(key);
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

      // In TOF each RPC contains up to 10 points, and one have to select only
      // one of them
      {
        int iPointSelected = -1;
        int iTmcCurr       = -1;
        int rpcAddrCurr    = -1;
        bool bTrkHasHits   = false;
        double zDist       = std::numeric_limits<double>::max();
        double zCell       = std::numeric_limits<double>::signaling_NaN();
        for (int iP = 0; iP < nPoints; ++iP) {
          auto* pExtPoint = dynamic_cast<CbmTofPoint*>(fpBrPoints->Get(iFile, iEvent, iP));
          LOG_IF(fatal, !pExtPoint) << "NO POINT: TOF: file, event, index = " << iFile << ' ' << iEvent << ' ' << iP;

          int iTmc    = pExtPoint->GetTrackID();
          int rpcAddr = pExtPoint->GetDetectorID() << kNofBitsRpcAddress;
          // New point
          if (rpcAddrCurr != rpcAddr || iTmcCurr != iTmc) {  // The new interaction of the MC track with the TOF RPC
            if (iPointSelected != -1) {
              fvbPointIsLegit[iPointSelected] = true;
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
              zCell          = fpDetInterface->GetZrefModule(pExtPoint->GetDetectorID());
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
          fvbPointIsLegit[iPointSelected] = true;
        }
      }
    }
    else {
      // In MVD, STS, MuCh and TRD every MC point is legit
      std::fill(fvbPointIsLegit.begin(), fvbPointIsLegit.end(), true);
    }
  }

}  // namespace cbm::ca
