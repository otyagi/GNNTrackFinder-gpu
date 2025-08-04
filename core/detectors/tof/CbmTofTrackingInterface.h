/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmTofTrackingInterface.h
 * @brief  Input data and parameters interface from TOF subsystem used in L1 tracker (declaration)
 * @since  23.06.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#ifndef CbmTofTrackingInterface_h
#define CbmTofTrackingInterface_h 1

#include "CbmHit.h"
#include "CbmTofAddress.h"
#include "CbmTofCell.h"
#include "CbmTofDigiBdfPar.h"
#include "CbmTofDigiPar.h"
#include "CbmTofHit.h"
#include "CbmTrackingDetectorInterfaceBase.h"
#include "FairTask.h"
#include "Logger.h"
#include "TMath.h"

#include <vector>

class CbmTofDigiPar;

/// Class CbmTofTrackingInterface is a CbmTrackingDetetorInterfaceInit subtask, which provides necessary methods for L1 tracker
/// to access the geometry and dataflow settings.
///
class CbmTofTrackingInterface : public FairTask, public CbmTrackingDetectorInterfaceBase {
 public:
  /// @brief Default constructor
  CbmTofTrackingInterface();

  /// @brief Destructor
  ~CbmTofTrackingInterface();

  /// @brief Copy constructor
  CbmTofTrackingInterface(const CbmTofTrackingInterface&) = delete;

  /// @brief Move constructor
  CbmTofTrackingInterface(CbmTofTrackingInterface&&) = delete;

  /// @brief Copy assignment operator
  CbmTofTrackingInterface& operator=(const CbmTofTrackingInterface&) = delete;

  /// @brief Move assignment operator
  CbmTofTrackingInterface& operator=(CbmTofTrackingInterface&&) = delete;

  /// @brief Gets name of this subsystem
  std::string GetDetectorName() const override { return "TOF"; }

  /// @brief  Gets stereo angles of the two independent measured coordinates
  /// @note   The tracking does not use this method. It is only used by the QA task.
  /// @param  address  detector unique identifier
  /// @return [phiU, phiV] - Stereo angles [rad]
  std::tuple<double, double> GetStereoAnglesSensor(int /*address*/) const override
  {
    return std::tuple(0., TMath::Pi() * 0.5);
  }

  /// @brief  Gets a tracking station of a ToF hit
  /// @param  hit  A pointer to ToF hit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const CbmHit* hit) const override { return GetTrackingStationIndex(hit->GetAddress()); }

  /// @brief  Gets a tracking station by the address of element
  /// @param  address  Unique element address
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(int address) const override
  {
    int iSt = fDigiBdfPar->GetTrackingStation(CbmTofAddress::GetSmType(address), CbmTofAddress::GetSmId(address),
                                              CbmTofAddress::GetRpcId(address));
    return iSt;
  }

  /// @brief  Gets a tracking station of a FairMCPoint
  /// @param  point  A pointer to FairMCHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const FairMCPoint* point) const override
  {
    return GetTrackingStationIndex(point->GetDetectorID());
  }

  /// @brief  Gets reference z of the detector module (e.g., RPC for TOF)
  /// @param  address  Address of the module
  /// @return Reference z of module (if not defined, the ref z of the station will be returned)
  double GetZrefModule(int address) override
  {
    auto* pCell = dynamic_cast<CbmTofCell*>(fDigiPar->GetCell(address));
    assert(pCell);
    return pCell->GetZ();
  }

  /// @brief  Check if station provides time measurements
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Flag: true - station provides time measurements, false - station does not provide time measurements
  bool IsTimeInfoProvided(int /*stationId*/) const override { return true; }

  /// @brief FairTask: Init method
  InitStatus Init() override;

  /// @brief Gets pointer to the instance of the CbmTofTrackingInterface
  static CbmTofTrackingInterface* Instance() { return fpInstance; }

  /// @brief FairTask: ReInit method
  InitStatus ReInit() override;

  /// @brief FairTask: sets parameter containers up
  void SetParContainers() override;

 private:
  inline static CbmTofTrackingInterface* fpInstance{nullptr};  ///< Instance of the class

  CbmTofDigiPar* fDigiPar{nullptr};
  CbmTofDigiBdfPar* fDigiBdfPar{nullptr};

  std::vector<double> fTofStationZ{};     ///< Centers of TOF stations along z-axis [cm]
  std::vector<double> fTofStationZMin{};  ///< Lower bounds of TOF stations along z-axis [cm]
  std::vector<double> fTofStationZMax{};  ///< Upper bounds of TOF stations along z-axis [cm]

  ClassDefOverride(CbmTofTrackingInterface, 0);
};

#endif  // CbmTofTrackingInterface
