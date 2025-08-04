/* Copyright (C) 2021-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaStationInitializer.h
/// \brief  An interface for the CA Station object initialization
/// \since  18.12.2021
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaObjectInitController.h"
#include "CaSimd.h"
#include "CaStation.h"

#include <bitset>
#include <functional>
#include <string>

namespace cbm::algo::ca
{
  enum class EDetectorID;

  /// A base class which provides interface to L1Algo station geometry
  class StationInitializer {
    /// Enumeration for internal logic control
    enum class EManagementFlag
    {
      kThicknessMapMoved,  ///< if the thickness map was moved from the StationInitializer instance
      kEnd
    };
    using ManagementFlags_t = std::bitset<static_cast<int>(EManagementFlag::kEnd)>;

   public:
    /// \enum  EInitKey
    /// \brief Enumeration of fields, which must be initialized so the object can pass the threshold
    enum class EInitKey
    {
      // Basic fields initialization
      kDetectorID,      ///< detector ID
      kStationID,       ///< station ID
      kTrackingStatus,  ///< flag, if station is used in tracking or not
      kXmax,            ///< max size in X direction
      kYmax,            ///< max size in Y direction
      // ca::Station initialization
      kType,          ///< station type
      kTimeInfo,      ///< if time info is used (flag)
      kFieldStatus,   ///< if station is placed in field (flag)
      kZref,          ///< reference z of the station
      kZmin,          ///< min z of the station
      kZmax,          ///< max z of the station
      kFieldSlice,    ///< ca::Station.ca::FieldSlice object initialization
      kGeoLayerID,    ///< index of geo layer in geometrical setup (including possibly inactive stations)
      // The last item is equal to the number of bits in fInitFlags
      kEnd
    };
    using InitController_t = ca::ObjectInitController<static_cast<int>(EInitKey::kEnd), EInitKey>;

    /// \brief Default constructor
    StationInitializer() = default;

    /// \brief Constructor from stationID and detectorID
    StationInitializer(EDetectorID detetorID, int stationID) noexcept;

    /// \brief Destructor
    ~StationInitializer() noexcept = default;

    /// \brief Copy constructor
    StationInitializer(const StationInitializer& other) = default;

    /// \brief Less operator for StationInitializer object to sort stations in Z
    bool operator<(const StationInitializer& right) const { return (GetZref() < right.GetZref()); }

    /// \brief Gets detector ID
    EDetectorID GetDetectorID() const { return fDetectorID; }

    /// \brief Gets field status: 0 - station is outside the field, 1 - station is inside the field
    int GetFieldStatus() const { return fStation.fieldStatus; }

    /// \brief Gets a const reference to the ca::ObjectInitController object
    const InitController_t& GetInitController() const { return fInitController; }

    /// \brief Gets a reference to ca::Station info field of the L1BaseStation info
    const Station<fvec>& GetStation() const;

    /// \brief Gets station ID
    int GetStationID() const { return fStationID; }

    /// \brief Gets station type
    int GetStationType() const { return fStation.type; }

    /// \brief Gets tracking status: true - station is active for tracking, false - station exists, but not used in tracking
    bool GetTrackingStatus() const { return fTrackingStatus; }

    /// \brief Gets maximum distance between station center and its edge in x direction
    double GetXmax() const { return fXmax; }

    /// \brief Gets maximum distance between station center and its edge in y direction
    double GetYmax() const { return fYmax; }

    /// \brief Gets double precised z position of the station [cm]
    double GetZref() const { return fZref; }

    /// \brief Gets min z of the station [cm]
    double GetZmin() const { return fZmin; }

    /// \brief Gets max z of the station [cm]
    double GetZmax() const { return fZmax; }

    /// \brief Gets SIMD vectorized z position of the station [cm]
    fvec GetZsimdVec() const { return fStation.fZ; }

    /// \brief Resets fields to the default values
    void Reset();

    /// \brief Sets detector ID
    void SetDetectorID(EDetectorID inID);

    /// \brief Sets flag: true - station is placed in field, false - station is placed outside the field
    void SetFieldStatus(int fieldStatus);

    /// \brief Sets arrays of the approcimation
    /// \param getField A user function, which gets a xyz array of position coordinates and fills B array
    ///                 of magnetic field components in position
    void SetFieldFunction(const std::function<void(const double (&xyz)[3], double (&B)[3])>& getFieldValue);

    /// \brief Sets geometry ID (index of the layer in the geometrical setup)
    void SetGeoLayerID(int geoLayerID);

    /// \brief Sets station ID
    [[deprecated("Please, use constructor to set station ID")]] void SetStationID(int inID);

    /// \brief Sets type of station
    void SetStationType(int inType);  // TODO: this is a temporary solution (S.Zh.)

    /// \brief Sets flag: 0 - time information is not provided by this detector type
    ///                   1 - time information is provided by the detector and can be used in tracking
    void SetTimeInfo(int inTimeInfo);

    /// \brief Sets tracking status: true - station is active for tracking, false - station exists, but not used in tracking
    void SetTrackingStatus(bool flag);

    /// \brief Sets maximum distance between station center and its edge in x direction
    void SetXmax(double aSize);

    /// \brief Sets maximum distance between station center and its edge in y direction
    void SetYmax(double aSize);

    /// \brief Sets reference z position of the station
    void SetZref(double inZ);

    /// \brief Sets min z of the station
    void SetZmin(double inZ);

    /// \brief Sets max z of the station
    void SetZmax(double inZ);

    /// \brief Swap method for easy implementation of move constructor and copy and move assignment operator
    void Swap(StationInitializer& other) noexcept;

    /// \brief String representation of class contents
    /// \param indentLevel  Number of indent characters in the output
    std::string ToString(int verbosityLevel = 0, int indentLevel = 0) const;

   private:
    EDetectorID fDetectorID{static_cast<EDetectorID>(0)};  ///< Detector ID
    int fStationID{-1};                                    ///< Local ID of a station
    bool fTrackingStatus{false};                           ///< Tracking status: true - station is used for tracking
    double fXmax{0};                       ///< Maximum distance between station center and its edge in x direction
    double fYmax{0};                       ///< Maximum distance between station center and its edge in y direction
    double fZref{0};                       ///< reference z
    double fZmin{0};                       ///< min z
    double fZmax{0};                       ///< max z
    Station<fvec> fStation{};              ///< ca::Station structure, describes a station in L1Algo
    InitController_t fInitController{};    ///< Class fileds initialization flags
    ManagementFlags_t fManagementFlags{};  ///< bitset flags to manage internal behaviour of the class
  };

  /// swap function for two StationInitializer objects, expected to be used instead of std::swap
  inline void swap(StationInitializer& a, StationInitializer& b) noexcept { a.Swap(b); }
}  // namespace cbm::algo::ca
