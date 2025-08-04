/* Copyright (C) 2006-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov [committer], Valentina Akishina, Igor Kulakov, Maksym Zyzak, Sergei Zharko */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction
 *
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de
 *
 *====================================================================
 *
 *  L1 main program
 *
 *====================================================================
 */

#include "CbmL1.h"

#include "CbmKfTarget.h"
#include "CbmMCDataManager.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmSetup.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdTrackingInterface.h"
#include "KfDefs.h"

#include <boost/filesystem.hpp>
// TODO: include of CbmSetup.h creates problems on Mac
// #include "CbmSetup.h"
#include "CaFramework.h"
#include "CaHit.h"
#include "CaToolsDebugger.h"
#include "CaToolsField.h"
#include "CbmEvent.h"
#include "CbmKfTrackingSetupBuilder.h"
#include "CbmMCDataObject.h"
#include "CbmStsFindTracks.h"
#include "CbmStsHit.h"
#include "CbmTrackingDetectorInterfaceInit.h"
#include "FairField.h"
#include "FairRunAna.h"
#include "KfRootUtils.h"
#include "TGeoArb8.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TMatrixD.h"
#include "TNtuple.h"
#include "TProfile2D.h"
#include "TROOT.h"
#include "TRandom3.h"
#include "TSystem.h"
#include "TVector3.h"
#include "TVectorD.h"

#include <TFile.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <list>
#include <sstream>

using namespace cbm::algo;  // TODO: remove this line

using CaTrack = cbm::algo::ca::Track;
using cbm::algo::ca::Parameters;
using cbm::ca::MCModule;
using cbm::ca::TimeSliceReader;

ClassImp(CbmL1);

static ca::Framework gAlgo _fvecalignment;  // TODO: Change coupling logic between ca::Framework and CbmL1

CbmL1* CbmL1::fpInstance = nullptr;


// ---------------------------------------------------------------------------------------------------------------------
//
CbmL1::CbmL1() : CbmL1("L1") {}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmL1::CbmL1(const char* name, Int_t verbose, Int_t performance) : FairTask(name, verbose), fPerformance(performance)
{
  if (!fpInstance) fpInstance = this;

  switch (fSTAPDataMode) {
    case 1: LOG(info) << "CbmL1: input data will be written for a standalone usage"; break;
    case 2: LOG(info) << "CbmL1: input data will be read from external files"; break;
    default: LOG(info) << "CbmL1: tracking will be run without external data R/W"; break;
  }

  fpIODataManager = std::make_shared<ca::DataManager>();

  this->DefineSTAPNames("");

  if (!CbmTrackingDetectorInterfaceInit::Instance()) {
    LOG(fatal) << "CbmL1: CbmTrackingDetectorInterfaceInit instance was not found. Please, add it as a task to your "
                  "reco macro right before the KF and L1 tasks:\n"
               << "\033[1;30mrun->AddTask(new CbmTrackingDetectorInterfaceInit());\033[0m";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmL1::~CbmL1()
{
  if (fpInstance == this) fpInstance = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::DisableTrackingStation(ca::EDetectorID detID, int iSt)
{
  if (ca::EDetectorID::END != detID) {
    fvmDisabledStationIDs[detID].insert(iSt);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmL1::ReInit()
{
  SetParContainers();
  return Init();
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus CbmL1::Init()
try {
  if (fVerbose > 1) {
    char y[20] = " [0;33;44m";         // yellow
    char Y[20] = " [1;33;44m";         // yellow bold
    char W[20] = " [1;37;44m";         // white bold
    char o[20] = " [0m\n";             // color off
    Y[0] = y[0] = W[0] = o[0] = 0x1B;  // escape character
    std::stringstream ss;

    ss << "\n\n";
    ss << "  " << W << "                                                                 " << o;
    ss << "  " << W << "  ===////======================================================  " << o;
    ss << "  " << W << "  =                                                           =  " << o;
    ss << "  " << W << "  =                   " << Y << "L1 on-line finder" << W << "                       =  " << o;
    ss << "  " << W << "  =                                                           =  " << o;
    ss << "  " << W << "  =  " << W << "Cellular Automaton 3.1 Vector" << y << " with " << W << "KF Quadro" << y
       << " technology" << W << "  =  " << o;
    ss << "  " << W << "  =                                                           =  " << o;
    ss << "  " << W << "  =  " << y << "Designed for CBM collaboration" << W << "                           =  " << o;
    ss << "  " << W << "  =  " << y << "All rights reserved" << W << "                                      =  " << o;
    ss << "  " << W << "  =                                                           =  " << o;
    ss << "  " << W << "  ========================================================////=  " << o;
    ss << "  " << W << "                                                                 " << o;
    ss << "\n\n";

    LOG(info) << ss.str();
  }

  cbm::ca::tools::SetOriginalCbmField();

  fHistoDir = gROOT->mkdir("L1");
  fHistoDir->mkdir("Input");
  fHistoDir->mkdir("Fit");

  fTableDir = gROOT->mkdir("L1Tables");

  // turn on reconstruction in sub-detectors

  auto* pSetupBuilder{cbm::kf::TrackingSetupBuilder::Instance()};
  bool bGeoMVD  = pSetupBuilder->IsInGeometry(ca::EDetectorID::kMvd);
  bool bGeoSTS  = pSetupBuilder->IsInGeometry(ca::EDetectorID::kSts);
  bool bGeoMUCH = pSetupBuilder->IsInGeometry(ca::EDetectorID::kMuch);
  bool bGeoTRD  = pSetupBuilder->IsInGeometry(ca::EDetectorID::kTrd);
  bool bGeoTOF  = pSetupBuilder->IsInGeometry(ca::EDetectorID::kTof);

  bool bUseMVD  = pSetupBuilder->HasHits(ca::EDetectorID::kMvd);
  bool bUseSTS  = pSetupBuilder->HasHits(ca::EDetectorID::kSts);
  bool bUseMUCH = pSetupBuilder->HasHits(ca::EDetectorID::kMuch);
  bool bUseTRD  = pSetupBuilder->HasHits(ca::EDetectorID::kTrd);
  bool bUseTOF  = pSetupBuilder->HasHits(ca::EDetectorID::kTof);

  LOG(info) << "!!!!!!! " << bUseSTS;

  // *****************************
  // **                         **
  // ** GEOMETRY INITIALIZATION **
  // **                         **
  // *****************************

  // Read parameters object from a binary file
  if (2 == fSTAPDataMode) {
    this->ReadSTAPParamObject();
  }
  // Parameters initialization, based on the FairRunAna chain
  else {
    // ********************************************
    // ** Base configuration file initialization **
    // ********************************************
    {
      // The parameters module of CA algorithm (L1Parameters) contains two sets of data fields: the geometry configura-
      // tion and the parameter configuration. The configuration both for geometry and parameters can be set using
      // either the accessor functions of the L1InitManager class, or reading the beforehand prepared L1Parameters
      // object. On top of that, the parameters configuration (including cuts, algorithm execution scenario etc.) can
      // be initialized from the YAML configuration files in two levels (base and user). The base level of the initia-
      // lization is required for CbmL1 mode, the user level of the initialization is optional.

      std::string mainConfig = std::string(gSystem->Getenv("VMCWORKDIR")) + "/macro/L1/configs/";
      switch (fTrackingMode) {
        case ca::TrackingMode::kSts: mainConfig += "ca_params_sts.yaml"; break;
        case ca::TrackingMode::kMcbm: mainConfig += "ca_params_mcbm.yaml"; break;
        case ca::TrackingMode::kGlobal: mainConfig += "ca_params_global.yaml"; break;
      }
      fInitManager.SetConfigMain(mainConfig);
    }

    fInitManager.SetDetectorNames(cbm::ca::kDetName);

    auto mvdInterface  = CbmMvdTrackingInterface::Instance();
    auto stsInterface  = CbmStsTrackingInterface::Instance();
    auto muchInterface = CbmMuchTrackingInterface::Instance();
    auto trdInterface  = CbmTrdTrackingInterface::Instance();
    auto tofInterface  = CbmTofTrackingInterface::Instance();

    int nMvdStationsGeom  = (bGeoMVD) ? mvdInterface->GetNtrackingStations() : 0;
    int nStsStationsGeom  = (bGeoSTS) ? stsInterface->GetNtrackingStations() : 0;
    int nMuchStationsGeom = (bGeoMUCH) ? muchInterface->GetNtrackingStations() : 0;
    int nTrdStationsGeom  = (bGeoTRD) ? trdInterface->GetNtrackingStations() : 0;
    int nTofStationsGeom  = (bGeoTOF) ? tofInterface->GetNtrackingStations() : 0;

    // **************************
    // ** Field initialization **
    // **************************

    if (FairRunAna::Instance()->GetField()) {
      fInitManager.SetFieldFunction([](const double(&inPos)[3], double(&outB)[3]) {
        FairRunAna::Instance()->GetField()->GetFieldValue(inPos, outB);
      });
    }
    else {
      fInitManager.SetFieldFunction([](const double(&)[3], double(&outB)[3]) {
        outB[0] = 0.;
        outB[1] = 0.;
        outB[2] = 0.;
      });
    }

    // ***************************
    // ** Target initialization **
    // ***************************

    {
      const auto& pTarget = cbm::kf::Target::Instance();
      fTargetX            = pTarget->GetX();
      fTargetY            = pTarget->GetY();
      fTargetZ            = pTarget->GetZ();
    }
    fInitManager.SetTargetPosition(fTargetX, fTargetY, fTargetZ);

    // *********************************
    // ** Target field initialization **
    // *********************************

    // FIXME: SZh 22.08.2023:
    //   Is there anyway to calculate a step between field slices?
    fInitManager.InitTargetField(/*zStep = */ 2.5 /*cm*/);  // Replace zStep -> sizeZfieldRegion = 2 * zStep (TODO)


    // **************************************
    // **                                  **
    // ** STATIONS GEOMETRY INITIALIZATION **
    // **                                  **
    // **************************************


    // ***************************************************
    // ** Active tracking detector subsystems selection **
    // ***************************************************


    // Explicitly disable MVD, if it is not required from the STSFindTracks task
    if (bUseMVD) {
      auto* pTrackFinderTask = dynamic_cast<CbmStsFindTracks*>(FairRunAna::Instance()->GetTask("STSFindTracks"));
      if (pTrackFinderTask) {
        bUseMVD = pTrackFinderTask->MvdUsage();
      }
    }


    // *************************************
    // ** Stations layout initialization  **
    // *************************************

    std::vector<ca::StationInitializer> vStations;
    vStations.reserve(100);
    bool bDisableTime = false;  /// TMP, move to parameters!

    // *** MVD stations info ***
    if (bGeoMVD) {
      for (int iSt = 0; iSt < nMvdStationsGeom; ++iSt) {
        auto stationInfo = ca::StationInitializer(ca::EDetectorID::kMvd, iSt);
        // TODO: SZh 15.08.2022: Replace station type with ca::EDetectorID
        stationInfo.SetStationType(1);  // MVD
        stationInfo.SetTimeInfo(!bDisableTime && mvdInterface->IsTimeInfoProvided(iSt));
        stationInfo.SetFieldStatus(fTrackingMode == ca::TrackingMode::kMcbm ? 0 : 1);
        stationInfo.SetZref(mvdInterface->GetZref(iSt));
        stationInfo.SetZmin(mvdInterface->GetZmin(iSt));
        stationInfo.SetZmax(mvdInterface->GetZmax(iSt));
        stationInfo.SetXmax(std::max(fabs(mvdInterface->GetXmax(iSt)), fabs(mvdInterface->GetXmin(iSt))));
        stationInfo.SetYmax(std::max(fabs(mvdInterface->GetYmax(iSt)), fabs(mvdInterface->GetYmin(iSt))));
        stationInfo.SetTrackingStatus(true);
        if (fvmDisabledStationIDs[ca::EDetectorID::kMvd].find(iSt)
            != fvmDisabledStationIDs[ca::EDetectorID::kMvd].cend()) {
          stationInfo.SetTrackingStatus(false);
        }
        if (!bUseMVD) {
          stationInfo.SetTrackingStatus(false);
        }
        fInitManager.AddStation(stationInfo);
        LOG(info) << "- MVD station " << iSt << " at z = " << stationInfo.GetZref() << " cm ";
      }
    }

    // *** STS stations info ***
    if (bGeoSTS) {
      for (int iSt = 0; iSt < nStsStationsGeom; ++iSt) {
        auto stationInfo = ca::StationInitializer(ca::EDetectorID::kSts, iSt);
        // TODO: SZh 15.08.2022: Replace station type with ca::EDetectorID
        stationInfo.SetStationType(0);  // STS
        stationInfo.SetTimeInfo(!bDisableTime && stsInterface->IsTimeInfoProvided(iSt));
        stationInfo.SetFieldStatus(ca::TrackingMode::kMcbm == fTrackingMode ? 0 : 1);
        stationInfo.SetZref(stsInterface->GetZref(iSt));
        stationInfo.SetZmin(stsInterface->GetZmin(iSt));
        stationInfo.SetZmax(stsInterface->GetZmax(iSt));
        stationInfo.SetXmax(std::max(fabs(stsInterface->GetXmax(iSt)), fabs(stsInterface->GetXmin(iSt))));
        stationInfo.SetYmax(std::max(fabs(stsInterface->GetYmax(iSt)), fabs(stsInterface->GetYmin(iSt))));

        stationInfo.SetTrackingStatus(true);
        if (fvmDisabledStationIDs[ca::EDetectorID::kSts].find(iSt)
            != fvmDisabledStationIDs[ca::EDetectorID::kSts].cend()) {
          stationInfo.SetTrackingStatus(false);
        }
        if (!bUseSTS) {
          stationInfo.SetTrackingStatus(false);
        }
        fInitManager.AddStation(stationInfo);
        LOG(info) << "- STS station " << iSt << " at z = " << stationInfo.GetZref() << " cm ";
      }
    }

    // *** MuCh stations info ***
    if (bGeoMUCH) {
      for (int iSt = 0; iSt < nMuchStationsGeom; ++iSt) {
        auto stationInfo = ca::StationInitializer(ca::EDetectorID::kMuch, iSt);
        // TODO: SZh 15.08.2022: Replace station type with ca::EDetectorID
        stationInfo.SetStationType(2);  // MuCh
        stationInfo.SetTimeInfo(!bDisableTime && muchInterface->IsTimeInfoProvided(iSt));
        stationInfo.SetFieldStatus(0);
        stationInfo.SetZref(muchInterface->GetZref(iSt));
        stationInfo.SetZmin(muchInterface->GetZmin(iSt));
        stationInfo.SetZmax(muchInterface->GetZmax(iSt));
        stationInfo.SetXmax(std::max(fabs(muchInterface->GetXmax(iSt)), fabs(muchInterface->GetXmin(iSt))));
        stationInfo.SetYmax(std::max(fabs(muchInterface->GetYmax(iSt)), fabs(muchInterface->GetYmin(iSt))));

        stationInfo.SetTrackingStatus(true);
        if (fvmDisabledStationIDs[ca::EDetectorID::kMuch].find(iSt)
            != fvmDisabledStationIDs[ca::EDetectorID::kMuch].cend()) {
          stationInfo.SetTrackingStatus(false);
        }
        if (!bUseMUCH) {
          stationInfo.SetTrackingStatus(false);
        }
        fInitManager.AddStation(stationInfo);
        LOG(info) << "- MuCh station " << iSt << " at z = " << stationInfo.GetZref() << " cm";
      }
    }

    // *** TRD stations info ***
    if (bGeoTRD) {
      for (int iSt = 0; iSt < nTrdStationsGeom; ++iSt) {
        auto stationInfo = ca::StationInitializer(ca::EDetectorID::kTrd, iSt);
        // TODO: SZh 15.08.2022: Replace station type with ca::EDetectorID
        stationInfo.SetStationType(3);
        stationInfo.SetTimeInfo(!bDisableTime && trdInterface->IsTimeInfoProvided(iSt));
        stationInfo.SetFieldStatus(0);
        stationInfo.SetZref(trdInterface->GetZref(iSt));
        stationInfo.SetZmin(trdInterface->GetZmin(iSt));
        stationInfo.SetZmax(trdInterface->GetZmax(iSt));
        stationInfo.SetXmax(std::max(fabs(trdInterface->GetXmax(iSt)), fabs(trdInterface->GetXmin(iSt))));
        stationInfo.SetYmax(std::max(fabs(trdInterface->GetYmax(iSt)), fabs(trdInterface->GetYmin(iSt))));

        if (ca::TrackingMode::kGlobal == fTrackingMode) {
          stationInfo.SetTimeInfo(false);
        }
        stationInfo.SetTrackingStatus(true);
        if (fvmDisabledStationIDs[ca::EDetectorID::kTrd].find(iSt)
            != fvmDisabledStationIDs[ca::EDetectorID::kTrd].cend()) {
          stationInfo.SetTrackingStatus(false);
        }
        if (!bUseTRD) {
          stationInfo.SetTrackingStatus(false);
        }
        fInitManager.AddStation(stationInfo);
        LOG(info) << "- TRD station " << iSt << " at z = " << stationInfo.GetZref() << " cm";
      }
    }

    // *** TOF stations info ***
    if (bGeoTOF) {
      for (int iSt = 0; iSt < nTofStationsGeom; ++iSt) {
        auto stationInfo = ca::StationInitializer(ca::EDetectorID::kTof, iSt);
        // TODO: SZh 15.08.2022: Replace station type with ca::EDetectorID
        stationInfo.SetStationType(4);
        stationInfo.SetTimeInfo(!bDisableTime && tofInterface->IsTimeInfoProvided(iSt));
        stationInfo.SetFieldStatus(0);
        stationInfo.SetZref(tofInterface->GetZref(iSt));
        stationInfo.SetZmin(tofInterface->GetZmin(iSt));
        stationInfo.SetZmax(tofInterface->GetZmax(iSt));
        stationInfo.SetXmax(std::max(fabs(tofInterface->GetXmax(iSt)), fabs(tofInterface->GetXmin(iSt))));
        stationInfo.SetYmax(std::max(fabs(tofInterface->GetYmax(iSt)), fabs(tofInterface->GetYmin(iSt))));

        stationInfo.SetTrackingStatus(true);
        if (fvmDisabledStationIDs[ca::EDetectorID::kTof].find(iSt)
            != fvmDisabledStationIDs[ca::EDetectorID::kTof].cend()) {
          stationInfo.SetTrackingStatus(false);
        }
        if (!bUseTOF) {
          stationInfo.SetTrackingStatus(false);
        }
        fInitManager.AddStation(stationInfo);
        LOG(info) << "- TOF station " << iSt << " at z = " << stationInfo.GetZref() << " cm";
      }
    }

    // Init station layout: sort stations in z-position and init maps of station local, geo and active indices
    fInitManager.InitStationLayout();
    fInitManager.ReadInputConfigs();

    // *************************
    // ** Initialize KF-setup **
    // *************************
    {
      auto trackerSetup  = pSetupBuilder->MakeSetup<float>(cbm::algo::kf::EFieldMode::Intrpl);
      TString geoTag     = "";
      if (auto* pSetup = CbmSetup::Instance()) {
        geoTag = pSetup->GetProvider()->GetSetup().GetTag();
      }
      if (geoTag.IsNull()) {
        geoTag = fSTAPDataPrefix;
      }
      TString outputFile = fSTAPDataDir + "/" + geoTag + "." + TString(kSTAPSetupSuffix.data());
      cbm::algo::kf::SetupBuilder::Store(trackerSetup, outputFile.Data());
      fInitManager.SetGeometrySetup(trackerSetup);
    }

    // *******************************
    // ** Initialize search windows **
    // *******************************

    if (fsInputSearchWindowsFilename.size()) {
      fInitManager.DevSetIsParSearchWUsed();
      fInitManager.ReadSearchWindows(fsInputSearchWindowsFilename);
    }
    else {
      fInitManager.DevSetIsParSearchWUsed(false);
    }

    // Form parameters container
    if (!fInitManager.FormParametersContainer()) {
      return kFATAL;
    }

    bUseMVD  = fInitManager.IsActive(ca::EDetectorID::kMvd);
    bUseSTS  = fInitManager.IsActive(ca::EDetectorID::kSts);
    bUseMUCH = fInitManager.IsActive(ca::EDetectorID::kMuch);
    bUseTRD  = fInitManager.IsActive(ca::EDetectorID::kTrd);
    bUseTOF  = fInitManager.IsActive(ca::EDetectorID::kTof);

    // Write parameters object to file if needed
    if (1 == fSTAPDataMode || 4 == fSTAPDataMode) {
      this->WriteSTAPParamObject();
    }
  }

  if (fInitMode == EInitMode::Param) {
    auto parameters = fInitManager.TakeParameters();
    LOG(info) << '\n' << parameters.ToString(1);
    return kSUCCESS;
  }

  // Init L1 algo core

  // FIXME: SZh 22.08.2023:
  //   Re-organize the the relation between CbmL1 and ca::Framework. I believe, we don't need a global pointer to the ca::Framework
  //   instance.
  fpAlgo = &gAlgo;

  //
  // ** Send formed parameters object to ca::Framework instance **
  //
  std::shared_ptr<ca::Parameters<float>> pParameters{nullptr};
  {
    auto parameters = fInitManager.TakeParameters();
    pParameters     = std::make_shared<ca::Parameters<float>>(parameters);
    fpAlgo->ReceiveParameters(std::move(parameters));
  }
  fpAlgo->Init(fTrackingMode);


  // Initialize time-slice reader
  fpTSReader = std::make_unique<TimeSliceReader>();
  fpTSReader->SetDetector(ca::EDetectorID::kMvd, bUseMVD);
  fpTSReader->SetDetector(ca::EDetectorID::kSts, bUseSTS);
  fpTSReader->SetDetector(ca::EDetectorID::kMuch, bUseMUCH);
  fpTSReader->SetDetector(ca::EDetectorID::kTrd, bUseTRD);
  fpTSReader->SetDetector(ca::EDetectorID::kTof, bUseTOF);

  fpTSReader->RegisterParameters(pParameters);
  fpTSReader->RegisterHitIndexContainer(fvExternalHits);
  fpTSReader->RegisterIODataManager(fpIODataManager);
  fpTSReader->RegisterQaHitContainer(fvHitDebugInfo);
  if (!fpTSReader->InitRun()) {
    return kFATAL;
  }

  if (fPerformance) {
    fpMCModule = std::make_unique<MCModule>(fVerbose, fPerformance);
    fpMCModule->SetDetector(ca::EDetectorID::kMvd, bUseMVD);
    fpMCModule->SetDetector(ca::EDetectorID::kSts, bUseSTS);
    fpMCModule->SetDetector(ca::EDetectorID::kMuch, bUseMUCH);
    fpMCModule->SetDetector(ca::EDetectorID::kTrd, bUseTRD);
    fpMCModule->SetDetector(ca::EDetectorID::kTof, bUseTOF);

    fpMCModule->RegisterMCData(fMCData);
    fpMCModule->RegisterRecoTrackContainer(fvRecoTracks);
    fpMCModule->RegisterHitIndexContainer(fvExternalHits);
    fpMCModule->RegisterParameters(pParameters);
    fpMCModule->RegisterQaHitContainer(fvHitDebugInfo);
    fpMCModule->RegisterFirstHitIndexes(fpTSReader->GetHitFirstIndexDet());
    if (!fpMCModule->InitRun()) {
      return kFATAL;
    }
  }

  LOG(info) << pParameters->ToString(1);
  LOG(info) << "----- Numbers of stations active in tracking -----";
  LOG(info) << "  MVD:   " << pParameters->GetNstationsActive(ca::EDetectorID::kMvd);
  LOG(info) << "  STS:   " << pParameters->GetNstationsActive(ca::EDetectorID::kSts);
  LOG(info) << "  MuCh:  " << pParameters->GetNstationsActive(ca::EDetectorID::kMuch);
  LOG(info) << "  TRD:   " << pParameters->GetNstationsActive(ca::EDetectorID::kTrd);
  LOG(info) << "  TOF:   " << pParameters->GetNstationsActive(ca::EDetectorID::kTof);
  LOG(info) << "  Total: " << pParameters->GetNstationsActive();

  fNStations = pParameters->GetNstationsActive();

  // TODO: Probably move from CbmL1 to a more proper place (e.g. kf::Setup)
  {
    //std::stringstream msg;
    //msg << " ----- Material budget map monitoring: active setup -----\n";
    const auto& activeSetup = pParameters->GetActiveSetup();
    for (int iSt = 0; iSt < activeSetup.GetNofLayers(); ++iSt) {
      fMaterialMonitor.emplace_back(&(activeSetup.GetMaterial(iSt)), Form("Station %d", iSt));
      //msg << monitor.ToString() << '\n';
    }
    //msg << " --------------------------------------------------------";
    //LOG(info) << '\n' << msg.str();
  }

  DumpMaterialToFile("L1material.root");

  // Initialize monitor
  fMonitor.Reset();

  return kSUCCESS;
}
catch (const std::exception& err) {
  LOG(error) << "CbmL1: initialization failed. Reason: " << err.what();
  return kFATAL;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::Reconstruct(CbmEvent* event)
{
  ca::TrackingMonitorData monitorData;

  fpTSReader->ReadEvent(event);
  if (fPerformance) {
    fpMCModule->InitEvent(event);  // reads points and MC tracks
    fpMCModule->MatchHits();       // matches points with hits
  }
  fpAlgo->ReceiveInputData(fpIODataManager->TakeInputData());

  // Material monitoring: mark active areas
  {
    for (const auto& hit : fpAlgo->GetInputData().GetHits()) {
      fMaterialMonitor[hit.Station()].MarkActiveBin(hit.X(), hit.Y());
    }
  }
  //LOG(info) << "CHECK: hit ids = " << fvExternalHits.size() << ", hits = " << fpIODataManager->GetNofHits()
  //<< ", dbg hits = " << fvHitDebugInfo.size();

  fpAlgo->SetMonitorData(monitorData);

  if (nullptr != event) {
    LOG_IF(debug, fVerbose > 0) << "\n=======  Ca Track finder: process event " << event->GetNumber() << " ...";
  }
  else {
    LOG_IF(debug, fVerbose > 0) << "\n=======  Ca Track finder: process timeslice ...";
  }

  fpAlgo->FindTracks();
  //       IdealTrackFinder();
  fTrackingTime = fpAlgo->fCaRecoTime;  // TODO: remove (not used)

  LOG_IF(debug, fVerbose > 0) << "Ca Track Finder finished, found " << fpAlgo->fRecoTracks.size() << " tracks";

  // Update monitor data after the actual tracking
  monitorData = fpAlgo->GetMonitorData();

  // save reconstructed tracks

  fvRecoTracks.clear();
  fvRecoTracks.reserve(fpAlgo->fRecoTracks.size());

  int trackFirstHit = 0;

  // FIXME: Rewrite
  for (const auto& caTrk : fpAlgo->fRecoTracks) {
    CbmL1Track t;
    t.Set(caTrk.fParFirst);
    t.TLast.Set(caTrk.fParLast);
    t.Tpv.Set(caTrk.fParPV);
    t.Hits.clear();

    for (int i = 0; i < caTrk.fNofHits; i++) {
      int caHitId  = fpAlgo->fRecoHits[trackFirstHit + i];
      int cbmHitID = fpAlgo->GetInputData().GetHit(caHitId).Id();
      t.Hits.push_back(cbmHitID);
    }
    fvRecoTracks.push_back(t);
    trackFirstHit += caTrk.fNofHits;
    //fMonitor.IncrementCounter(EMonitorKey::kRecoHit, it->fNofHits);
  }

  //fMonitor.IncrementCounter(EMonitorKey::kRecoTrack, fvRecoTracks.size());
  LOG(debug) << "CA Track Finder: " << fpAlgo->fCaRecoTime << " s/sub-ts";

  // output performance
  if (fPerformance) {
    LOG_IF(info, fVerbose) << "Performance...";

    fpMCModule->MatchTracks();  // matches reco and MC tracks, fills the MC-truth fields of reco tracks

    //
    // tracker input performance is moved to external QA tasks.
    // InputPerformance() method is not optimised to run with the event builder
    // TODO: verify QA tasks and remove InputPerformance()
    // InputPerformance();
    //

    EfficienciesPerformance();
    HistoPerformance();
    TrackFitPerformance();
    if (fsMcTripletsOutputFilename.size()) {
      DumpMCTripletsToTree();
    }
    // TimeHist();
    ///    WriteSIMDKFData();
    LOG_IF(info, fVerbose > 1) << "Tracking performance... done";
  }
  LOG_IF(debug, fVerbose > 1) << "End of CA";

  ++fEventNo;
  fMonitor.AddMonitorData(monitorData);
}

// -----   Finish CbmStsFitPerformanceTask task   -----------------------------
void CbmL1::Finish()
{
  if (fPerformance) {
    // FieldApproxCheck();
    // FieldIntegralCheck();
    EfficienciesPerformance(kTRUE);
  }

  {
    // monitor the material
    std::stringstream msg;
    msg << '\n';
    msg << "\033[31;1m ***************************\033[0m\n";
    msg << "\033[31;1m **  CA Tracking monitor  **\033[0m\n";
    msg << "\033[31;1m ***************************\033[0m\n";

    // TODO: Probably move from CbmL1 to a more proper place (e.g. kf::Setup)
    {
      msg << '\n';
      msg << " ----- Material budget map monitoring: active setup -----\n";
      for (auto& monitor : fMaterialMonitor) {
        msg << monitor.ToString() << '\n';
      }
      msg << " --------------------------------------------------------\n";
    }

    // monitor of the reconstructed tracks
    msg << '\n' << fMonitor.ToString();
    LOG(info) << msg.str();
  }

  TDirectory* curr   = gDirectory;
  TFile* currentFile = gFile;


  // Open output file and write histograms
  boost::filesystem::path p = (FairRunAna::Instance()->GetUserOutputFileName()).Data();
  std::string dir           = p.parent_path().string();
  if (dir.empty()) dir = ".";
  {
    std::string histoOutName = dir + "/L1_histo_" + p.filename().string();
    LOG(info) << "\033[31;1mL1 performance histograms will be saved to: \033[0m" << histoOutName;
    TFile* outfile = new TFile(histoOutName.c_str(), "RECREATE");
    outfile->cd();
    writedir2current(fHistoDir);
    outfile->Close();
    outfile->Delete();
  }
  {
    std::string tablesOutName = dir + "/L1_perftable_" + p.filename().string();
    LOG(info) << "\033[31;1mL1 performance tables will be saved to: \033[0m" << tablesOutName;
    TFile* outfile = new TFile(tablesOutName.c_str(), "RECREATE");
    outfile->cd();
    writedir2current(fTableDir);
    outfile->Close();
    outfile->Delete();
  }

  if (fpMcTripletsOutFile) {
    fpMcTripletsOutFile->cd();
    fpMcTripletsTree->Write();
    fpMcTripletsOutFile->Close();
    fpMcTripletsOutFile->Delete();
  }

  gFile      = currentFile;
  gDirectory = curr;
  fpAlgo->Finish();
  cbm::ca::tools::Debugger::Instance().Write();
}


// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::writedir2current(TObject* obj)
{
  if (!obj->IsFolder())
    obj->Write();
  else {
    TDirectory* cur = gDirectory;
    TDirectory* sub = cur->mkdir(obj->GetName());
    sub->cd();
    TList* listSub = (dynamic_cast<TDirectory*>(obj))->GetList();
    TIter it(listSub);
    while (TObject* obj1 = it())
      writedir2current(obj1);
    cur->cd();
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::IdealTrackFinder()
{
  fpAlgo->fRecoTracks.clear();
  fpAlgo->fRecoHits.clear();

  for (auto& MC : fMCData.GetTrackContainer()) {
    if (!MC.IsReconstructable()) continue;
    if (!(MC.GetId() >= 0)) continue;

    if (MC.GetNofHits() < 4) continue;

    CaTrack algoTr;
    algoTr.fNofHits = 0;

    ca::Vector<int> hitIndices("CbmL1::hitIndices", fpAlgo->GetParameters().GetNstationsActive(), -1);

    for (unsigned int iH : MC.GetHitIndexes()) {
      const CbmL1HitDebugInfo& hit = fvHitDebugInfo[iH];
      const int iStation           = hit.GetStationId();
      if (iStation >= 0) hitIndices[iStation] = iH;
    }


    for (int iH = 0; iH < fpAlgo->GetParameters().GetNstationsActive(); iH++) {
      const int hitI = hitIndices[iH];
      if (hitI < 0) continue;

      // fpAlgo->fRecoHits.push_back(hitI);
      algoTr.fNofHits++;
    }


    if (algoTr.fNofHits < 3) continue;

    for (int iH = 0; iH < fpAlgo->GetParameters().GetNstationsActive(); iH++) {
      const int hitI = hitIndices[iH];
      if (hitI < 0) continue;
      fpAlgo->fRecoHits.push_back(hitI);
    }

    algoTr.fParFirst.X()    = MC.GetStartX();
    algoTr.fParFirst.Y()    = MC.GetStartY();
    algoTr.fParFirst.Z()    = MC.GetStartZ();
    algoTr.fParFirst.Tx()   = MC.GetTx();
    algoTr.fParFirst.Ty()   = MC.GetTy();
    algoTr.fParFirst.Qp()   = MC.GetCharge() / MC.GetP();
    algoTr.fParFirst.Time() = MC.GetStartT();
    fpAlgo->fRecoTracks.push_back(algoTr);
  }
}  // void CbmL1::IdealTrackFinder()


/// -----   STandAlone Package service-functions  -----------------------------

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::DefineSTAPNames(const char* dirName)
{
  // FIXME: SZh 01.03.2023: Clean STAP names
  namespace bfs = boost::filesystem;

  if (fSTAPDataMode == 0) {
    return;
  }

  // Define file prefix (/path/to/data/setup.reco.root -> "setup.reco")
  bfs::path pathToRecoOutput = FairRunAna::Instance()->GetUserOutputFileName().Data();
  fSTAPDataPrefix            = pathToRecoOutput.filename().string();
  fSTAPDataPrefix.ReplaceAll(".root", "");
  fSTAPDataPrefix = fSTAPDataPrefix.Strip(TString::EStripType::kBoth, '.');

  TString sDirName = TString(dirName);
  if (sDirName.Length() == 0) {
    fSTAPDataDir = pathToRecoOutput.parent_path().string();
  }
  else if (bfs::exists(sDirName.Data()) && bfs::is_directory(sDirName.Data())) {
    fSTAPDataDir = sDirName;
  }
  else {
    fSTAPDataDir = ".";
  }

  // TODO: Clean-up the names and pathes
  if (fSTAPDataDir.Length() == 0) {
    fSTAPDataDir = ".";
  }

  LOG(info) << "CbmL1: STAP data root directory is \033[1;32m" << bfs::system_complete(fSTAPDataDir.Data())
            << "\033[0m";

  if (fSTAPDataMode == 4) {
    return;
  }

  // Directory for handling L1InputData objects
  TString sInputDataDir = fSTAPDataDir + "/" + fSTAPDataPrefix + "_cainputdata";
  if (!bfs::exists(sInputDataDir.Data())) {
    LOG(warn) << "CbmL1: directory for tracking input data does not exist. It will be created";
    bfs::create_directories(sInputDataDir.Data());
  }
  LOG(info) << "CbmL1: STAP tracking input jobs directory is \033[1;32m" << bfs::system_complete(sInputDataDir.Data())
            << "\033[0m";
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::WriteSTAPParamObject()
{
  TString filename = fSTAPParamFile.IsNull()
                       ? fSTAPDataDir + "/" + fSTAPDataPrefix + "." + TString(kSTAPParamSuffix.data())
                       : fSTAPParamFile;
  fInitManager.WriteParametersObject(filename.Data());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::WriteSTAPAlgoInputData(int iJob)  // must be called after ReadEvent
{
  TString filename =
    fSTAPDataDir + "/" + fSTAPDataPrefix + "_cainputdata/" + +TString::Format(kSTAPAlgoIDataSuffix.data(), iJob);

  // Write file
  fpIODataManager->WriteInputData(filename.Data());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::WriteSTAPPerfInputData()  // must be called after ReadEvent
{
  LOG(fatal) << "CbmL1: Running in standalone mode is not available at the moment. It will be updated soon...";
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::ReadSTAPParamObject()
{
  TString filename = fSTAPDataDir + "/" + fSTAPDataPrefix + "." + TString(kSTAPParamSuffix.data());
  fInitManager.ReadParametersObject(filename.Data());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::ReadSTAPAlgoInputData(int iJob)
{
  TString filename = fSTAPDataDir + "/" + TString(kSTAPAlgoIDataDir) + "/" + fSTAPDataPrefix + "."
                     + TString::Format(kSTAPAlgoIDataSuffix.data(), iJob);

  // Read file
  fpIODataManager->ReadInputData(filename.Data());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::ReadSTAPPerfInputData()
{
  LOG(fatal) << "CbmL1: Running in standalone mode is not available at the moment. It will be updated soon...";
}

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::DumpMaterialToFile(TString fileName)
{
  auto* currentFile = gFile;
  TFile f           = TFile{fileName, "RECREATE"};
  f.cd();
  // TODO: (!) replace CbmL1::Instance()->fpAlgo->GetParameters() with the cbm::ca::ParametersHandler::Instance()->Get()
  //           everywhere outside cbm::algo
  const auto& activeTrackingSetup = CbmL1::Instance()->fpAlgo->GetParameters().GetActiveSetup();
  for (int iSt = 0; iSt < activeTrackingSetup.GetNofLayers(); ++iSt) {
    const auto& material = activeTrackingSetup.GetMaterial(iSt);
    TString name         = Form("tracking_station%d", iSt);
    TString title        = Form("Tracking station %d: Rad. thickness in %%. Z region [%.2f, %.2f] cm.", iSt,
                         material.GetZmin(), material.GetZmax());
    if (fMatBudgetParallelProjection) {
      title += " Horizontal projection.";
    }
    else {
      title += " Radial projection.";
    }
    auto* h = ::kf::tools::RootUtils::ToHistogram(material, name, title);
    h->SetStats(kFALSE);
    h->SetDirectory(&f);
  }
  f.Write();
  currentFile->cd();
}
