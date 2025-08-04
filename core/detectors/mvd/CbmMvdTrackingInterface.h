/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmMvdTrackingInterface.h
 * @brief  Input data and parameters interface from MVD subsystem used in L1 tracker (declaration)
 * @since  31.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#ifndef CbmMvdTrackingInterface_h
#define CbmMvdTrackingInterface_h 1

#include "CbmHit.h"
#include "CbmMvdDetectorId.h"
#include "CbmMvdHit.h"
#include "CbmMvdStationPar.h"
#include "CbmTrackingDetectorInterfaceBase.h"

#include <FairTask.h>  // for InitStatus, FairTask

#include <Rtypes.h>  // for ClassDef
#include <TMath.h>   // for Pi

#include <algorithm>
#include <limits>
#include <string>

class TBuffer;
class TClass;
class TMemberInspector;

/// @brief CbmMvdTrackingInterface
/// @brief A CbmL1 subtask, which provides necessary methods for L1 tracker to access the geometry and dataflow
///        settings.
///
class CbmMvdTrackingInterface : public FairTask, public CbmTrackingDetectorInterfaceBase, public CbmMvdDetectorId {
public:
  /// @brief Default constructor
  CbmMvdTrackingInterface();

  /// @brief Destructor
  ~CbmMvdTrackingInterface();

  /// @brief Copy constructor
  CbmMvdTrackingInterface(const CbmMvdTrackingInterface&) = delete;

  /// @brief Move constructor
  CbmMvdTrackingInterface(CbmMvdTrackingInterface&&) = delete;

  /// @brief Copy assignment operator
  CbmMvdTrackingInterface& operator=(const CbmMvdTrackingInterface&) = delete;

  /// @brief Move assignment operator
  CbmMvdTrackingInterface& operator=(CbmMvdTrackingInterface&&) = delete;

  /// @brief Gets name of this subsystem
  std::string GetDetectorName() const override { return "MVD"; }

  /// --- to be removed --- Gets the tracking station radiation length
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Radiation length [cm]
  // TODO: remove this method
  [[deprecated]] double GetRadLength(int stationId) const
  {
    return fMvdStationPar->GetZThickness(stationId) / (10. * fMvdStationPar->GetZRadThickness(stationId));
  }

  /// @brief  Gets stereo angles of the two independent measured coordinates
  /// @note   The tracking does not use this method. It is only used by the QA task.
  /// @param  address  detector unique identifier
  /// @return [phiU, phiV] - Stereo angles [rad]
  std::tuple<double, double> GetStereoAnglesSensor(int /*address*/) const override
  {
    return std::tuple(0., TMath::Pi() / 2.);
  }

  /// --- to be removed --- Gets the tracking station thickness along the Z-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Station thickness [cm]
  // TODO: remove this method
  [[deprecated]] double GetSensorThickness(int stationId) const { return fMvdStationPar->GetZThickness(stationId); }

  /// @brief  Gets a tracking station of a CbmHit
  /// @param  hit  A pointer to CbmHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const CbmHit* hit) const override
  {
    auto hitMvd = [&] {
      if constexpr (kUseDynamicCast) { return dynamic_cast<const CbmMvdHit*>(hit); }
      else {
        return static_cast<const CbmMvdHit*>(hit);
      }
    }();
    return hitMvd->GetStationNr();
  }

  /// @brief  Gets a tracking station of a FairMCPoint
  /// @param  point  A pointer to FairMCHit
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(const FairMCPoint* point) const override;

  /// @brief  Gets a tracking station by the address of element (detectorID in terms of MVD)
  /// @param  detectorId  Unique element address (detectorID in terms of MVD)
  /// @return Local index of the tracking station
  int GetTrackingStationIndex(int detectorId) const override { return StationNr(detectorId); }


  /// @brief FairTask: Init method
  InitStatus Init() override;

  /// @brief Gets pointer to the instance of the CbmMvdTrackingInterface
  static CbmMvdTrackingInterface* Instance() { return fpInstance; }

  /// @brief  Check if the detector provides time measurements
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Flag: true - station provides time measurements, false - station does not provide time measurements
  bool IsTimeInfoProvided(int /*stationId*/) const override { return true; }

  /// @brief FairTask: ReInit method
  InitStatus ReInit() override;

  /// @brief FairTask: sets parameter containers up
  void SetParContainers() override;

 private:
  inline static CbmMvdTrackingInterface* fpInstance{nullptr};  ///< Instance of the class

  const CbmMvdStationPar* fMvdStationPar {nullptr};  ///< Pointer to the Mvd station parameters

  ClassDefOverride(CbmMvdTrackingInterface, 0);
};

#endif  // CbmMvdTrackingInterface
