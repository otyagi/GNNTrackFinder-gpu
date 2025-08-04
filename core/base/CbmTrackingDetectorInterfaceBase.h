/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/***************************************************************************************************
 * @file   CbmTrackingDetectorInterfaceBase.h
 * @brief  Base abstract class for tracking detector interface to L1
 * @since  31.05.2022
 * @author S.Zharko <s.zharko@gsi.de>
 ***************************************************************************************************/

#ifndef CbmTrackingDetectorInterfaceBase_h
#define CbmTrackingDetectorInterfaceBase_h 1

#include "CbmHit.h"
#include "CbmPixelHit.h"
#include "FairMCPoint.h"

#include <array>
#include <cmath>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

class TGeoNode;
class TString;

/// @class CbmTrackingDetectorInterfaceBase
/// @brief Abstract class, which should be inherited by every detecting subsystem tracking interface class
///
class CbmTrackingDetectorInterfaceBase {
 public:
  /// @struct VolumeInfo
  /// @brief  Structure to store geometry information of each station
  struct VolumeInfo {
    double fXmin{+std::numeric_limits<double>::max()};  ///< Lower bound in x-direction [cm]
    double fXmax{-std::numeric_limits<double>::max()};  ///< Upper bound in x-direction [cm]
    double fYmin{+std::numeric_limits<double>::max()};  ///< Lower bound in y-direction [cm]
    double fYmax{-std::numeric_limits<double>::max()};  ///< Upper bound in y-direction [cm]
    double fZmin{+std::numeric_limits<double>::max()};  ///< Lower bound in z-direction [cm]
    double fZmax{-std::numeric_limits<double>::max()};  ///< Upper bound in z-direction [cm]

    /// @brief Constructor from parameters
    VolumeInfo(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
      : fXmin(xMin)
      , fXmax(xMax)
      , fYmin(yMin)
      , fYmax(yMax)
      , fZmin(zMin)
      , fZmax(zMax)
    {
    }

    /// @brief Rule of five
    VolumeInfo() = default;
    //VolumeInfo(const VolumeInfo&) = default;
    //VolumeInfo(VolumeInfo&&) = default;
    //VolumeInfo& operator=(const VolumeInfo&) = default;
    //VolumeInfo& operator=(VolumeInfo&&) = default;

    /// @brief Compound assingment of another volume
    VolumeInfo& operator+=(const VolumeInfo& other)
    {
      fXmin = std::min(fXmin, other.fXmin);
      fXmax = std::max(fXmax, other.fXmax);
      fYmin = std::min(fYmin, other.fYmin);
      fYmax = std::max(fYmax, other.fYmax);
      fZmin = std::min(fZmin, other.fZmin);
      fZmax = std::max(fZmax, other.fZmax);
      return *this;
    }

    /// @brief String representation of the structure
    std::string ToString() const;
  };

 public:
  /// @brief Virtual destructor
  virtual ~CbmTrackingDetectorInterfaceBase() {}

  /// @brief Checks detector interface: boundary conditions of the parameters
  bool Check() const;

  /// @brief Returns the name of the detector subsystem
  virtual std::string GetDetectorName() const = 0;

  /// @brief Gets actual number of stations, provided by the current geometry setup
  int GetNtrackingStations() const { return fvStationFullVolume.size(); }

  /// @brief Gets stereo angles of the two independent measured coordinates
  /// @note  The tracking does not use this method. It is only used by the QA task.
  /// @param  address  detector unique identifier
  /// @return [phiU, phiV] - Stereo angles [rad]
  virtual std::tuple<double, double> GetStereoAnglesSensor(int address) const = 0;

  /// @brief  Gets a tracking station of a CbmHit
  /// @param  hit  A pointer to CbmHit
  /// @return Local index of the tracking station
  virtual int GetTrackingStationIndex(const CbmHit* hit) const = 0;

  /// @brief  Gets a tracking station of a FairMCPoint
  /// @param  point  A pointer to FairMCHit
  /// @return Local index of the tracking station
  virtual int GetTrackingStationIndex(const FairMCPoint* point) const = 0;

  /// @brief  Gets a tracking station by the address of element
  /// @param  address  Unique element address
  /// @return Local index of the tracking station
  virtual int GetTrackingStationIndex(int address) const = 0;

  /// @brief  Gets upper bound of a station passive volume along the X-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Size of station along the X-axis [cm]
  double GetXmax(int stationId) const { return fvStationFullVolume[stationId].fXmax; }

  /// @brief  Gets lower bound of a station passive volume along the X-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Size of station along the X-axis [cm]
  double GetXmin(int stationId) const { return fvStationFullVolume[stationId].fXmin; }

  /// @brief  Gets upper bound of a station passive volume along the Y-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Size of station along the Y-axis [cm]
  double GetYmax(int stationId) const { return fvStationFullVolume[stationId].fYmax; }

  /// @brief  Gets lower bound of a station passive volume along the Y-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Size of station along the Y-axis [cm]
  double GetYmin(int stationId) const { return fvStationFullVolume[stationId].fYmin; }

  /// @brief  Gets reference z of the station passive volume
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return Reference z position of the station [cm]
  double GetZref(int stationId) const { return 0.5 * (GetActiveZmin(stationId) + GetActiveZmax(stationId)); }

  /// @brief  Gets reference z of the detector module (e.g., RPC for TOF)
  /// @param  address  Address of the module
  /// @return Reference z of module (if not defined, the ref z of the station will be returned)
  virtual double GetZrefModule(int address) { return GetZref(GetTrackingStationIndex(address)); }

  /// @brief  Gets min z of the station passive volume
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return min Z of the station [cm]
  double GetZmin(int stationId) const { return fvStationFullVolume[stationId].fZmin; }

  /// @brief  Gets max z of the station passive volume
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return max Z of the station [cm]
  double GetZmax(int stationId) const { return fvStationFullVolume[stationId].fZmax; }

  /// @brief  Gets lower bound of the station active volume along x-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return min X of the station active volume [cm]
  double GetActiveXmin(int stationId) const { return fvStationActiveVolume[stationId].fXmin; }

  /// @brief  Gets upper bound of the station active volume along x-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return max X of the station active volume [cm]
  double GetActiveXmax(int stationId) const { return fvStationActiveVolume[stationId].fXmax; }

  /// @brief  Gets lower bound of the station active volume along y-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return min Y of the station active volume [cm]
  double GetActiveYmin(int stationId) const { return fvStationActiveVolume[stationId].fYmin; }

  /// @brief  Gets upper bound of the station active volume along y-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return max Y of the station active volume [cm]
  double GetActiveYmax(int stationId) const { return fvStationActiveVolume[stationId].fYmax; }

  /// @brief  Gets lower bound of the station active volume along z-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return min Z of the station active volume [cm]
  double GetActiveZmin(int stationId) const { return fvStationActiveVolume[stationId].fZmin; }

  /// @brief  Gets upper bound of the station active volume along z-axis
  /// @param  stationId  Tracking station ID in the setup (NOTE: must be in range [0..GetNstations()-1])
  /// @return max Z of the station active volume [cm]
  double GetActiveZmax(int stationId) const { return fvStationActiveVolume[stationId].fZmax; }

  /// @brief  Check if station provides time measurements
  /// @param  stationId  Tracking station ID in the setup
  /// @return Flag: true - station provides time measurements, false - station does not provide time measurements
  virtual bool IsTimeInfoProvided(int stationId) const = 0;

  /// @brief  Gets x,y,t ranges of a CbmPixelHit
  /// @param  hit  A hit
  /// @return range X, Y, T
  virtual std::tuple<double, double, double> GetHitRanges(const CbmPixelHit& hit) const
  {
    // by default assume gaussian distributions of errors
    return std::tuple(3.5 * hit.GetDx(), 3.5 * hit.GetDy(), 3.5 * hit.GetTimeError());
  }

  /// Technical flag: true - all downcasts are done with dynamic_cast followed by checks for nullptr (increases
  /// computing time, better for debug), false - all downcasts are done with static_cast without sanity checks
  /// (decreases computing time, but can cause bugs)
  static constexpr bool kUseDynamicCast{true};

  /// @brief Prints all the parameters into table and saves the table as a string
  std::string ToString() const;

 protected:
  /// \brief If use legacy tracking detector interface definition
  /// FIXME:  Remove after testing
  static constexpr bool kLegacy{false};

  /// @brief Collects paths of the components
  /// @param[in]    detector  Name hint of the detector
  /// @param[in]    component Name of the active node component
  /// @param[inout] path      Path to the node
  /// @param[inout] pNode     Pointer to the node
  /// @return                 Vector of paths to the matched components
  std::vector<TString> CollectNodes(const TString& detector, const TString& component, const TString& path,
                                    TGeoNode* pNode);

  /// @brief Creates volume info from a geo node provided by path
  /// @param path  Path to volume in TGeoManager
  /// @throw std::runtime_error  If node is not found by path
  VolumeInfo ReadVolume(const TString& path);

  // NOTE: SZh 10.09.2024: For now the passive and active volumes destinguish in TRD and TOF only. For STS
  //                       the z-components are active, and the x- and y-components are passive. For
  //                       MVD and MuCh the components are legacy (active? taken from digitizers).
  std::vector<VolumeInfo> fvStationFullVolume{};    ///< Geometric properties of each station passive volume
  std::vector<VolumeInfo> fvStationActiveVolume{};  ///< Geometric properties of each station active volume
};

#endif  // CbmTrackingDetectorInterfaceBase_h
