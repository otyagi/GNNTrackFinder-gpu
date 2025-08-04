/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmTrdTrackingInterface.h
 * @brief  Input data and parameters interface from TRD subsystem used in L1 tracker (declaration)
 * @since  31.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#ifndef CbmTrdTrackingInterface_h
#define CbmTrdTrackingInterface_h 1

#include "CbmHit.h"
#include "CbmTrackingDetectorInterfaceBase.h"
#include "CbmTrdAddress.h"
#include "CbmTrdParModDigi.h"
#include "CbmTrdParSetDigi.h"
#include "FairTask.h"
#include "TMath.h"

#include <iostream>
#include <vector>

/// @class CbmTrdTrackingInterface
/// @brief A CbmL1 subtask, which provides necessary methods for CA tracker to access the geometry and dataflow s
/// ettings.
///
/// @note For TRD one tracking station is a TRD module!
///
class CbmTrdTrackingInterface : public FairTask, public CbmTrackingDetectorInterfaceBase {
 public:
  /// @brief Default constructor
  CbmTrdTrackingInterface();

  /// @brief Destructor
  ~CbmTrdTrackingInterface();

  /// @brief Copy constructor
  CbmTrdTrackingInterface(const CbmTrdTrackingInterface&) = delete;

  /// @brief Move constructor
  CbmTrdTrackingInterface(CbmTrdTrackingInterface&&) = delete;

  /// @brief Copy assignment operator
  CbmTrdTrackingInterface& operator=(const CbmTrdTrackingInterface&) = delete;

  /// @brief Move assignment operator
  CbmTrdTrackingInterface& operator=(CbmTrdTrackingInterface&&) = delete;

  /// @brief Gets name of this subsystem
  std::string GetDetectorName() const override { return "TRD"; }

  /// @brief  Gets stereo angles of the two independent measured coordinates
  /// @note   The tracking does not use this method. It is only used by the QA task.
  /// @param  address  detector unique identifier
  /// @return [phiU, phiV] - Stereo angles [rad]
  std::tuple<double, double> GetStereoAnglesSensor(int address) const override;

  /// @brief  Gets a tracking station of a CbmHit
  /// @param  hit  A pointer to CbmHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const CbmHit* hit) const override { return GetTrackingStationIndex(hit->GetAddress()); }

  /// @brief  Gets a tracking station by the address
  /// @param  address  Unique element address
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(int address) const override { return CbmTrdAddress::GetLayerId(address); }

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

  /// @brief  Gets x,y,t ranges of a CbmTrdHit
  /// @param  hit  A hit
  /// @return range X, Y, T
  std::tuple<double, double, double> GetHitRanges(const CbmPixelHit& hit) const override;

  /// @brief FairTask: Init method
  InitStatus Init() override;

  /// @brief Gets pointer to the instance of the CbmTrdTrackingInterface
  static CbmTrdTrackingInterface* Instance() { return fpInstance; }

  /// @brief FairTask: ReInit method
  InitStatus ReInit() override;

  /// @brief  FairTask: sets parameter containers up
  void SetParContainers() override;

 private:
  /// @brief  Gets pointer to the TRD module
  /// @param  moduleId  Id of the Trd module
  /// @return Pointer to the particular CbmTrdParModDigi object
  __attribute__((always_inline)) CbmTrdParModDigi* GetTrdModulePar(int moduleId) const
  {
    return static_cast<CbmTrdParModDigi*>(fTrdDigiPar->GetModulePar(fTrdDigiPar->GetModuleId(moduleId)));
  }

  inline static CbmTrdTrackingInterface* fpInstance{nullptr};  ///< Instance of the class

  CbmTrdParSetDigi* fTrdDigiPar{nullptr};
  //CbmTrdParModDigi* fTrdModuleInfo {nullptr};

  ClassDefOverride(CbmTrdTrackingInterface, 0);
};

#endif  // CbmTrdTrackingInterface
