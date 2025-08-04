/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmMuchTrackingInterface.h
 * @brief  Input data and parameters interface from MuCh subsystem used in L1 tracker (declaration)
 * @since  31.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#ifndef CbmMuchTrackingInterface_h
#define CbmMuchTrackingInterface_h 1

#include "CbmHit.h"
#include "CbmMuchGeoScheme.h"
#include "CbmMuchModuleGem.h"
#include "CbmMuchPad.h"
#include "CbmMuchStation.h"
#include "CbmTrackingDetectorInterfaceBase.h"

#include "FairTask.h"

#include "TMath.h"
#include "TString.h"

#include <iostream>
#include <vector>

class CbmMuchLayer;

/// @class CbmMuchTrackingInterface
/// @brief A CbmL1 subtask, which provides necessary methods for L1 tracker to access the geometry and dataflow
///        settings.
///
/// @note  For MuCh one tracking station is a MuCh layer!
///
class CbmMuchTrackingInterface : public FairTask, public CbmTrackingDetectorInterfaceBase {
public:
  /// @brief Default constructor
  CbmMuchTrackingInterface();

  /// @brief Destructor
  ~CbmMuchTrackingInterface();

  /// @brief Copy constructor
  CbmMuchTrackingInterface(const CbmMuchTrackingInterface&) = delete;

  /// @brief Move constructor
  CbmMuchTrackingInterface(CbmMuchTrackingInterface&&) = delete;

  /// @brief Copy assignment operator
  CbmMuchTrackingInterface& operator=(const CbmMuchTrackingInterface&) = delete;

  /// @brief Move assignment operator
  CbmMuchTrackingInterface& operator=(CbmMuchTrackingInterface&&) = delete;

  /// @brief FairTask: Init method
  InitStatus Init() override;

  /// @brief FairTask: ReInit method
  InitStatus ReInit() override;

  /// @brief Gets pointer to the instance of the CbmMuchTrackingInterface
  static CbmMuchTrackingInterface* Instance() { return fpInstance; }

  /// @brief Gets name of this subsystem
  std::string GetDetectorName() const override { return "MuCh"; }

  /// @brief  Gets a tracking station of a CbmHit
  /// @param  hit  A pointer to CbmHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const CbmHit* hit) const override { return GetTrackingStationIndex(hit->GetAddress()); }

  /// @brief  Gets a tracking station by the address of element
  /// @param  address  Unique element address
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(int address) const override
  {
    return fFirstTrackingStation.at(CbmMuchGeoScheme::GetStationIndex(address))
           + CbmMuchGeoScheme::GetLayerIndex(address);
  }

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

  /// @brief  Gets stereo angles of the two independent measured coordinates
  /// @note   The tracking does not use this method. It is only used by the QA task.
  /// @param  address  detector unique identifier
  /// @return [phiU, phiV] - Stereo angles [rad]
  std::tuple<double, double> GetStereoAnglesSensor(int /*address*/) const override
  {
    return std::tuple(0., TMath::Pi() * 0.5);
  }

  /// @brief FairTask: sets parameter containers up
  void SetParContainers() override;

 private:
  /// @brief  Gets pointer to the TRD module
  /// @param  stationId  Tracking staton ID
  /// @return Pointer to the particular CbmTrdParModDigi object
  __attribute__((always_inline)) CbmMuchLayer* GetMuchLayer(int traStationId) const
  {
    auto [muchSta, muchLayer] = ConvTrackingStationId2MuchId(traStationId);
    return fGeoScheme->GetLayer(muchSta, muchLayer);
  }

  /// @brief Calculates MuCh station ID from tracker station ID
  /// @param stationId  Index of the tracking station
  std::pair<int, int> ConvTrackingStationId2MuchId(int traStationId) const;

  inline static CbmMuchTrackingInterface* fpInstance{nullptr};  ///< Instance of the class

  CbmMuchGeoScheme* fGeoScheme {nullptr};  ///< MuCh geometry scheme instance

  std::vector<int> fFirstTrackingStation {};

  ClassDefOverride(CbmMuchTrackingInterface, 0);
};


#endif  // CbmMuchTrackingInterface
