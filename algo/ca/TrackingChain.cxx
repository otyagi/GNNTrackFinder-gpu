/* Copyright (C) 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingChain.cxx
/// \date   14.09.2023
/// \brief  A chain class to execute CA tracking algorithm in online reconstruction (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "TrackingChain.h"

#include "CaDefs.h"
#include "CaHit.h"
#include "CaInitManager.h"
#include "CaParameters.h"
#include "KfSetupBuilder.h"
#include "ParFiles.h"
#include "compat/OpenMP.h"
#include "yaml/Yaml.h"

#include <boost/archive/binary_oarchive.hpp>

#include <fstream>
#include <set>
#include <unordered_map>

#include <fmt/format.h>
#include <xpu/host.h>

using cbm::algo::TrackingChain;
using cbm::algo::ca::EDetectorID;
using cbm::algo::ca::Framework;
using cbm::algo::ca::HitTypes_t;
using cbm::algo::ca::InitManager;
using cbm::algo::ca::Parameters;
using cbm::algo::ca::Qa;
using cbm::algo::ca::Track;
using cbm::algo::ca::constants::clrs::CL;   // clear text
using cbm::algo::ca::constants::clrs::GNb;  // grin bald text

// ---------------------------------------------------------------------------------------------------------------------
//
TrackingChain::TrackingChain(ECbmRecoMode recoMode, const std::unique_ptr<cbm::algo::qa::Manager>& qaManager,
                             std::string_view name)
  : fQa(Qa(qaManager, name))
  , fRecoMode(recoMode)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingChain::Init()
{
  if (fpSetup.get() == nullptr) {
    throw std::runtime_error("Tracking Chain: TrackingSetup object was not registered");
  }


  // ------ Read tracking chain parameters from the config
  ParFiles parFiles(Opts().RunId());
  fConfig = yaml::ReadFromFile<TrackingChainConfig>(Opts().ParamsDir() / parFiles.ca.mainConfig);

  // ------ Read parameters from binary
  auto geomCfgFile  = (Opts().ParamsDir() / fConfig.fsGeomConfig).string();
  auto setupCfgFile = (Opts().ParamsDir() / fConfig.fsSetupFilename).string();
  auto mainCfgFile  = (Opts().ParamsDir() / fConfig.fsMainConfig).string();
  auto userCfgFile  = fConfig.fsUserConfig;

  L_(info) << "Tracking Chain: reading geometry from CA parameters file " << GNb << geomCfgFile << CL << '\n';
  L_(info) << "Tracking Chain: reading geometry setup file " << GNb << setupCfgFile << CL << '\n';
  L_(info) << "Tracking Chain: reading parameters from CA main config " << GNb << mainCfgFile << CL << '\n';

  //* InitManager instantiation
  auto manager = InitManager{};
  manager.SetDetectorNames(ca::kDetName);
  manager.SetConfigMain(mainCfgFile);
  if (!userCfgFile.empty()) {
    L_(info) << "Tracking Chain: applying user configuration from " << GNb << userCfgFile << CL << '\n';
    manager.SetConfigUser(userCfgFile);
  }

  //* Read parameters object from the geomCfgFile to intialize tracking stations
  {
    manager.ReadParametersObject(geomCfgFile);  // geometry setup
    auto paramIn = manager.TakeParameters();
    manager.ClearSetupInfo();

    //* Read setup
    auto geoSetup = kf::SetupBuilder::Load<ca::fvec>(setupCfgFile);
    kf::Target<double> target(geoSetup.GetTarget());

    //* Initialize tracking parameters
    manager.SetFieldFunction([](const double(&)[3], double(&outB)[3]) {
      outB[0] = 0.;
      outB[1] = 0.;
      outB[2] = 0.;
    });
    manager.SetTargetPosition(target.GetX(), target.GetY(), target.GetZ());
    manager.InitTargetField(2.5 /*cm*/);
    manager.AddStations(paramIn);  // Initialize stations
    manager.InitStationLayout();
    manager.ReadInputConfigs();
    manager.SetGeometrySetup(geoSetup);
    manager.DevSetIsParSearchWUsed(false);
    if (!manager.FormParametersContainer()) {
      throw std::runtime_error("Initialization of CA parameters failed");
    }
  }
  auto parameters = manager.TakeParameters();

  L_(info) << "Tracking Chain: parameters object: \n" << parameters.ToString(1) << '\n';

  // ------ Used detector subsystem flags
  fbDetUsed.fill(false);
  fbDetUsed[EDetectorID::kSts] = Opts().Has(fles::Subsystem::STS) && parameters.IsActive(EDetectorID::kSts);
  fbDetUsed[EDetectorID::kTof] = Opts().Has(fles::Subsystem::TOF) && parameters.IsActive(EDetectorID::kTof);
  fbDetUsed[EDetectorID::kTrd] = Opts().Has(fles::Subsystem::TRD) && parameters.IsActive(EDetectorID::kTrd);

  // ------ Initialize CA framework
  fCaMonitor.Reset();
  if (ECbmRecoMode::Timeslice == fRecoMode) {
    fCaFramework.SetNofThreads(Opts().NumOMPThreads() == std::nullopt ? openmp::GetMaxThreads()
                                                                      : *(Opts().NumOMPThreads()));
  }
  else {
    fCaFramework.SetNofThreads(1);
  }
  fCaFramework.ReceiveParameters(std::move(parameters));
  fCaFramework.Init(ca::TrackingMode::kMcbm);

  // ------ Initialize QA modules
  if (fQa.IsActive()) {
    fQa.RegisterParameters(&fCaFramework.GetParameters());
    fQa.Init();
  }
  L_(info) << "TRACKING QA: " << fQa.IsActive();
}

// ---------------------------------------------------------------------------------------------------------------------
//
TrackingChain::Output_t TrackingChain::Run(Input_t recoResults)
{
  xpu::scoped_timer t_("CA");  // TODO: pass timings to monitoring for throughput?
  fCaMonitorData.Reset();
  fCaMonitorData.StartTimer(ca::ETimer::TrackingChain);

  // ----- Init input data ---------------------------------------------------------------------------------------------
  fCaMonitorData.StartTimer(ca::ETimer::PrepareInputData);
  this->PrepareInput(recoResults);
  fCaMonitorData.StopTimer(ca::ETimer::PrepareInputData);

  // ----- Run reconstruction ------------------------------------------------------------------------------------------
  fCaFramework.SetMonitorData(fCaMonitorData);
  fCaFramework.FindTracks();
  fCaMonitorData = fCaFramework.GetMonitorData();

  // ----- Init output data --------------------------------------------------------------------------------------------
  return PrepareOutput();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingChain::Finalize()
{
  L_(info) << fCaMonitor.ToString();
  if (fConfig.fbStoreMonitor) {
    auto fileName = "./" + fConfig.fsMoniOutName;
    std::ofstream ofs(fileName);
    boost::archive::binary_oarchive oa(ofs);
    oa << fCaMonitor;
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void TrackingChain::PrepareInput(Input_t recoResults)
{
  fNofHitKeys  = 0;
  int nHitsTot = recoResults.stsHits.NElements() + recoResults.tofHits.NElements() + recoResults.trdHits.NElements();
  L_(debug) << "Tracking chain: input has " << nHitsTot << " hits";
  fCaDataManager.ResetInputData(nHitsTot);
  faHitExternalIndices.clear();
  faHitExternalIndices.reserve(nHitsTot);
  if (fbDetUsed[EDetectorID::kSts]) {
    fCaMonitorData.StartTimer(ca::ETimer::PrepareStsHits);
    ReadHits<EDetectorID::kSts>(recoResults.stsHits);
    fCaMonitorData.StopTimer(ca::ETimer::PrepareStsHits);
  }
  if (fbDetUsed[EDetectorID::kTrd]) {
    fCaMonitorData.StartTimer(ca::ETimer::PrepareTrdHits);
    ReadHits<EDetectorID::kTrd>(recoResults.trdHits);
    fCaMonitorData.StopTimer(ca::ETimer::PrepareTrdHits);
  }
  if (fbDetUsed[EDetectorID::kTof]) {
    fCaMonitorData.StartTimer(ca::ETimer::PrepareTofHits);
    ReadHits<EDetectorID::kTof>(recoResults.tofHits);
    fCaMonitorData.StopTimer(ca::ETimer::PrepareTofHits);
  }
  faHitExternalIndices.shrink_to_fit();
  fCaDataManager.SetNhitKeys(fNofHitKeys);
  L_(debug) << "Tracking chain: " << fCaDataManager.GetNofHits() << " hits will be passed to the ca::Framework";
  fCaMonitorData.StartTimer(ca::ETimer::InputDataTransmission);
  fCaFramework.ReceiveInputData(fCaDataManager.TakeInputData());
  fCaMonitorData.StopTimer(ca::ETimer::InputDataTransmission);
}

// ---------------------------------------------------------------------------------------------------------------------
//
TrackingChain::Output_t TrackingChain::PrepareOutput()
{
  Output_t output;
  output.tracks = std::move(fCaFramework.fRecoTracks);
  int nTracks   = output.tracks.size();

  output.stsHitIndices.reset(nTracks);
  output.tofHitIndices.reset(nTracks);
  output.trdHitIndices.reset(nTracks);

  int trackFirstHit = 0;
  for (int iTrk = 0; iTrk < nTracks; ++iTrk) {
    output.stsHitIndices[iTrk].clear();
    output.tofHitIndices[iTrk].clear();
    output.trdHitIndices[iTrk].clear();
    int nHits = output.tracks[iTrk].fNofHits;
    for (int iHit = 0; iHit < nHits; ++iHit) {
      int iHitInternal = fCaFramework.GetInputData().GetHit(fCaFramework.fRecoHits[trackFirstHit + iHit]).Id();
      const auto [detID, iPartition, iPartHit] = faHitExternalIndices[iHitInternal];
      switch (detID) {
        // FIXME: store a global hit index instead of (partition, hit)
        case ca::EDetectorID::kSts: output.stsHitIndices[iTrk].push_back(std::make_pair(iPartition, iPartHit)); break;
        case ca::EDetectorID::kTof: output.tofHitIndices[iTrk].push_back(std::make_pair(iPartition, iPartHit)); break;
        case ca::EDetectorID::kTrd: output.trdHitIndices[iTrk].push_back(std::make_pair(iPartition, iPartHit)); break;
        default: break;
      }
    }
    fCaMonitorData.IncrementCounter(ca::ECounter::RecoStsHit, output.stsHitIndices[iTrk].size());
    fCaMonitorData.IncrementCounter(ca::ECounter::RecoTofHit, output.tofHitIndices[iTrk].size());
    fCaMonitorData.IncrementCounter(ca::ECounter::RecoTrdHit, output.trdHitIndices[iTrk].size());

    trackFirstHit += nHits;
  }

  if (ECbmRecoMode::Timeslice == fRecoMode) {
    L_(info) << "TrackingChain: Timeslice contains " << fCaMonitorData.GetCounterValue(ca::ECounter::RecoTrack)
             << " tracks, with " << fCaMonitorData.GetCounterValue(ca::ECounter::RecoStsHit) << " sts hits, "
             << fCaMonitorData.GetCounterValue(ca::ECounter::RecoTofHit) << " tof hits, "
             << fCaMonitorData.GetCounterValue(ca::ECounter::RecoTrdHit) << " trd hits"
             << "; the FindTracks routine ran " << fCaMonitorData.GetTimer(ca::ETimer::FindTracks).GetTotal() << " s";
  }

  // QA
  if (fQa.IsActive()) {
    fCaMonitorData.StartTimer(ca::ETimer::Qa);
    fQa.RegisterInputData(&fCaFramework.GetInputData());
    fQa.RegisterTracks(&output.tracks);
    fQa.RegisterRecoHitIndices(&fCaFramework.fRecoHits);
    fQa.Exec();
    fCaMonitorData.StopTimer(ca::ETimer::Qa);
  }

  fCaMonitorData.StopTimer(ca::ETimer::TrackingChain);

  if constexpr (kDEBUG) {  // Monitor data per TS
    if (fCaFramework.GetNofThreads() == 1 && fCaFramework.GetNofThreads() == 10) {
      int tsIndex   = fCaMonitor.GetCounterValue(ca::ECounter::TrackingCall);
      auto fileName = fmt::format("./ca_monitor_nth_{}_ts{}.dat", fCaFramework.GetNofThreads(), tsIndex);
      std::ofstream ofs(fileName);
      boost::archive::binary_oarchive oa(ofs);
      ca::TrackingMonitor monitorPerTS;
      monitorPerTS.AddMonitorData(fCaMonitorData);
      oa << monitorPerTS;
    }
  }

  fCaMonitor.AddMonitorData(fCaMonitorData);
  output.monitorData = fCaMonitorData;

  return output;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<EDetectorID DetID>
void TrackingChain::ReadHits(PartitionedSpan<const ca::HitTypes_t::at<DetID>> hits)
{
  int nSt = fCaFramework.GetParameters().GetNstationsActive();

  using Hit_t           = ca::HitTypes_t::at<DetID>;
  constexpr bool IsMvd  = (DetID == EDetectorID::kMvd);
  constexpr bool IsSts  = (DetID == EDetectorID::kSts);
  constexpr bool IsMuch = (DetID == EDetectorID::kMuch);
  constexpr bool IsTrd  = (DetID == EDetectorID::kTrd);
  constexpr bool IsTof  = (DetID == EDetectorID::kTof);

  xpu::t_add_bytes(hits.NElements() * sizeof(Hit_t));  // Assumes call from Run, for existence of timer!

  int64_t dataStreamDet = static_cast<int64_t>(DetID) << 60;  // detector part of the data stream
  int64_t dataStream    = 0;
  for (size_t iPartition = 0; iPartition < hits.NPartitions(); ++iPartition, ++dataStream) {
    const auto& [vHits, extHitAddress] = hits.Partition(iPartition);
    // ---- Define data stream and station index
    //int64_t dataStream = dataStreamDet | extHitAddress;
    int iStLocal = fpSetup->GetTrackingStation<ca::ToFlesSubsystem<DetID>()>(extHitAddress);
    if (iStLocal < 0) {
      continue;  // Station is not used for tracking (e.g. TOF SMtype 5)
    }

    int iStActive = fCaFramework.GetParameters().GetStationIndexActive(iStLocal, DetID);

    //size_t iOffset = hits.Offsets()[iPartition];
    if (iStActive < 0) {
      continue;  // legit
    }
    if (iStActive >= nSt) {
      L_(error) << "TrackingChain: found hit with wrong active station index above the upper limit: " << iStActive
                << ", detector: " << ca::kDetName[DetID];
      continue;
    }
    double lastTime = -1e9;
    //double prevTime = -1;
    ca::HitKeyIndex_t firstHitKey = fNofHitKeys;
    for (size_t iPartHit = 0; iPartHit < vHits.size(); ++iPartHit) {
      const auto& hit = vHits[iPartHit];

      //if constexpr (IsTrd) {
      //  switch (extHitAddress) {
      //    case 0x5:
      //      if (!(fabs(hit.Z() - 116.77) < 10)) {
      //        L_(info) << "DBG! " << extHitAddress << ' ' << hit.Z();
      //      }
      //      break;
      //    case 0x15:
      //      if (!(fabs(hit.Z() - 163.8) < 10)) {
      //        L_(info) << "DBG! "  << extHitAddress << ' ' << hit.Z();
      //      }
      //      break;
      //    case 0x25:
      //      if (!(fabs(hit.Z() - 190.8) < 10)) {
      //        L_(info) << "DBG! "  << extHitAddress << ' ' << hit.Z();
      //      }
      //      break;
      //  }
      //}


      //int iHitExt     = iOffset + iPartHit;
      // ---- Fill ca::Hit
      fCaMonitorData.StartTimer(ca::ETimer::CaHitCreation);
      ca::Hit caHit;
      if constexpr (IsSts) {
        caHit.SetFrontKey(firstHitKey + hit.fFrontClusterId);
        caHit.SetBackKey(firstHitKey + hit.fBackClusterId);
        //L_(info) << ", hit="  << iHitExt << ", f=" << hit.fFrontClusterId << ", b=" << hit.fBackClusterId;
      }
      else {
        caHit.SetFrontKey(firstHitKey + iPartHit);
        caHit.SetBackKey(caHit.FrontKey());
      }

      if constexpr (IsTof) {
        // Cut the BMon hits (if any)
        if (hit.Z() < 0.1) {
          // FIXME: Provide BMon addresses explicitly in all the parameter files
          continue;
        }
      }

      caHit.SetX(hit.X());
      caHit.SetY(hit.Y());
      caHit.SetZ(hit.Z());
      caHit.SetT(hit.Time());
      caHit.SetDx2(hit.Dx() * hit.Dx() + fCaFramework.GetParameters().GetMisalignmentXsq(DetID));
      caHit.SetDy2(hit.Dy() * hit.Dy() + fCaFramework.GetParameters().GetMisalignmentYsq(DetID));
      if constexpr (IsSts) caHit.SetDxy(hit.fDxy);
      caHit.SetDt2(hit.TimeError() * hit.TimeError() + fCaFramework.GetParameters().GetMisalignmentTsq(DetID));
      /// FIXME: Define ranges from the hit, when will be available
      //out << iStLocal << " " << extHitAddress << " " << hit.Z() << '\n';
      caHit.SetRangeX(3.5 * hit.Dx());
      caHit.SetRangeY(3.5 * hit.Dy());
      caHit.SetRangeT(3.5 * hit.TimeError());
      if constexpr (IsTrd) {
        if (iStLocal == 0) {
          caHit.SetRangeX(1.7);
          caHit.SetRangeY(3.5);
          caHit.SetRangeT(200.);
        }
        else if (iStLocal == 1) {
          caHit.SetRangeY(sqrt(3.) * hit.Dy());
        }
        else {
          caHit.SetRangeX(sqrt(3.) * hit.Dx());
        }
      }
      caHit.SetStation(iStActive);
      caHit.SetId(fCaDataManager.GetNofHits());
      if (caHit.Check()) {
        if (ECbmRecoMode::Timeslice == fRecoMode) {
          if ((caHit.T() < lastTime - 1000.) && (dataStream < 100000)) {
            dataStream++;
          }
          lastTime = caHit.T();
          fCaDataManager.PushBackHit(caHit, dataStreamDet | dataStream);
        }
        else {
          fCaDataManager.PushBackHit(caHit);  // A single data stream in event-by-event mode
        }
        faHitExternalIndices.push_back(std::make_tuple(DetID, iPartition, iPartHit));
        if (fNofHitKeys <= caHit.FrontKey()) {
          fNofHitKeys = caHit.FrontKey() + 1;
        }
        if (fNofHitKeys <= caHit.BackKey()) {
          fNofHitKeys = caHit.BackKey() + 1;
        }
      }
      else {
        if constexpr (IsMvd) {
          fCaMonitorData.IncrementCounter(ca::ECounter::UndefinedMvdHit);
        }
        if constexpr (IsSts) {
          fCaMonitorData.IncrementCounter(ca::ECounter::UndefinedStsHit);
        }
        if constexpr (IsMuch) {
          fCaMonitorData.IncrementCounter(ca::ECounter::UndefinedMuchHit);
        }
        if constexpr (IsTrd) {
          fCaMonitorData.IncrementCounter(ca::ECounter::UndefinedTrdHit);
        }
        if constexpr (IsTof) {
          fCaMonitorData.IncrementCounter(ca::ECounter::UndefinedTofHit);
        }
      }
      fCaMonitorData.StopTimer(ca::ETimer::CaHitCreation);
      //prevTime = caHit.T();
      // ---- Update number of hit keys
    }  // iPartHit
  }    // iPartition
}
