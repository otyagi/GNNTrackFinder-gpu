/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaInitManager.cxx
/// \brief  Input data management class for the CA tracking algorithm (implementation)
/// \since  19.01.2022
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CaInitManager.h"

#include "CaConfigReader.h"
#include "KfSetupBuilder.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace cbm::algo::ca
{
  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::AddStation(const StationInitializer& inStation) { fvStationInfo.push_back(inStation); }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::AddStations(const Parameters<fvec>& par)
  {
    const auto& stations = par.GetStations();
    for (int iSt = 0; iSt < par.GetNstationsActive(); ++iSt) {
      auto stationIn = Station<double>(stations[iSt]);
      // Get detector ID of the station
      auto [detId, locId] = par.GetStationIndexLocal(par.GetActiveToGeoMap()[iSt]);
      auto stationInfo    = ca::StationInitializer(detId, locId);
      stationInfo.SetStationType(stationIn.type);
      stationInfo.SetZmin(0.);  // NOTE: Deprecated (not used)
      stationInfo.SetZmax(0.);  // NOTE: Deprecated (not used)
      stationInfo.SetZref(stationIn.fZ);
      stationInfo.SetXmax(stationIn.Xmax);
      stationInfo.SetYmax(stationIn.Ymax);
      stationInfo.SetFieldStatus(stationIn.fieldStatus);
      stationInfo.SetTimeInfo(stationIn.timeInfo);
      stationInfo.SetTrackingStatus(true);
      this->AddStation(stationInfo);
    }
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::CheckInit()
  {
    this->CheckCAIterationsInit();
    this->CheckStationsInfoInit();
    fbConfigIsRead       = false;
    fbGeometryConfigLock = false;
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ClearSetupInfo()
  {
    // Clear stations set and a thickness map
    fvStationInfo.clear();
    fInitController.SetFlag(EInitKey::kStationsInfo, false);

    // Set number of stations do default values
    this->ClearStationLayout();

    // Clear field info
    fParameters.fVertexFieldRegion = kf::FieldRegion<fvec>();
    fParameters.fVertexFieldValue  = kf::FieldValue<fvec>();
    fInitController.SetFlag(EInitKey::kPrimaryVertexField, false);

    // Clear target position
    fParameters.fTargetPos.fill(Undef<float>);
    fTargetZ = 0.;
    fInitController.SetFlag(EInitKey::kTargetPos, false);

    // Clear field function
    fFieldFunction = FieldFunction_t([](const double(&)[3], double(&)[3]) {});
    fInitController.SetFlag(EInitKey::kFieldFunction, false);

    // Clear other flags
    fParameters.fRandomSeed       = 1;
    fParameters.fGhostSuppression = 0;
    fInitController.SetFlag(EInitKey::kRandomSeed, false);
    fInitController.SetFlag(EInitKey::kGhostSuppression, false);

    fParameters.fMisalignmentX.fill(0.);
    fParameters.fMisalignmentY.fill(0.);
    fParameters.fMisalignmentT.fill(0.);

    fParameters.fDevIsIgnoreHitSearchAreas     = false;
    fParameters.fDevIsUseOfOriginalField       = false;
    fParameters.fDevIsMatchDoubletsViaMc       = false;
    fParameters.fDevIsMatchTripletsViaMc       = false;
    fParameters.fDevIsExtendTracksViaMc        = false;
    fParameters.fDevIsSuppressOverlapHitsViaMc = false;
    fParameters.fDevIsParSearchWUsed           = false;

    fbGeometryConfigLock = false;
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ClearCAIterations()
  {
    fParameters.fCAIterations.clear();
    fCAIterationsNumberCrosscheck = -1;
    fInitController.SetFlag(EInitKey::kCAIterations, false);
    fInitController.SetFlag(EInitKey::kCAIterationsNumberCrosscheck, false);
  }

  // --------------------------------------------------------------------------------------------------------------------
  // NOTE: this function should be called once in the SendParameters
  bool InitManager::FormParametersContainer()
  {
    // Read configuration files
    this->ReadInputConfigs();
    if (!fbConfigIsRead) {  // Check config reading status
      return false;
    }

    if (!fParameters.fDevIsParSearchWUsed) {
      fInitController.SetFlag(EInitKey::kSearchWindows, true);
    }

    // Apply magnetic field to the station info objects
    std::for_each(fvStationInfo.begin(), fvStationInfo.end(), [&](auto& st) { st.SetFieldFunction(fFieldFunction); });

    // Check initialization
    this->CheckInit();

    if (!fInitController.IsFinalized()) {
      LOG(error) << "ca::InitManager: Attempt to form parameters container before all necessary fields were initialized"
                 << fInitController.ToString();
      return false;
    }

    {  // Form array of stations
      auto destIt = fParameters.fStations.begin();
      for (const auto& station : fvStationInfo) {
        if (!station.GetTrackingStatus()) {
          continue;
        }
        *destIt = station.GetStation();
        ++destIt;
      }
    }

    // Check the consistency of the parameters object. If object inconsistent, it throws std::logic_error
    try {
      fParameters.CheckConsistency();
    }
    catch (const std::logic_error& err) {
      LOG(error) << "ca::InitManager: parameters container consistency check failed. Reason: " << err.what();
      return false;
    }

    return true;
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  int InitManager::GetNstationsActive() const
  {
    if (!fInitController.GetFlag(EInitKey::kStationLayoutInitialized)) {
      std::stringstream msg;
      msg << "ca::InitManager: number of active stations cannot be accessed until the station layout is initialized";
      throw std::runtime_error(msg.str());
    }
    return fParameters.GetNstationsActive();
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  int InitManager::GetNstationsActive(EDetectorID detectorID) const
  {
    if (!fInitController.GetFlag(EInitKey::kStationLayoutInitialized)) {
      std::stringstream msg;
      msg << "ca::InitManager: number of active stations cannot be accessed until the station layout is initialized";
      throw std::runtime_error(msg.str());
    }
    return fParameters.GetNstationsActive(detectorID);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  int InitManager::GetNstationsGeometry() const
  {
    if (!fInitController.GetFlag(EInitKey::kStationLayoutInitialized)) {
      std::stringstream msg;
      msg << "ca::InitManager: number of geometry stations cannot be accessed until the station layout is initialized";
      throw std::runtime_error(msg.str());
    }
    return fParameters.GetNstationsGeometry();
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  int InitManager::GetNstationsGeometry(EDetectorID detectorID) const
  {
    if (!fInitController.GetFlag(EInitKey::kStationLayoutInitialized)) {
      std::stringstream msg;
      msg << "ca::InitManager: number of geometry stations cannot be accessed until the station layout is initialized";
      throw std::runtime_error(msg.str());
    }
    return fParameters.GetNstationsGeometry(detectorID);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  std::vector<StationInitializer>& InitManager::GetStationInfo()
  {
    if (!fInitController.GetFlag(EInitKey::kStationLayoutInitialized)) {
      std::stringstream msg;
      msg << "ca::InitManager: station info container cannot be accessed until the station layout is initialized";
      throw std::runtime_error(msg.str());
    }
    return fvStationInfo;
  }


  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::InitStationLayout()
  {
    LOG(info) << "ca::InitManager::InitStationLayout(): ....";
    this->ClearStationLayout();
    std::sort(fvStationInfo.begin(), fvStationInfo.end());

    for (const auto& aStation : fvStationInfo) {
      ++fParameters.fvFirstGeoId[static_cast<int>(aStation.GetDetectorID()) + 1];
    }
    for (int iDet = 1; iDet < static_cast<int>(fParameters.fvFirstGeoId.size()) - 1; ++iDet) {
      fParameters.fvFirstGeoId[iDet + 1] += fParameters.fvFirstGeoId[iDet];
    }

    fParameters.fNstationsActiveTotal = 0;
    for (int iStGeo = 0; iStGeo < static_cast<int>(fvStationInfo.size()); ++iStGeo) {
      const auto& aStation = fvStationInfo[iStGeo];
      int iDet             = static_cast<int>(aStation.GetDetectorID());
      int iStLocal         = aStation.GetStationID();
      // Fill local -> geo map
      fParameters.fvGeoToLocalIdMap[iStGeo] = std::make_pair(aStation.GetDetectorID(), iStLocal);
      // Fill geo -> local map
      fParameters.fvLocalToGeoIdMap[fParameters.fvFirstGeoId[iDet] + iStLocal] = iStGeo;
      // Fill geo <-> active map
      int iStActive                        = aStation.GetTrackingStatus() ? fParameters.fNstationsActiveTotal++ : -1;
      fParameters.fvGeoToActiveMap[iStGeo] = iStActive;
      if (iStActive > -1) {
        fParameters.fvActiveToGeoMap[iStActive] = iStGeo;
      }
    }
    fInitController.SetFlag(EInitKey::kStationLayoutInitialized, true);

    for (auto& aStation : fvStationInfo) {
      aStation.SetGeoLayerID(fParameters.GetStationIndexGeometry(aStation.GetStationID(), aStation.GetDetectorID()));
    }
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::InitTargetField(double zStep)
  {
    if (fInitController.GetFlag(EInitKey::kPrimaryVertexField)) {
      LOG(warn) << "ca::InitManager::InitTargetField: attempt to reinitialize the field value and field region "
                << "near target. Ignore";
      return;
    }

    // Check for field function
    if (!fInitController.GetFlag(EInitKey::kFieldFunction)) {
      std::stringstream msg;
      msg << "Attempt to initialize the field value and field region near target before initializing field function";
      throw std::runtime_error(msg.str());
    }
    // Check for target defined
    if (!fInitController.GetFlag(EInitKey::kTargetPos)) {
      std::stringstream msg;
      msg << "Attempt to initialize the field value and field region near target before the target position"
          << "initialization";
      throw std::runtime_error(msg.str());
    }
    constexpr int nDimensions{3};
    constexpr int nPointsNodal{3};

    std::array<double, nPointsNodal> inputNodalZ{fTargetZ, fTargetZ + zStep, fTargetZ + 2. * zStep};
    std::array<kf::FieldValue<fvec>, nPointsNodal> B{};
    std::array<fvec, nPointsNodal> z{};
    // loop over nodal points
    for (int idx = 0; idx < nPointsNodal; ++idx) {
      double point[nDimensions]{0., 0., inputNodalZ[idx]};
      double field[nDimensions]{};
      fFieldFunction(point, field);
      z[idx] = inputNodalZ[idx];
      B[idx].Set(field[0], field[1], field[2]);
    }  // loop over nodal points: end
    fParameters.fVertexFieldRegion.Set(B[0], z[0], B[1], z[1], B[2], z[2]);
    fParameters.fVertexFieldValue = B[0];

    fInitController.SetFlag(EInitKey::kPrimaryVertexField);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::PushBackCAIteration(const Iteration& iteration)
  {
    // TODO: probably some checks must be inserted here (S.Zharko)
    if (!fInitController.GetFlag(EInitKey::kCAIterationsNumberCrosscheck)) {
      std::stringstream msg;
      msg << "Attempt to push back a CA track finder iteration before the number of iterations was defined";
      throw std::runtime_error(msg.str());
    }
    fParameters.fCAIterations.push_back(iteration);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ReadInputConfigs()
  {
    if (!fbConfigIsRead) {
      LOG(info) << "ca::InitManager: reading parameter configuration ...";
      try {
        auto configReader = ConfigReader(this, 4);
        configReader.SetMainConfigPath(fsConfigInputMain);
        configReader.SetUserConfigPath(fsConfigInputUser);
        configReader.SetGeometryLock(fbGeometryConfigLock);
        configReader.Read();
        fbConfigIsRead = true;
        LOG(info) << "ca::InitManager: reading parameter configuration ... \033[1;32mdone\033[0m";
      }
      catch (const std::runtime_error& err) {
        LOG(error) << "ca::InitManager: reading parameter configuration ... \033[1;31mfail\033[0m. Reason: "
                   << err.what();
      }
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ReadGeometrySetup(const std::string& fileName)
  {
    // Open input binary file
    this->SetGeometrySetup(cbm::algo::kf::SetupBuilder::Load<fvec>(fileName));
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ReadParametersObject(const std::string& fileName)
  {
    using namespace constants;
    // clrs::CL  - end colored log
    // clrs::GNb - bold green log
    // clrs::RDb - bold red log

    // Open input binary file
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
      std::stringstream msg;
      msg << "ca::InitManager: parameters data file \"" << clrs::GNb << fileName << clrs::CL << "\" was not found";
      throw std::runtime_error(msg.str());
    }

    // Get L1InputData object
    try {
      boost::archive::binary_iarchive ia(ifs);
      ia >> fParameters;
      fbGeometryConfigLock = true;
    }
    catch (const std::exception&) {
      std::stringstream msg;
      msg << "ca::InitManager: parameters file \"" << clrs::GNb << fileName << clrs::CL
          << "\" has incorrect data format or was corrupted";
      throw std::runtime_error(msg.str());
    }

    fInitController.SetFlag(EInitKey::kStationLayoutInitialized, true);
    fInitController.SetFlag(EInitKey::kPrimaryVertexField, true);
    fInitController.SetFlag(EInitKey::kSearchWindows, true);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ReadSearchWindows(const std::string& fileName)
  {
    // Open input binary file
    std::ifstream ifs(fileName);
    if (!ifs) {
      std::stringstream msg;
      msg << "ca::InitManager: search window file \"" << fileName << "\" was not found";
      throw std::runtime_error(msg.str());
    }

    try {
      boost::archive::text_iarchive ia(ifs);
      int nPars    = -1;
      int nWindows = -1;
      ia >> nPars;
      assert(nPars == 1);  // Currently only the constant windows are available
      ia >> nWindows;
      std::stringstream errMsg;
      for (int iW = 0; iW < nWindows; ++iW) {
        SearchWindow swBuffer;
        ia >> swBuffer;
        int iStationID = swBuffer.GetStationID();
        int iTrackGrID = swBuffer.GetTrackGroupID();
        if (iStationID < 0 || iStationID > constants::size::MaxNstations) {
          errMsg << "\t- wrong station id for entry " << iW << ": " << iStationID << " (should be between 0 and "
                 << constants::size::MaxNstations << ")\n";
        }
        if (iTrackGrID < 0 || iTrackGrID > constants::size::MaxNtrackGroups) {
          errMsg << "\t- wrong track group id for entry " << iW << ": " << iTrackGrID << " (should be between 0 and "
                 << constants::size::MaxNtrackGroups << ")\n";
        }
        fParameters.fSearchWindows[iTrackGrID * constants::size::MaxNstations + iStationID] = swBuffer;
      }
      if (errMsg.str().size()) {
        std::stringstream msg;
        msg << "ca::InitManager: some errors occurred while reading search windows: " << errMsg.str();
        throw std::runtime_error(msg.str());
      }
    }
    catch (const std::exception& err) {
      std::stringstream msg;
      msg << "search windows file \"" << fileName << "\" has incorrect data format or was corrupted. ";
      msg << "Exception catched: " << err.what();
      throw std::runtime_error(msg.str());
    }

    fInitController.SetFlag(EInitKey::kSearchWindows, true);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::SetCAIterationsNumberCrosscheck(int nIterations)
  {
    fCAIterationsNumberCrosscheck = nIterations;
    auto& iterationsContainer     = fParameters.fCAIterations;

    // NOTE: should be called to prevent multiple copies of objects between the memory reallocations
    iterationsContainer.reserve(nIterations);
    fInitController.SetFlag(EInitKey::kCAIterationsNumberCrosscheck);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::SetFieldFunction(const FieldFunction_t& fieldFunction)
  {
    if (!fInitController.GetFlag(EInitKey::kFieldFunction)) {
      fFieldFunction = fieldFunction;
      fInitController.SetFlag(EInitKey::kFieldFunction);
    }
    else {
      LOG(warn) << "ca::InitManager::SetFieldFunction: attempt to reinitialize the field function. Ignored";
    }
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::SetGhostSuppression(int ghostSuppression)
  {
    if (fInitController.GetFlag(EInitKey::kGhostSuppression)) {
      LOG(warn)
        << "ca::InitManager::SetGhostSuppression: attempt of reinitializating the ghost suppresion flag. Ignore";
      return;
    }
    fParameters.fGhostSuppression = ghostSuppression;
    fInitController.SetFlag(EInitKey::kGhostSuppression);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::SetRandomSeed(unsigned int seed)
  {
    if (fInitController.GetFlag(EInitKey::kRandomSeed)) {
      LOG(warn) << "ca::InitManager::SetRandomSeed: attempt of reinitializating the random seed. Ignore";
      return;
    }
    fParameters.fRandomSeed = seed;
    fInitController.SetFlag(EInitKey::kRandomSeed);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::SetTargetPosition(double x, double y, double z)
  {
    if (fInitController.GetFlag(EInitKey::kTargetPos)) {
      LOG(warn) << "ca::InitManager::SetTargetPosition: attempt to reinitialize the target position. Ignore";
      return;
    }

    /// Fill fvec target fields
    fParameters.fTargetPos[0] = x;
    fParameters.fTargetPos[1] = y;
    fParameters.fTargetPos[2] = z;
    /// Set additional field for z component in double precision
    fTargetZ = z;
    fInitController.SetFlag(EInitKey::kTargetPos);
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  Parameters<fvec>&& InitManager::TakeParameters() { return std::move(fParameters); }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::WriteParametersObject(const std::string& fileName) const
  {
    using constants::clrs::CL;
    using constants::clrs::GNb;
    // Open output binary file
    std::ofstream ofs(fileName, std::ios::binary);
    if (!ofs) {
      std::stringstream msg;
      msg << "ca::InitManager: failed opening file \"" << GNb << fileName << CL << "\" to write parameters object";
      throw std::runtime_error(msg.str());
    }

    LOG(info) << "ca::InitManager: writing CA parameters object to file \"" << GNb << fileName << '\"' << CL;

    // Serialize L1Parameters object and write
    boost::archive::binary_oarchive oa(ofs);
    oa << fParameters;
  }

  // --------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::CheckCAIterationsInit()
  {
    bool ifInitPassed = true;
    if (!fInitController.GetFlag(EInitKey::kCAIterations)) {
      int nIterationsActual   = fParameters.fCAIterations.size();
      int nIterationsExpected = fCAIterationsNumberCrosscheck;
      if (nIterationsActual != nIterationsExpected) {
        LOG(warn) << "ca::InitManager::CheckCAIterations: incorrect number of iterations registered: "
                  << nIterationsActual << " of " << nIterationsExpected << " expected";
        ifInitPassed = false;
      }
    }
    fInitController.SetFlag(EInitKey::kCAIterations, ifInitPassed);
  }

  // --------------------------------------------------------------------------------------------------------------------
  // TODO: REWRITE! and add const qualifier (S.Zharko)
  void InitManager::CheckStationsInfoInit()
  {
    bool ifInitPassed = true;
    if (!fInitController.GetFlag(EInitKey::kStationsInfo)) {
      // (1) Check the stations themselves
      bool bStationsFinalized = std::all_of(fvStationInfo.begin(), fvStationInfo.end(),
                                            [](const auto& st) { return st.GetInitController().IsFinalized(); });
      if (!bStationsFinalized) {
        std::stringstream msg;
        msg << "ca::InitManager: At least one of the StationInitializer objects is not finalized";
      }

      // (2) Check for maximum allowed number of stations
      if (fParameters.GetNstationsGeometry() > constants::size::MaxNstations) {
        std::stringstream msg;
        msg << "Actual total number of registered stations in geometry (" << fParameters.GetNstationsGeometry()
            << ") is larger then possible (" << constants::size::MaxNstations
            << "). Please, select another set of active tracking detectors or recompile the code with enlarged"
            << " constants::size::MaxNstations value";
        throw std::runtime_error(msg.str());
      }
    }
    fInitController.SetFlag(EInitKey::kStationsInfo, ifInitPassed);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void InitManager::ClearStationLayout()
  {
    fParameters.fvFirstGeoId.fill(0);
    fParameters.fvLocalToGeoIdMap.fill(0);
    fParameters.fvGeoToLocalIdMap.fill(std::make_pair(static_cast<EDetectorID>(0), -1));
    fParameters.fvGeoToActiveMap.fill(-1);  // Note: by default all the stations are inactive
    fParameters.fvActiveToGeoMap.fill(0);
    fParameters.fNstationsActiveTotal = -1;
    fInitController.SetFlag(EInitKey::kStationLayoutInitialized, false);
  }
}  // namespace cbm::algo::ca
