/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmStsTrackingInterface.h
 * @brief  Input data and parameters interface from STS subsystem used in L1 tracker (declaration)
 * @since  27.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#ifndef CbmStsTrackingInterface_h
#define CbmStsTrackingInterface_h 1

#include "CbmHit.h"
#include "CbmStsParSetModule.h"
#include "CbmStsParSetSensor.h"
#include "CbmStsParSetSensorCond.h"
#include "CbmStsSetup.h"
#include "CbmStsStation.h"
#include "CbmTrackingDetectorInterfaceBase.h"

#include "FairTask.h"

#include "TMath.h"

#include <iostream>


/// @class CbmStsTrackingInterface
/// @brief A CbmL1 subtask, which provides necessary methods for CA tracker to access the geometry and dataflow
///        settings
///
class CbmStsTrackingInterface : public FairTask, public CbmTrackingDetectorInterfaceBase {
public:
  /// @brief Default constructor
  CbmStsTrackingInterface();

  /// @brief Destructor
  ~CbmStsTrackingInterface();

  /// @brief Copy constructor
  CbmStsTrackingInterface(const CbmStsTrackingInterface&) = delete;

  /// @brief Move constructor
  CbmStsTrackingInterface(CbmStsTrackingInterface&&) = delete;

  /// @brief Copy assignment operator
  CbmStsTrackingInterface& operator=(const CbmStsTrackingInterface&) = delete;

  /// @brief Move assignment operator
  CbmStsTrackingInterface& operator=(CbmStsTrackingInterface&&) = delete;

  /// @brief FairTask: Init method
  InitStatus Init() override;

  /// @brief FairTask: ReInit method
  InitStatus ReInit() override;

  /// @brief Gets pointer to the instance of the CbmStsTrackingInterface class
  static CbmStsTrackingInterface* Instance() { return fpInstance; }

  /// @brief Gets name of this subsystem
  std::string GetDetectorName() const override { return "STS"; }

  /// @brief  Gets stereo angles of the two independent measured coordinates
  /// @note   The tracking does not use this method. It is only used by the QA task.
  /// @param  address  detector unique identifier
  /// @return [phiU, phiV] - Stereo angles [rad]
  std::tuple<double, double> GetStereoAnglesSensor(int address) const override;

  /// @brief  Gets a tracking station of a CbmHit
  /// @param  hit  A pointer to CbmHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const CbmHit* hit) const override { return GetTrackingStationIndex(hit->GetAddress()); }

  /// @brief  Gets a tracking station by the address of element
  /// @param  address  Unique element address
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(int address) const override { return CbmStsSetup::Instance()->GetStationNumber(address); }

  /// @brief  Gets a tracking station of a FairMCPoint
  /// @param  point  A pointer to FairMCHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const FairMCPoint* point) const override
  {
    return GetTrackingStationIndex(point->GetDetectorID());
  }

  /// @brief  Check if station provides time measurements
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Flag: true - station provides time measurements, false - station does not provide time measurements
  bool IsTimeInfoProvided(int /*stationId*/) const override { return true; }

  /// -- to be removed -- Gets station radiation length
  /// \param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// \return Radiation length [cm]
  // TODO: remove this method
  [[deprecated]] double GetRadLength(int stationId) const { return GetStsStation(stationId)->GetRadLength(); }

  /// -- to be removed -- Gets station thickness along the Z-axis
  /// \param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// \return Station thickness [cm]
  // TODO: remove this method
  [[deprecated]] double GetSensorThickness(int stationId) const { return GetStsStation(stationId)->GetSensorD(); }

  /// @brief FairTask: sets parameter containers up
  void SetParContainers() override;

 private:
  /// @brief  Gets pointer to the STS station object
  /// @param  stationId  Tracking staton ID
  /// @return Pointer to the particular CbmStsStation object
  __attribute__((always_inline)) CbmStsStation* GetStsStation(int stationId) const
  {
    return CbmStsSetup::Instance()->GetStation(stationId);
  }

  inline static CbmStsTrackingInterface* fpInstance{nullptr};

  CbmStsParSetSensor* fStsParSetSensor{nullptr};          ///<
  CbmStsParSetSensorCond* fStsParSetSensorCond{nullptr};  ///<
  CbmStsParSetModule* fStsParSetModule{nullptr};          ///<

  ClassDefOverride(CbmStsTrackingInterface, 0);
};

#endif  // CbmStsTrackingInterface
